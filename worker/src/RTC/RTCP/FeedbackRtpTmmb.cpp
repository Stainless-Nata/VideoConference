#define MS_CLASS "RTC::RTCP::FeedbackRtpTmmb"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpTmmb.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */
	template <typename T>
	FeedbackRtpTmmbItem<T>* FeedbackRtpTmmbItem<T>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header + length value.
		if (HeaderSize > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Tmmb item, discarded");

			return nullptr;
		}

		std::unique_ptr<FeedbackRtpTmmbItem> item(new FeedbackRtpTmmbItem(data));

		if (!item->IsCorrect())
			return nullptr;

		return item.release();
	}

	/* Instance methods. */
	template <typename T>
	FeedbackRtpTmmbItem<T>::FeedbackRtpTmmbItem(const uint8_t* data)
	{
		this->ssrc = Utils::Byte::Get4Bytes(data, 0);

		// Read the 4 bytes block.
		uint32_t compact = Utils::Byte::Get4Bytes(data, 4);
		// Read each component.
		uint8_t exponent = compact >> 26;             // 6 bits.
		uint64_t mantissa = (compact >> 9) & 0x1ffff; // 17 bits.

		this->overhead = compact & 0x1ff;             // 9 bits.

		// Get the bitrate out of exponent and mantissa.
		this->bitrate = (mantissa << exponent);

		if ((this->bitrate >> exponent) != mantissa)
		{
			MS_WARN_TAG(rtcp, "invalid TMMB bitrate value : %" PRIu64" x 2^%" PRIu8, mantissa, exponent);

			this->isCorrect = false;
		}
	}

	template <typename T>
	size_t FeedbackRtpTmmbItem<T>::Serialize(uint8_t* buffer)
	{
		static constexpr uint32_t MaxMantissa = 0x1ffff; // 17 bits.

		uint64_t mantissa = this->bitrate;
		uint32_t exponent = 0;

		while (mantissa > MaxMantissa)
		{
			mantissa >>= 1;
			++exponent;
		}

		Utils::Byte::Set4Bytes(buffer, 0, this->ssrc);

		uint32_t compact = (exponent << 26) | (mantissa << 9) | this->overhead;

		Utils::Byte::Set4Bytes(buffer, 4, compact);

		return HeaderSize;
	}

	template <typename T>
	void FeedbackRtpTmmbItem<T>::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<FeedbackRtpTmmbItem>");
		MS_DUMP("  ssrc     : %" PRIu32, this->GetSsrc());
		MS_DUMP("  bitrate  : %" PRIu64, this->GetBitrate());
		MS_DUMP("  overhead : %" PRIu16, this->GetOverhead());
		MS_DUMP("</FeedbackRtpTmmbItem>");
	}

	/* Specialization for Tmmbr class. */

	template<>
	const FeedbackRtp::MessageType FeedbackRtpTmmbItem<FeedbackRtpTmmbr>::MessageType = FeedbackRtp::MessageType::TMMBR;

	/* Specialization for Tmmbn class. */

	template<>
	const FeedbackRtp::MessageType FeedbackRtpTmmbItem<FeedbackRtpTmmbn>::MessageType = FeedbackRtp::MessageType::TMMBN;

	// Explicit instantiation to have all FeedbackRtpTmmbItem definitions in this file.
	template class FeedbackRtpTmmbItem<FeedbackRtpTmmbr>;
	template class FeedbackRtpTmmbItem<FeedbackRtpTmmbn>;
}}
