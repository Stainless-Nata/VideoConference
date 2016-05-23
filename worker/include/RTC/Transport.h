#ifndef MS_RTC_TRANSPORT_H
#define MS_RTC_TRANSPORT_H

#include "common.h"
#include "RTC/UdpSocket.h"
#include "RTC/TcpServer.h"
#include "RTC/TcpConnection.h"
#include "RTC/IceCandidate.h"
#include "RTC/IceServer.h"
#include "RTC/StunMessage.h"
#include "RTC/TransportTuple.h"
#include "RTC/DtlsTransport.h"
#include "RTC/SrtpSession.h"
#include "RTC/RtpReceiver.h"
#include "RTC/RtpPacket.h"
#include "RTC/RtcpPacket.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class Transport :
		public RTC::UdpSocket::Listener,
		public RTC::TcpServer::Listener,
		public RTC::TcpConnection::Listener,
		public RTC::IceServer::Listener,
		public RTC::DtlsTransport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onTransportClosed(RTC::Transport* transport) = 0;
		};

	public:
		class RtpListener
		{
		public:
			bool HasSsrc(uint32_t ssrc, RTC::RtpReceiver* rtpReceiver);
			bool HasMuxId(std::string& muxId, RTC::RtpReceiver* rtpReceiver);
			bool HasPayloadType(uint8_t payloadType, RTC::RtpReceiver* rtpReceiver);

		public:
			// Table of SSRC / RtpReceiver pairs.
			std::unordered_map<uint32_t, RTC::RtpReceiver*> ssrcTable;
			//  Table of MID RTP header extension / RtpReceiver pairs.
			std::unordered_map<std::string, RTC::RtpReceiver*> muxIdTable;
			// Table of RTP payload type / RtpReceiver pairs.
			std::unordered_map<uint8_t, RTC::RtpReceiver*> ptTable;
		};

	public:
		Transport(Listener* listener, Channel::Notifier* notifier, uint32_t transportId, Json::Value& data);
		virtual ~Transport();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void AddRtpReceiver(RTC::RtpReceiver* rtpReceiver);
		void RemoveRtpReceiver(RTC::RtpReceiver* rtpReceiver);
		void SendRtpPacket(RTC::RtpPacket* packet);

	private:
		void MayRunDtlsTransport();
		RTC::RtpReceiver* GetRtpReceiver(RTC::RtpPacket* packet);
		void RollbackRtpReceiver(RTC::RtpReceiver* rtpReceiver, std::vector<uint32_t>& previousSsrcs, std::string& previousMuxId, std::vector<uint8_t>& previousPayloadTypes);

	/* Private methods to unify UDP and TCP behavior. */
	private:
		void onPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onStunDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onDtlsDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

	/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		virtual void onPacketRecv(RTC::UdpSocket *socket, const uint8_t* data, size_t len, const struct sockaddr* remote_addr) override;

	/* Pure virtual methods inherited from RTC::TcpServer::Listener. */
	public:
		virtual void onRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection, bool is_closed_by_peer) override;

	/* Pure virtual methods inherited from RTC::TcpConnection::Listener. */
	public:
		virtual void onPacketRecv(RTC::TcpConnection *connection, const uint8_t* data, size_t len) override;

	/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		virtual void onOutgoingStunMessage(RTC::IceServer* iceServer, RTC::StunMessage* msg, RTC::TransportTuple* tuple) override;
		virtual void onIceSelectedTuple(IceServer* iceServer, RTC::TransportTuple* tuple) override;
		virtual void onIceConnected(IceServer* iceServer) override;
		virtual void onIceCompleted(IceServer* iceServer) override;
		virtual void onIceDisconnected(IceServer* iceServer) override;

	/* Pure virtual methods inherited from RTC::DtlsTransport::Listener. */
	public:
		virtual void onDtlsConnecting(DtlsTransport* dtlsTransport) override;
		virtual void onDtlsConnected(DtlsTransport* dtlsTransport, RTC::SrtpSession::Profile srtp_profile, uint8_t* srtp_local_key, size_t srtp_local_key_len, uint8_t* srtp_remote_key, size_t srtp_remote_key_len, std::string& remoteCert) override;
		virtual void onDtlsFailed(DtlsTransport* dtlsTransport) override;
		virtual void onDtlsClosed(DtlsTransport* dtlsTransport) override;
		virtual void onOutgoingDtlsData(RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;
		virtual void onDtlsApplicationData(RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;

	public:
		// Passed by argument.
		uint32_t transportId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Allocated by this.
		RTC::IceServer* iceServer = nullptr;
		std::vector<RTC::UdpSocket*> udpSockets;
		std::vector<RTC::TcpServer*> tcpServers;
		RTC::DtlsTransport* dtlsTransport = nullptr;
		RTC::SrtpSession* srtpRecvSession = nullptr;
		RTC::SrtpSession* srtpSendSession = nullptr;
		// Others.
		bool allocated = false;
		// Others (ICE).
		std::vector<IceCandidate> iceLocalCandidates;
		RTC::TransportTuple* selectedTuple = nullptr;
		// Others (DTLS).
		bool remoteDtlsParametersGiven = false;
		RTC::DtlsTransport::Role dtlsLocalRole = RTC::DtlsTransport::Role::AUTO;
		// Others (RtpListener).
		RtpListener rtpListener;
	};

	/* Inline instance methods. */

	inline
	bool Transport::RtpListener::HasSsrc(uint32_t ssrc, RTC::RtpReceiver* rtpReceiver)
	{
		auto it = this->ssrcTable.find(ssrc);

		if (it == this->ssrcTable.end())
		{
			return false;
		}
		else
		{
			return (it->second != rtpReceiver);
		}
	}

	inline
	bool Transport::RtpListener::HasMuxId(std::string& muxId, RTC::RtpReceiver* rtpReceiver)
	{
		auto it = this->muxIdTable.find(muxId);

		if (it == this->muxIdTable.end())
		{
			return false;
		}
		else
		{
			return (it->second != rtpReceiver);
		}
	}

	inline
	bool Transport::RtpListener::HasPayloadType(uint8_t payloadType, RTC::RtpReceiver* rtpReceiver)
	{
		auto it = this->ptTable.find(payloadType);

		if (it == this->ptTable.end())
		{
			return false;
		}
		else
		{
			return (it->second != rtpReceiver);
		}
	}
}

#endif
