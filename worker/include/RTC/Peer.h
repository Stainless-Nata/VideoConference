#ifndef MS_RTC_PEER_H
#define MS_RTC_PEER_H

#include "common.h"
#include "RTC/Transport.h"
#include "RTC/RtpReceiver.h"
#include "RTC/RtpSender.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class Peer :
		public RTC::Transport::Listener,
		public RTC::RtpReceiver::Listener,
		public RTC::RtpSender::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onPeerClosed(RTC::Peer* peer) = 0;
			virtual void onPeerRtpReceiverReady(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) = 0;
		};

	public:
		Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName);
		virtual ~Peer();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);

	private:
		RTC::Transport* GetTransportFromRequest(Channel::Request* request, uint32_t* transportId = nullptr);
		RTC::RtpReceiver* GetRtpReceiverFromRequest(Channel::Request* request, uint32_t* rtpReceiverId = nullptr);
		RTC::RtpSender* GetRtpSenderFromRequest(Channel::Request* request, uint32_t* rtpSenderId = nullptr);

	/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		virtual void onTransportClosed(RTC::Transport* transport) override;

	/* Pure virtual methods inherited from RTC::RtpReceiver::Listener. */
	public:
		virtual void onRtpReceiverParameters(RTC::RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters) override;
		virtual void onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver) override;

	/* Pure virtual methods inherited from RTC::RtpSender::Listener. */
	public:
		virtual void onRtpSenderParameters(RTC::RtpSender* rtpSender, RTC::RtpParameters* rtpParameters) override;
		virtual void onRtpSenderClosed(RTC::RtpSender* rtpSender) override;

	public:
		// Passed by argument.
		uint32_t peerId;
		std::string peerName;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Others.
		std::unordered_map<uint32_t, RTC::Transport*> transports;
		std::unordered_map<uint32_t, RTC::RtpReceiver*> rtpReceivers;
		std::unordered_map<uint32_t, RTC::RtpSender*> rtpSenders;
	};
}

#endif
