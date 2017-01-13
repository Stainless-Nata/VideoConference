#define MS_CLASS "TEST::RTP"
#define MS_LOG_DEV

#include "fct.h"
#include "common.h"
#include "RTC/RtpPacket.h"
#include "Logger.h"
#include <fstream>

using namespace RTC;

FCTMF_SUITE_BGN(test_rtp)
{
	FCT_TEST_BGN(parse_rtp_packet)
	{
		// DOC: http://insanecoding.blogspot.com.es/2011/11/how-to-read-in-file-in-c.html

		uint8_t buffer[65536];
		std::ifstream in("worker/test/data/packet1.raw", std::ios::ate | std::ios::binary);

		if (in)
		{
			size_t file_size = (size_t)in.tellg() - 1;

			in.seekg(0, std::ios::beg);

			in.read((char*)buffer, file_size);
			in.close();

			RtpPacket* packet = RtpPacket::Parse(buffer, file_size);

			if (!packet)
			{
				MS_ERROR("not a RTP packet");

				// TODO: fail the test here.
				return;
			}

			// packet->Dump();

			fct_chk(packet->HasMarker() == false);
			fct_chk(packet->HasExtensionHeader() == true);
			fct_chk_eq_int(packet->GetExtensionHeaderId(), 48862);
			fct_chk_eq_int(packet->GetExtensionHeaderLength(), 4);
			fct_chk_eq_int(packet->GetPayloadType(), 111);
			fct_chk_eq_int(packet->GetSequenceNumber(), 23617);
			fct_chk_eq_int(packet->GetTimestamp(), 1660241882);
			fct_chk_eq_int(packet->GetSsrc(), 2674985186);

			delete packet;
		}
		else
		{
			// TODO: What here? how to fail the test?
			MS_ERROR("file not found");
		}
	}
	FCT_TEST_END()
}
FCTMF_SUITE_END();
