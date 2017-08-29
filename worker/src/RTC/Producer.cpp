#define MS_CLASS "RTC::Producer"
// #define MS_LOG_DEV

#include "RTC/Producer.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/Transport.hpp"

namespace RTC
{
	/* Instance methods. */

	Producer::Producer(
	    Listener* listener, Channel::Notifier* notifier, uint32_t producerId, RTC::Media::Kind kind, RTC::Transport* transport)
	    : producerId(producerId), kind(kind), listener(listener), notifier(notifier), transport(transport)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
	}

	Producer::~Producer()
	{
		MS_TRACE();

		delete this->rtpParameters;

		ClearRtpStreams();
	}

	void Producer::Destroy()
	{
		MS_TRACE();

		// Notify.
		this->notifier->Emit(this->producerId, "close");

		// Notify the listener.
		this->listener->OnProducerClosed(this);

		delete this;
	}

	Json::Value Producer::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProducerId{ "producerId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringHasTransport{ "hasTransport" };
		static const Json::StaticString JsonStringSsrcAudioLevelId{ "ssrcAudioLevelId" };
		static const Json::StaticString JsonStringAbsSendTimeId{ "absSendTimeId" };
		static const Json::StaticString JsonStringRtpRawEventEnabled{ "rtpRawEventEnabled" };
		static const Json::StaticString JsonStringRtpObjectEventEnabled{ "rtpObjectEventEnabled" };
		static const Json::StaticString JsonStringRtpStreams{ "rtpStreams" };
		static const Json::StaticString JsonStringRtpStream{ "rtpStream" };

		Json::Value json(Json::objectValue);
		Json::Value jsonRtpStreams(Json::arrayValue);

		json[JsonStringProducerId] = Json::UInt{ this->producerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		if (this->rtpParameters != nullptr)
			json[JsonStringRtpParameters] = this->rtpParameters->ToJson();
		else
			json[JsonStringRtpParameters] = Json::nullValue;

		json[JsonStringHasTransport] = this->transport != nullptr;

		if (this->knownHeaderExtensions.ssrcAudioLevelId)
			json[JsonStringSsrcAudioLevelId] = this->knownHeaderExtensions.ssrcAudioLevelId;

		if (this->knownHeaderExtensions.absSendTimeId)
			json[JsonStringAbsSendTimeId] = this->knownHeaderExtensions.absSendTimeId;

		json[JsonStringRtpRawEventEnabled] = this->rtpRawEventEnabled;

		json[JsonStringRtpObjectEventEnabled] = this->rtpObjectEventEnabled;

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			jsonRtpStreams.append(rtpStream->ToJson());
		}
		json[JsonStringRtpStreams] = jsonRtpStreams;

		return json;
	}

	void Producer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::PRODUCER_CLOSE:
			{
#ifdef MS_LOG_DEV
				uint32_t producerId = this->producerId;
#endif

				Destroy();

				MS_DEBUG_DEV("Producer closed [producerId:%" PRIu32 "]", producerId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::PRODUCER_RECEIVE:
			{
				static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
				static const Json::StaticString JsonStringRtpMapping{ "rtpMapping" };

				if (!request->data[JsonStringRtpParameters].isObject())
				{
					request->Reject("missing data.rtpParameters");

					return;
				}
				else if (!request->data[JsonStringRtpMapping].isObject())
				{
					request->Reject("missing data.rtpMapping");

					return;
				}

				if (this->rtpParameters)
				{
					request->Reject("Producer already has RTP parameters");

					return;
				}

				try
				{
					// NOTE: This may throw.
					this->rtpParameters = new RTC::RtpParameters(request->data[JsonStringRtpParameters]);

					auto transport = GetTransport();

					// NOTE: This may throw.
					if (transport != nullptr)
						transport->AddProducer(this);

					// NOTE: This may throw.
					CreateRtpMapping(request->data[JsonStringRtpMapping]);
				}
				catch (const MediaSoupError& error)
				{
					if (this->rtpParameters)
					{
						delete this->rtpParameters;
						this->rtpParameters = nullptr;
					}

					request->Reject(error.what());

					return;
				}

				// Create RtpStreamRecv instances.
				for (auto& encoding : this->rtpParameters->encodings)
				{
					CreateRtpStream(encoding);
				}

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_PAUSE:
			{
				this->paused = true;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_RESUME:
			{
				bool wasPaused = this->paused;

				this->paused = false;

				request->Accept();

				if (wasPaused)
					RequestFullFrame();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_SET_RTP_RAW_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpRawEventEnabled = request->data[JsonStringEnabled].asBool();

				request->Accept();

				// If set, require a full frame.
				if (this->rtpRawEventEnabled)
					RequestFullFrame();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_SET_RTP_OBJECT_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpObjectEventEnabled = request->data[JsonStringEnabled].asBool();

				request->Accept();

				// If set, require a full frame.
				if (this->rtpObjectEventEnabled)
					RequestFullFrame();

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void Producer::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringObject{ "object" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringMarker{ "marker" };
		static const Json::StaticString JsonStringSequenceNumber{ "sequenceNumber" };
		static const Json::StaticString JsonStringTimestamp{ "timestamp" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };

		// Find the corresponding RtpStreamRecv.
		uint32_t ssrc = packet->GetSsrc();

		RTC::RtpStreamRecv* rtpStream{ nullptr };

		if (this->rtpStreams.find(ssrc) != this->rtpStreams.end())
		{
			rtpStream = this->rtpStreams[ssrc];

			// Process the packet.
			if (!rtpStream->ReceivePacket(packet))
				return;
		}
		else if (this->mapRtxStreams.find(ssrc) != this->mapRtxStreams.end())
		{
			rtpStream = this->mapRtxStreams[ssrc];

			// Process the packet.
			if (!rtpStream->ReceiveRtxPacket(packet))
				return;
		}
		else
		{
			MS_WARN_TAG(rtp, "no RtpStream found for given RTP packet [ssrc:%" PRIu32 "]", ssrc);

			return;
		}

		// If paused stop here.
		if (this->paused)
			return;

		// Emit "rtpraw" if enabled.
		if (this->rtpRawEventEnabled)
		{
			this->notifier->EmitWithBinary(
			    this->producerId, "rtpraw", packet->GetData(), packet->GetSize());
		}

		// Emit "rtpobject" is enabled.
		if (this->rtpObjectEventEnabled)
		{
			Json::Value eventData(Json::objectValue);
			Json::Value jsonObject(Json::objectValue);

			jsonObject[JsonStringPayloadType]    = Json::UInt{ packet->GetPayloadType() };
			jsonObject[JsonStringMarker]         = packet->HasMarker();
			jsonObject[JsonStringSequenceNumber] = Json::UInt{ packet->GetSequenceNumber() };
			jsonObject[JsonStringTimestamp]      = Json::UInt{ packet->GetTimestamp() };
			jsonObject[JsonStringSsrc]           = Json::UInt{ packet->GetSsrc() };

			eventData[JsonStringObject] = jsonObject;

			this->notifier->EmitWithBinary(
			    this->producerId, "rtpobject", packet->GetPayload(), packet->GetPayloadLength(), eventData);
		}

		// Apply the Producer RTP mapping before dispatching the packet to the Router.
		ApplyRtpMapping(packet);

		// Notify the listener.
		this->listener->OnProducerRtpPacket(this, packet);
	}

	void Producer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;
			auto* report   = rtpStream->GetRtcpReceiverReport();

			report->SetSsrc(rtpStream->GetSsrc());
			packet->AddReceiverReport(report);
		}

		this->lastRtcpSentTime = now;
	}

	void Producer::ReceiveRtcpFeedback(RTC::RTCP::FeedbackPsPacket* packet) const
	{
		MS_TRACE();

		if (this->transport == nullptr)
			return;

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet->GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

			return;
		}

		packet->Serialize(RTC::RTCP::Buffer);
		this->transport->SendRtcpPacket(packet);
	}

	void Producer::ReceiveRtcpFeedback(RTC::RTCP::FeedbackRtpPacket* packet) const
	{
		MS_TRACE();

		if (this->transport == nullptr)
			return;

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet->GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

			return;
		}

		packet->Serialize(RTC::RTCP::Buffer);
		this->transport->SendRtcpPacket(packet);
	}

	void Producer::RequestFullFrame() const
	{
		MS_TRACE();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			rtpStream->RequestFullFrame();
		}
	}

	void Producer::CreateRtpMapping(Json::Value& rtpMapping)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringCodecPayloadTypes{ "codecPayloadTypes" };
		static const Json::StaticString JsonStringHeaderExtensionIds{ "headerExtensionIds" };

		if (!rtpMapping[JsonStringCodecPayloadTypes].isArray())
			MS_THROW_ERROR("missing rtpMapping.codecPayloadTypes");
		else if (!rtpMapping[JsonStringHeaderExtensionIds].isArray())
			MS_THROW_ERROR("missing rtpMapping.headerExtensionIds");

		for (auto& pair : rtpMapping[JsonStringCodecPayloadTypes])
		{
			if (!pair.isArray() || pair.size() != 2 || !pair[0].isUInt() || !pair[1].isUInt())
				MS_THROW_ERROR("wrong rtpMapping entry");

			auto sourcePayloadType = static_cast<uint8_t>(pair[0].asUInt());
			auto mappedPayloadType = static_cast<uint8_t>(pair[1].asUInt());

			this->rtpMapping.codecPayloadTypes[sourcePayloadType] = mappedPayloadType;
		}

		for (auto& pair : rtpMapping[JsonStringHeaderExtensionIds])
		{
			if (!pair.isArray() || pair.size() != 2 || !pair[0].isUInt() || !pair[1].isUInt())
				MS_THROW_ERROR("wrong rtpMapping entry");

			auto sourceHeaderExtensionId = static_cast<uint8_t>(pair[0].asUInt());
			auto mappedHeaderExtensionId = static_cast<uint8_t>(pair[1].asUInt());

			this->rtpMapping.headerExtensionIds[sourceHeaderExtensionId] = mappedHeaderExtensionId;
		}

		// Also, fill the id of well known RTP header extensions with the mapped ids
		// (if any).

		auto& idMapping = this->rtpMapping.headerExtensionIds;
		uint8_t ssrcAudioLevelId{ 0 };
		uint8_t absSendTimeId{ 0 };

		for (auto& exten : this->rtpParameters->headerExtensions)
		{
			if (this->kind == RTC::Media::Kind::AUDIO && (ssrcAudioLevelId == 0u) &&
			    exten.type == RTC::RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL)
			{
				if (idMapping.find(exten.id) != idMapping.end())
					ssrcAudioLevelId = idMapping[exten.id];
				else
					ssrcAudioLevelId = exten.id;

				this->knownHeaderExtensions.ssrcAudioLevelId = ssrcAudioLevelId;
			}

			if ((absSendTimeId == 0u) && exten.type == RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME)
			{
				if (idMapping.find(exten.id) != idMapping.end())
					absSendTimeId = idMapping[exten.id];
				else
					absSendTimeId = exten.id;

				this->knownHeaderExtensions.absSendTimeId = absSendTimeId;
			}
		}
	}

	void Producer::CreateRtpStream(RTC::RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		// Don't create an RtpStreamRecv if the encoding has no SSRC.
		// TODO: For simulcast or, if not announced, this would be done
		// dynamicall by the RtpListener when matching a RID with its SSRC.
		if (encoding.ssrc == 0u)
			return;

		uint32_t ssrc = encoding.ssrc;

		// Don't create a RtpStreamRecv if there is already one for the same SSRC.
		// TODO: This may not work for SVC codecs.
		if (this->rtpStreams.find(ssrc) != this->rtpStreams.end())
			return;

		// Get the codec of the stream/encoding.
		auto& codec = this->rtpParameters->GetCodecForEncoding(encoding);
		bool useNack{ false };
		bool usePli{ false };
		bool useRemb{ false };

		for (auto& fb : codec.rtcpFeedback)
		{
			if (!useNack && fb.type == "nack")
			{
				MS_DEBUG_TAG(rtcp, "enabling NACK generation");

				useNack = true;
			}
			if (!usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_TAG(rtcp, "enabling PLI generation");

				usePli = true;
			}
			else if (!useRemb && fb.type == "goog-remb")
			{
				MS_DEBUG_TAG(rbe, "enabling REMB");

				useRemb = true;
			}
		}

		// Create stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = ssrc;
		params.payloadType = codec.payloadType;
		params.mime        = codec.mime;
		params.clockRate   = codec.clockRate;
		params.useNack     = useNack;
		params.usePli      = usePli;

		// Create a RtpStreamRecv for receiving a media stream.
		this->rtpStreams[ssrc] = new RTC::RtpStreamRecv(this, params);

		// Enable REMB in the transport if requested.
		if (useRemb)
			this->transport->EnableRemb();

		// Check rtx capabilities.
		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			if (this->mapRtxStreams.find(encoding.rtx.ssrc) != this->mapRtxStreams.end())
				return;

			auto& codec    = this->rtpParameters->GetRtxCodecForEncoding(encoding);
			auto rtpStream = this->rtpStreams[ssrc];

			rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
			this->mapRtxStreams[encoding.rtx.ssrc] = rtpStream;
		}
	}

	void Producer::ClearRtpStreams()
	{
		MS_TRACE();

		for (auto& kv : this->rtpStreams)
		{
			auto rtpStream = kv.second;

			delete rtpStream;
		}

		this->rtpStreams.clear();
	}

	void Producer::OnNackRequired(RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers)
	{
		if (this->transport == nullptr)
			return;

		RTC::RTCP::FeedbackRtpNackPacket packet(0, rtpStream->GetSsrc());
		auto it        = seqNumbers.begin();
		const auto end = seqNumbers.end();

		while (it != end)
		{
			uint16_t seq;
			uint16_t bitmask{ 0 };

			seq = *it;
			++it;

			while (it != end)
			{
				uint16_t shift = *it - seq - 1;

				if (shift <= 15)
				{
					bitmask |= (1 << shift);
					++it;
				}
				else
				{
					break;
				}
			}

			auto nackItem = new RTC::RTCP::FeedbackRtpNackItem(seq, bitmask);

			packet.AddItem(nackItem);
		}

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet.GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtx, "cannot send RTCP NACK packet, size too big (%zu bytes)", packet.GetSize());

			return;
		}

		packet.Serialize(RTC::RTCP::Buffer);
		this->transport->SendRtcpPacket(&packet);
	}

	void Producer::OnPliRequired(RTC::RtpStreamRecv* rtpStream)
	{
		// TODO: Can this happen?
		if (this->transport == nullptr)
			return;

		RTC::RTCP::FeedbackPsPliPacket packet(0, rtpStream->GetSsrc());

		packet.Serialize(RTC::RTCP::Buffer);

		// Send two, because it's free.
		this->transport->SendRtcpPacket(&packet);
		this->transport->SendRtcpPacket(&packet);
	}

	void Producer::ApplyRtpMapping(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		auto& codecPayloadTypeMap  = this->rtpMapping.codecPayloadTypes;
		auto& headerExtensionIdMap = this->rtpMapping.headerExtensionIds;
		auto payloadType           = packet->GetPayloadType();

		// Mangle payload type.

		if (codecPayloadTypeMap.find(payloadType) != codecPayloadTypeMap.end())
		{
			packet->SetPayloadType(codecPayloadTypeMap[payloadType]);
		}

		// Mangle header extension ids.

		packet->MangleExtensionHeaderIds(headerExtensionIdMap);

		if (this->knownHeaderExtensions.ssrcAudioLevelId)
		{
			packet->AddExtensionMapping(
			    RtpHeaderExtensionUri::Type::SSRC_AUDIO_LEVEL,
			    this->knownHeaderExtensions.ssrcAudioLevelId);
		}

		if (this->knownHeaderExtensions.absSendTimeId)
		{
			packet->AddExtensionMapping(
			    RtpHeaderExtensionUri::Type::ABS_SEND_TIME, this->knownHeaderExtensions.absSendTimeId);
		}
	}
} // namespace RTC
