#define MS_CLASS "RTC::RtpCodecParameters"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(Json::Value& data) :
		RtpCodec(data)
	{
		MS_TRACE();

		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_parameters("parameters");

		// `payloadType` is mandatory.
		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing RtpCodecParameters.payloadType");

		this->payloadType = (uint8_t)data[k_payloadType].asUInt();

		// `rtx` is optional.
		if (data[k_rtx].isObject())
		{
			this->rtx = RTCRtpCodecRtxParameters(data[k_rtx]);
			this->hasRtx = true;
		}

		// `rtcpFeedback` is optional.
		if (data[k_rtcpFeedback].isArray())
		{
			auto& json_rtcpFeedback = data[k_rtcpFeedback];

			for (Json::UInt i = 0; i < json_rtcpFeedback.size(); i++)
			{
				RtcpFeedback rtcpFeedback(json_rtcpFeedback[i]);

				// Append to the rtcpFeedback vector.
				this->rtcpFeedback.push_back(rtcpFeedback);
			}
		}
	}

	Json::Value RtpCodecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_name("name");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_parameters("parameters");

		Json::Value json(Json::objectValue);

		// Add `name`.
		json[k_name] = this->mime.GetName();

		// Add `payloadType`.
		json[k_payloadType] = (Json::UInt)this->payloadType;

		// Add `clockRate`.
		json[k_clockRate] = (Json::UInt)this->clockRate;

		// Add `maxptime`.
		if (this->maxptime)
			json[k_maxptime] = (Json::UInt)this->maxptime;

		// Add `ptime`.
		if (this->ptime)
			json[k_ptime] = (Json::UInt)this->ptime;

		// Add `numChannels`.
		if (this->numChannels > 1)
			json[k_numChannels] = (Json::UInt)this->numChannels;

		// Add `rtx`
		if (this->hasRtx)
			json[k_rtx] = this->rtx.toJson();

		// Add `rtcpFeedback`.
		json[k_rtcpFeedback] = Json::arrayValue;

		for (auto& entry : this->rtcpFeedback)
		{
			json[k_rtcpFeedback].append(entry.toJson());
		}

		// Add `parameters`.
		json[k_parameters] = this->parameters.toJson();

		return json;
	}
}
