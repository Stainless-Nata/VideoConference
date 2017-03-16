#define MS_CLASS "RTC::RTCP::FeedbackRtp"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/FeedbackRtpTmmb.hpp"
#include "RTC/RTCP/FeedbackRtpTllei.hpp"
#include "RTC/RTCP/FeedbackRtpEcn.hpp"
#include "Logger.hpp"

namespace RTC { namespace RTCP
{
	/* Class methods. */

	template<typename Item>
	FeedbackRtpItemsPacket<Item>* FeedbackRtpItemsPacket<Item>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

			return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::unique_ptr<FeedbackRtpItemsPacket<Item>> packet(new FeedbackRtpItemsPacket<Item>(commonHeader));

		size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

		while (len - offset > 0)
		{
			Item* item = Item::Parse(data+offset, len-offset);

			if (item)
			{
				packet->AddItem(item);
				offset += item->GetSize();
			}
			else
			{
				break;
			}
		}

		return packet.release();
	}

	/* Instance methods. */

	template<typename Item>
	size_t FeedbackRtpItemsPacket<Item>::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		size_t offset = FeedbackPacket::Serialize(buffer);

		for (auto item : this->items)
		{
			offset += item->Serialize(buffer + offset);
		}

		return offset;
	}

	template<typename Item>
	void FeedbackRtpItemsPacket<Item>::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<%s>", FeedbackRtpPacket::MessageType2String(Item::MessageType).c_str());
		FeedbackRtpPacket::Dump();
		for (auto item : this->items)
		{
			item->Dump();
		}
		MS_DUMP("</%s>", FeedbackRtpPacket::MessageType2String(Item::MessageType).c_str());
	}

	// Explicit instantiation to have all FeedbackRtpPacket definitions in this file.
	template class FeedbackRtpItemsPacket<FeedbackRtpNackItem>;
	template class FeedbackRtpItemsPacket<FeedbackRtpTmmbrItem>;
	template class FeedbackRtpItemsPacket<FeedbackRtpTmmbnItem>;
	template class FeedbackRtpItemsPacket<FeedbackRtpTlleiItem>;
	template class FeedbackRtpItemsPacket<FeedbackRtpEcnItem>;
}}
