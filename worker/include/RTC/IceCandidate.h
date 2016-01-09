#ifndef MS_RTC_ICE_CANDIDATE_H
#define MS_RTC_ICE_CANDIDATE_H

#include "common.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	class IceCandidate
	{
	public:
		enum class Protocol
		{
			UDP = 1,
			TCP
		};

	public:
		enum class CandidateType
		{
			HOST = 1
		};

	public:
		enum class TcpCandidateType
		{
			PASSIVE = 1
		};

	public:
		IceCandidate(RTC::UDPSocket* udpSocket, uint64_t priority);
		IceCandidate(RTC::TCPServer* tcpServer, uint64_t priority);

		Json::Value toJson();

	private:
		// Others.
		std::string foundation;
		uint64_t priority;
		std::string ip;
		Protocol protocol;
		MS_PORT port;
		CandidateType type;
		TcpCandidateType tcpType;
	};
}

#endif
