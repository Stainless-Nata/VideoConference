#ifndef MS_RTC_RTP_DICTIONARIES_H
#define MS_RTC_RTP_DICTIONARIES_H

#include "common.h"
#include "RTC/CustomParameters.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <json/json.h>

namespace RTC
{
	class Media
	{
	public:
		enum class Kind : uint8_t
		{
			AUDIO = 1,
			VIDEO,
			DEPTH
		};

	public:
		static Kind GetKind(std::string& str);
		static Json::StaticString& GetJsonString(Kind kind);

	private:
		static std::unordered_map<std::string, Kind> string2Kind;
		static std::map<Kind, Json::StaticString> kind2Json;
	};

	class RtpCodecMime
	{
	public:
		enum class Type : uint8_t
		{
			UNSET = 0,
			AUDIO,
			VIDEO
		};

	public:
		enum class Subtype : uint16_t
		{
			UNSET = 0,
			// Audio codecs:
			OPUS = 100,
			PCMA,
			PCMU,
			ISAC,
			G722,
			// Video codecs:
			VP8 = 200,
			VP9,
			H264,
			H265,
			// Feature codecs:
			RTX = 1000,
			ULPFEC,
			FLEXFEC,
			RED,
			CN,
			TELEPHONE_EVENT
		};

	private:
		static std::unordered_map<std::string, Type> string2Type;
		static std::map<Type, std::string> type2String;
		static std::unordered_map<std::string, Subtype> string2Subtype;
		static std::map<Subtype, std::string> subtype2String;

	public:
		RtpCodecMime() {};
		virtual ~RtpCodecMime();

		void SetName(std::string& name);

		std::string& ToString()
		{
			return this->name;
		}

		bool IsMediaCodec()
		{
			return this->subtype >= Subtype::OPUS && this->subtype < Subtype::RTX;
		}

		bool IsFeatureCodec()
		{
			return this->subtype >= Subtype::RTX;
		}

	public:
		Type    type = Type::UNSET;
		Subtype subtype = Subtype::UNSET;

	private:
		std::string name;
	};

	class RtcpFeedback
	{
	public:
		RtcpFeedback(Json::Value& data);
		virtual ~RtcpFeedback();

		Json::Value toJson();

	public:
		std::string type;
		std::string parameter;
	};

	class RtpCodecParameters
	{
	public:
		RtpCodecParameters(Json::Value& data);
		virtual ~RtpCodecParameters();

		Json::Value toJson();

	public:
		std::string               name;
		uint8_t                   payloadType = 0;
		uint32_t                  clockRate = 0;
		uint32_t                  maxptime = 0;
		uint32_t                  ptime = 0;
		uint32_t                  numChannels = 0;
		std::vector<RtcpFeedback> rtcpFeedback;
		RTC::CustomParameters     parameters;

	public:
		RtpCodecMime              mime;
	};

	class RtpFecParameters
	{
	public:
		RtpFecParameters() {};
		RtpFecParameters(Json::Value& data);
		virtual ~RtpFecParameters();

		Json::Value toJson();

	public:
		std::string mechanism;
		uint32_t    ssrc = 0;
	};

	class RtpRtxParameters
	{
	public:
		RtpRtxParameters() {};
		RtpRtxParameters(Json::Value& data);
		virtual ~RtpRtxParameters();

		Json::Value toJson();

	public:
		uint32_t ssrc = 0;
	};

	class RtpEncodingParameters
	{
	public:
		RtpEncodingParameters(Json::Value& data);
		RtpEncodingParameters();
		virtual ~RtpEncodingParameters();

		Json::Value toJson();

	public:
		uint32_t                 ssrc = 0;
		uint8_t                  codecPayloadType = 0;
		bool                     hasCodecPayloadType = false;
		RtpFecParameters         fec;
		bool                     hasFec = false;
		RtpRtxParameters         rtx;
		bool                     hasRtx = false;
		double                   resolutionScale = 1.0;
		double                   framerateScale = 1.0;
		uint32_t                 maxFramerate = 0;
		bool                     active = true;
		std::string              encodingId;
		std::vector<std::string> dependencyEncodingIds;
	};

	class RtpHeaderExtensionParameters
	{
	public:
		RtpHeaderExtensionParameters() {};
		RtpHeaderExtensionParameters(Json::Value& data);
		virtual ~RtpHeaderExtensionParameters();

		Json::Value toJson();

	public:
		std::string           uri;
		uint16_t              id = 0;
		bool                  encrypt = false;
		RTC::CustomParameters parameters;
	};

	class RtcpParameters
	{
	public:
		RtcpParameters() {};
		RtcpParameters(Json::Value& data);
		virtual ~RtcpParameters();

		Json::Value toJson();

	public:
		std::string cname;
		uint32_t    ssrc = 0;
		bool        reducedSize = false;
	};

	class RtpParameters
	{
	public:
		// Constructor for receiver's parameters.
		RtpParameters(Json::Value& data);
		// Constructor for sender's parameters.
		RtpParameters(const RtpParameters* RtpParameters);
		virtual ~RtpParameters();

		Json::Value toJson();

	private:
		void ValidateCodecs();
		void ValidateEncodings();

	public:
		std::string                               muxId;
		std::vector<RtpCodecParameters>           codecs;
		std::vector<RtpEncodingParameters>        encodings;
		std::vector<RtpHeaderExtensionParameters> headerExtensions;
		RtcpParameters                            rtcp;
		bool                                      hasRtcp = false;
		Json::Value                               userParameters;
	};

	class RtpCodecCapability
	{
	public:
		RtpCodecCapability(Json::Value& data);
		virtual ~RtpCodecCapability();

		Json::Value toJson();

	public:
		Media::Kind               kind;
		std::string               name;
		uint8_t                   preferredPayloadType = 0;
		uint32_t                  clockRate = 0;
		uint32_t                  maxptime = 0;
		uint32_t                  ptime = 0;
		uint32_t                  numChannels = 0;
		std::vector<RtcpFeedback> rtcpFeedback;
		RTC::CustomParameters     parameters;
		uint16_t                  maxTemporalLayers = 0;
		uint16_t                  maxSpatialLayers = 0;
		bool                      svcMultiStreamSupport = false;

	public:
		RtpCodecMime              mime;
	};

	class RtpHeaderExtension
	{
	public:
		RtpHeaderExtension(Json::Value& data);
		virtual ~RtpHeaderExtension();

		Json::Value toJson();

	public:
		Media::Kind kind;
		std::string uri;
		uint16_t    preferredId = 0;
		bool        preferredEncrypt = false;
	};

	class RtpCapabilities
	{
	public:
		RtpCapabilities() {};
		RtpCapabilities(Json::Value& data);
		virtual ~RtpCapabilities();

		Json::Value toJson();

	private:
		void ValidateCodecs();

	public:
		std::vector<RtpCodecCapability> codecs;
		std::vector<RtpHeaderExtension> headerExtensions;
		std::vector<std::string>        fecMechanisms;
	};
}

#endif
