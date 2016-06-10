#define MS_CLASS "RTC::RtpCodecCapability"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpCodecCapability::RtpCodecCapability(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_name("name");
		static const Json::StaticString k_preferredPayloadType("preferredPayloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_parameters("parameters");
		static const Json::StaticString k_maxTemporalLayers("maxTemporalLayers");
		static const Json::StaticString k_maxSpatialLayers("maxSpatialLayers");
		static const Json::StaticString k_svcMultiStreamSupport("svcMultiStreamSupport");

		if (!data.isObject())
			MS_THROW_ERROR("RtpCodecCapability is not an object");

		// `kind` is mandatory.
		if (!data[k_kind].isString())
			MS_THROW_ERROR("missing RtpCodecCapability.kind");

		std::string kind = data[k_kind].asString();

		// NOTE: This may throw.
		this->kind = RTC::Media::GetKind(kind);

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing RtpCodecCapability.name");

		std::string name = data[k_name].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mime.SetName(name);

		// `preferredPayloadType` is mandatory.
		if (!data[k_preferredPayloadType].isUInt())
			MS_THROW_ERROR("missing RtpCodecCapability.preferredPayloadType");

		this->preferredPayloadType = (uint8_t)data[k_preferredPayloadType].asUInt();

		// `clockRate` is optional.
		if (data[k_clockRate].isUInt())
			this->clockRate = (uint32_t)data[k_clockRate].asUInt();

		// `maxptime` is optional.
		if (data[k_maxptime].isUInt())
			this->maxptime = (uint32_t)data[k_maxptime].asUInt();

		// `ptime` is optional.
		if (data[k_ptime].isUInt())
			this->ptime = (uint32_t)data[k_ptime].asUInt();

		// `numChannels` is optional.
		if (data[k_numChannels].isUInt())
			this->numChannels = (uint32_t)data[k_numChannels].asUInt();

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

		// `parameters` is optional.
		if (data[k_parameters].isObject())
			RTC::FillCustomParameters(this->parameters, data[k_parameters]);

		// `maxTemporalLayers` is optional.
		if (data[k_maxTemporalLayers].isUInt())
			this->maxTemporalLayers = (uint16_t)data[k_maxTemporalLayers].asUInt();

		// `maxSpatialLayers` is optional.
		if (data[k_maxSpatialLayers].isUInt())
			this->maxSpatialLayers = (uint16_t)data[k_maxSpatialLayers].asUInt();

		// `svcMultiStreamSupport` is optional.
		if (data[k_svcMultiStreamSupport].isBool())
			this->svcMultiStreamSupport = data[k_svcMultiStreamSupport].asBool();
	}

	Json::Value RtpCodecCapability::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_name("name");
		static const Json::StaticString k_preferredPayloadType("preferredPayloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_parameters("parameters");
		static const Json::StaticString k_maxTemporalLayers("maxTemporalLayers");
		static const Json::StaticString k_maxSpatialLayers("maxSpatialLayers");
		static const Json::StaticString k_svcMultiStreamSupport("svcMultiStreamSupport");

		Json::Value json(Json::objectValue);

		// Add `kind`.
		json[k_kind] = RTC::Media::GetJsonString(this->kind);

		// Add `name`.
		json[k_name] = this->mime.GetName();

		// Add `preferredPayloadType`.
		json[k_preferredPayloadType] = (Json::UInt)this->preferredPayloadType;

		// Add `clockRate`.
		if (this->clockRate)
			json[k_clockRate] = (Json::UInt)this->clockRate;

		// Add `maxptime`.
		if (this->maxptime)
			json[k_maxptime] = (Json::UInt)this->maxptime;

		// Add `ptime`.
		if (this->ptime)
			json[k_ptime] = (Json::UInt)this->ptime;

		// Add `numChannels`.
		if (this->numChannels)
			json[k_numChannels] = (Json::UInt)this->numChannels;

		// Add `rtcpFeedback`.
		json[k_rtcpFeedback] = Json::arrayValue;

		for (auto& entry : this->rtcpFeedback)
		{
			json[k_rtcpFeedback].append(entry.toJson());
		}

		// Add `parameters`.
		Json::Value json_parameters(Json::objectValue);

		for (auto& kv : this->parameters)
		{
			const std::string& key = kv.first;
			auto& parameterValue = kv.second;

			json_parameters[key] = parameterValue.toJson();
		}

		json[k_parameters] = json_parameters;

		// Add `maxTemporalLayers` (if set).
		if (this->maxTemporalLayers)
			json[k_maxTemporalLayers] = (Json::UInt)this->maxTemporalLayers;

		// Add `maxSpatialLayers` (if set).
		if (this->maxSpatialLayers)
			json[k_maxSpatialLayers] = (Json::UInt)this->maxSpatialLayers;

		// Add `svcMultiStreamSupport` (if set).
		if (this->svcMultiStreamSupport)
			json[k_svcMultiStreamSupport] = true;

		return json;
	}
}
