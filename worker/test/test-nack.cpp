#include "include/catch.hpp"
#include "common.h"
#include "RTC/RtpPacket.h"
#include "RTC/RTCP/FeedbackRtpNack.h"
#include "RTC/RtpStreamSend.h"
#include <vector>

using namespace RTC;

// Can retransmit up to 17 RTP packets.
static std::vector<RtpPacket*> rtpRetransmissionContainer(18);

SCENARIO("NACK and RTP packets retransmission", "[rtp][rtcp]")
{
	SECTION("receive NACK and get retransmitted packets")
	{
		uint8_t rtp_buffer1[] =
		{
			0b10000000, 0b01111011, 0b01010010, 0b00001110,
			0b01011011, 0b01101011, 0b11001010, 0b10110101,
			0, 0, 0, 2
		};
		uint8_t rtp_buffer2[65536];
		uint8_t rtp_buffer3[65536];
		uint8_t rtp_buffer4[65536];
		uint8_t rtp_buffer5[65536];

		// packet1 [pt:123, seq:21006, timestamp:1533790901]
		RtpPacket* packet1 = RtpPacket::Parse(rtp_buffer1, sizeof(rtp_buffer1));
		REQUIRE(packet1);
		REQUIRE(packet1->GetSequenceNumber() == 21006);
		REQUIRE(packet1->GetTimestamp() == 1533790901);

		// packet2 [pt:123, seq:21007, timestamp:1533790901]
		RtpPacket* packet2 = packet1->Clone(rtp_buffer2);
		packet2->SetSequenceNumber(21007);
		packet2->SetTimestamp(1533790901);
		REQUIRE(packet2->GetSequenceNumber() == 21007);
		REQUIRE(packet2->GetTimestamp() == 1533790901);

		// packet3 [pt:123, seq:21008, timestamp:1533793871]
		RtpPacket* packet3 = packet1->Clone(rtp_buffer3);
		packet3->SetSequenceNumber(21008);
		packet3->SetTimestamp(1533793871);
		REQUIRE(packet3->GetSequenceNumber() == 21008);
		REQUIRE(packet3->GetTimestamp() == 1533793871);

		// packet4 [pt:123, seq:21009, timestamp:1533793871]
		RtpPacket* packet4 = packet1->Clone(rtp_buffer4);
		packet4->SetSequenceNumber(21009);
		packet4->SetTimestamp(1533793871);
		REQUIRE(packet4->GetSequenceNumber() == 21009);
		REQUIRE(packet4->GetTimestamp() == 1533793871);

		// packet5 [pt:123, seq:21010, timestamp:1533796931]
		RtpPacket* packet5 = packet1->Clone(rtp_buffer4);
		packet5->SetSequenceNumber(21010);
		packet5->SetTimestamp(1533796931);
		REQUIRE(packet5->GetSequenceNumber() == 21010);
		REQUIRE(packet5->GetTimestamp() == 1533796931);

		// Create a RtpStreamSend.
		RtpStreamSend* stream = new RtpStreamSend(90000, 200);

		// Receive all the packets in order into the stream.
		stream->ReceivePacket(packet1);
		stream->ReceivePacket(packet2);
		stream->ReceivePacket(packet3);
		stream->ReceivePacket(packet4);
		stream->ReceivePacket(packet5);

		// Create a NACK item that request for all the packets.
		RTCP::NackItem nack_item(21006, htons(0b0000000000001111));
		REQUIRE(nack_item.GetPacketId() == 21006);
		REQUIRE(nack_item.GetLostPacketBitmask() == 0b0000000000001111);

		stream->RequestRtpRetransmission(nack_item.GetPacketId(), nack_item.GetLostPacketBitmask(), rtpRetransmissionContainer);

		REQUIRE(rtpRetransmissionContainer[0] == packet1);
		REQUIRE(rtpRetransmissionContainer[1] == packet2);
		REQUIRE(rtpRetransmissionContainer[2] == packet3);
		REQUIRE(rtpRetransmissionContainer[3] == packet4);
		REQUIRE(rtpRetransmissionContainer[4] == packet5);
		REQUIRE(rtpRetransmissionContainer[5] == nullptr);

		// Clean stuff.
		delete packet1;
		delete packet2;
		delete packet3;
		delete packet4;
		delete packet5;
		delete stream;
	}
}
