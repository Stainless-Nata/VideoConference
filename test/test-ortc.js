const { toBeType } = require('jest-tobetype');
const { UnsupportedError } = require('../lib/errors');
const ortc = require('../lib/ortc');

expect.extend({ toBeType });

test('generateRouterRtpCapabilities() succeeds', () =>
{
	const mediaCodecs =
	[
		{
			kind       : 'audio',
			mimeType   : 'audio/opus',
			clockRate  : 48000,
			channels   : 2,
			parameters :
			{
				useinbandfec : 1,
				foo          : 'bar'
			}
		},
		{
			kind                 : 'video',
			preferredPayloadType : 125, // Let's force it.
			mimeType             : 'video/VP8',
			clockRate            : 90000
		},
		{
			kind         : 'video',
			mimeType     : 'video/H264',
			clockRate    : 90000,
			rtcpFeedback : [], // Will be ignored.
			parameters   :
			{
				'level-asymmetry-allowed' : 1,
				'profile-level-id'        : '42e01f',
				foo                       : 'bar'
			}
		}
	];

	const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

	expect(rtpCapabilities.codecs.length).toBe(5);

	// opus.
	expect(rtpCapabilities.codecs[0]).toEqual(
		{
			kind                 : 'audio',
			mimeType             : 'audio/opus',
			preferredPayloadType : 100, // 100 is the first available dynamic PT.
			clockRate            : 48000,
			channels             : 2,
			rtcpFeedback         : [],
			parameters           :
			{
				useinbandfec : 1,
				foo          : 'bar'
			}
		});

	// VP8.
	expect(rtpCapabilities.codecs[1]).toEqual(
		{
			kind                 : 'video',
			mimeType             : 'video/VP8',
			preferredPayloadType : 125,
			clockRate            : 90000,
			rtcpFeedback         :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' },
				{ type: 'transport-cc' }
			],
			parameters : {}
		});

	// VP8 RTX.
	expect(rtpCapabilities.codecs[2]).toEqual(
		{
			kind                 : 'video',
			mimeType             : 'video/rtx',
			preferredPayloadType : 101, // 101 is the second available dynamic PT.
			clockRate            : 90000,
			rtcpFeedback         : [],
			parameters           :
			{
				apt : 125
			}
		});

	// H264.
	expect(rtpCapabilities.codecs[3]).toEqual(
		{
			kind                 : 'video',
			mimeType             : 'video/H264',
			preferredPayloadType : 102, // 102 is the second available dynamic PT.
			clockRate            : 90000,
			rtcpFeedback         :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' },
				{ type: 'goog-remb' },
				{ type: 'transport-cc' }
			],
			parameters :
			{

				'packetization-mode'      : 0,
				'level-asymmetry-allowed' : 1,
				'profile-level-id'        : '42e01f',
				foo                       : 'bar'
			}
		});

	// H264 RTX.
	expect(rtpCapabilities.codecs[4]).toEqual(
		{
			kind                 : 'video',
			mimeType             : 'video/rtx',
			preferredPayloadType : 103,
			clockRate            : 90000,
			rtcpFeedback         : [],
			parameters           :
			{
				apt : 102
			}
		});
});

test('generateRouterRtpCapabilities() with unsupported codecs throws UnsupportedError', () =>
{
	let mediaCodecs;

	mediaCodecs =
	[
		{
			kind      : 'audio',
			mimeType  : 'audio/chicken',
			clockRate : 8000,
			channels  : 4
		}
	];

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow(UnsupportedError);

	mediaCodecs =
	[
		{
			kind      : 'audio',
			mimeType  : 'audio/opus',
			clockRate : 48000,
			channels  : 1
		}
	];

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow(UnsupportedError);

	mediaCodecs =
	[
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 5
			}
		}
	];

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow(UnsupportedError);
});

test('generateRouterRtpCapabilities() with too many codecs throws', () =>
{
	const mediaCodecs = [];

	for (let i = 0; i < 100; ++i)
	{
		mediaCodecs.push(
			{
				kind      : 'audio',
				mimeType  : 'audio/opus',
				clockRate : 48000,
				channels  : 2
			});
	}

	expect(() => ortc.generateRouterRtpCapabilities(mediaCodecs))
		.toThrow('cannot allocate');
});

test('getProducerRtpParametersMapping(), getConsumableRtpParameters(), getConsumerRtpParameters() and getPipeConsumerRtpParameters() succeed', () =>
{
	const mediaCodecs =
	[
		{
			kind      : 'audio',
			mimeType  : 'audio/opus',
			clockRate : 48000,
			channels  : 2
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'level-asymmetry-allowed' : 1,
				'packetization-mode'      : 1,
				'profile-level-id'        : '4d0032',
				bar                       : 'lalala'
			}
		}
	];

	const routerRtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

	const rtpParameters =
	{
		codecs :
		[
			{
				mimeType     : 'video/H264',
				payloadType  : 111,
				clockRate    : 90000,
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'goog-remb' }
				],
				parameters :
				{
					foo                  : 1234,
					'packetization-mode' : 1,
					'profile-level-id'   : '4d0032'
				}
			},
			{
				mimeType    : 'video/rtx',
				payloadType : 112,
				clockRate   : 90000,
				parameters  :
				{
					apt : 111
				}
			}
		],
		headerExtensions :
		[
			{
				uri : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				id  : 1
			},
			{
				uri : 'urn:3gpp:video-orientation',
				id  : 2
			}
		],
		encodings :
		[
			{
				ssrc            : 11111111,
				rtx             : { ssrc: 11111112 },
				maxBitrate      : 111111,
				scalabilityMode : 'L1T3'
			},
			{
				ssrc            : 21111111,
				rtx             : { ssrc: 21111112 },
				maxBitrate      : 222222,
				scalabilityMode : 'L1T3'
			},
			{
				rid             : 'high',
				maxBitrate      : 333333,
				scalabilityMode : 'L1T3'
			}
		],
		rtcp :
		{
			cname : 'qwerty1234'
		}
	};

	const rtpMapping =
		ortc.getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);

	expect(rtpMapping.codecs).toEqual(
		[
			{ payloadType: 111, mappedPayloadType: 101 },
			{ payloadType: 112, mappedPayloadType: 102 }
		]);

	expect(rtpMapping.encodings[0].ssrc).toBe(11111111);
	expect(rtpMapping.encodings[0].rid).toBe(undefined);
	expect(rtpMapping.encodings[0].mappedSsrc).toBeType('number');
	expect(rtpMapping.encodings[1].ssrc).toBe(21111111);
	expect(rtpMapping.encodings[1].rid).toBe(undefined);
	expect(rtpMapping.encodings[1].mappedSsrc).toBeType('number');
	expect(rtpMapping.encodings[2].ssrc).toBe(undefined);
	expect(rtpMapping.encodings[2].rid).toBe('high');
	expect(rtpMapping.encodings[2].mappedSsrc).toBeType('number');

	const consumableRtpParameters = ortc.getConsumableRtpParameters(
		'video', rtpParameters, routerRtpCapabilities, rtpMapping);

	expect(consumableRtpParameters.codecs[0].mimeType).toBe('video/H264');
	expect(consumableRtpParameters.codecs[0].payloadType).toBe(101);
	expect(consumableRtpParameters.codecs[0].clockRate).toBe(90000);
	expect(consumableRtpParameters.codecs[0].parameters).toEqual(
		{
			foo                  : 1234,
			'packetization-mode' : 1,
			'profile-level-id'   : '4d0032'
		});

	expect(consumableRtpParameters.codecs[1].mimeType).toBe('video/rtx');
	expect(consumableRtpParameters.codecs[1].payloadType).toBe(102);
	expect(consumableRtpParameters.codecs[1].clockRate).toBe(90000);
	expect(consumableRtpParameters.codecs[1].parameters).toEqual({ apt: 101 });

	expect(consumableRtpParameters.encodings[0]).toEqual(
		{
			ssrc            : rtpMapping.encodings[0].mappedSsrc,
			maxBitrate      : 111111,
			scalabilityMode : 'L1T3'
		});
	expect(consumableRtpParameters.encodings[1]).toEqual(
		{
			ssrc            : rtpMapping.encodings[1].mappedSsrc,
			maxBitrate      : 222222,
			scalabilityMode : 'L1T3'
		});
	expect(consumableRtpParameters.encodings[2]).toEqual(
		{
			ssrc            : rtpMapping.encodings[2].mappedSsrc,
			maxBitrate      : 333333,
			scalabilityMode : 'L1T3'
		});

	expect(consumableRtpParameters.rtcp).toEqual(
		{
			cname       : rtpParameters.rtcp.cname,
			reducedSize : true,
			mux         : true
		});

	const remoteRtpCapabilities =
	{
		codecs :
		[
			{
				kind                 : 'audio',
				mimeType             : 'audio/opus',
				preferredPayloadType : 100,
				clockRate            : 48000,
				channels             : 2
			},
			{
				kind                 : 'video',
				mimeType             : 'video/H264',
				preferredPayloadType : 101,
				clockRate            : 90000,
				rtcpFeedback         :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'pli' },
					{ type: 'foo', parameter: 'FOO' }
				],
				parameters :
				{
					'packetization-mode' : 1,
					'profile-level-id'   : '4d0032',
					baz                  : 'LOLOLO'
				}
			},
			{
				kind                 : 'video',
				mimeType             : 'video/rtx',
				preferredPayloadType : 102,
				clockRate            : 90000,
				parameters           :
				{
					apt : 101
				}
			}
		],
		headerExtensions :
		[
			{
				kind             : 'audio',
				uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				preferredId      : 1,
				preferredEncrypt : false
			},
			{
				kind             : 'video',
				uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
				preferredId      : 1,
				preferredEncrypt : false
			},
			{
				kind             : 'video',
				uri              : 'urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id',
				preferredId      : 2,
				preferredEncrypt : false
			},
			{
				kind             : 'audio',
				uri              : 'urn:ietf:params:rtp-hdrext:ssrc-audio-level',
				preferredId      : 8,
				preferredEncrypt : false
			},
			{
				kind             : 'video',
				uri              : 'urn:3gpp:video-orientation',
				preferredId      : 11,
				preferredEncrypt : false
			},
			{
				kind             : 'video',
				uri              : 'urn:ietf:params:rtp-hdrext:toffset',
				preferredId      : 12,
				preferredEncrypt : false
			}
		]
	};

	const consumerRtpParameters =
		ortc.getConsumerRtpParameters(consumableRtpParameters, remoteRtpCapabilities);

	expect(consumerRtpParameters.codecs.length).toEqual(2);
	expect(consumerRtpParameters.codecs[0]).toEqual(
		{
			mimeType     : 'video/H264',
			payloadType  : 101,
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack' },
				{ type: 'nack', parameter: 'pli' },
				{ type: 'foo', parameter: 'FOO' }
			],
			parameters :
			{
				foo                  : 1234,
				'packetization-mode' : 1,
				'profile-level-id'   : '4d0032'
			}
		});
	expect(consumerRtpParameters.codecs[1]).toEqual(
		{
			mimeType     : 'video/rtx',
			payloadType  : 102,
			clockRate    : 90000,
			rtcpFeedback : [],
			parameters   :
			{
				apt : 101
			}
		});

	expect(consumerRtpParameters.encodings.length).toBe(1);
	expect(consumerRtpParameters.encodings[0].ssrc).toBeType('number');
	expect(consumerRtpParameters.encodings[0].rtx).toBeType('object');
	expect(consumerRtpParameters.encodings[0].rtx.ssrc).toBeType('number');
	expect(consumerRtpParameters.encodings[0].scalabilityMode).toBe('S3T3');

	expect(consumerRtpParameters.headerExtensions).toEqual(
		[
			{
				uri : 'urn:3gpp:video-orientation',
				id  : 11
			},
			{
				uri : 'urn:ietf:params:rtp-hdrext:toffset',
				id  : 12
			}
		]);

	expect(consumerRtpParameters.rtcp).toEqual(
		{
			cname       : rtpParameters.rtcp.cname,
			reducedSize : true,
			mux         : true
		});

	const pipeConsumerRtpParameters =
		ortc.getPipeConsumerRtpParameters(consumableRtpParameters);

	expect(pipeConsumerRtpParameters.codecs.length).toEqual(1);
	expect(pipeConsumerRtpParameters.codecs[0]).toEqual(
		{
			mimeType     : 'video/H264',
			payloadType  : 101,
			clockRate    : 90000,
			rtcpFeedback :
			[
				{ type: 'nack', parameter: 'pli' },
				{ type: 'ccm', parameter: 'fir' }
			],
			parameters :
			{
				foo                  : 1234,
				'packetization-mode' : 1,
				'profile-level-id'   : '4d0032'
			}
		});

	expect(pipeConsumerRtpParameters.encodings.length).toBe(3);
	expect(pipeConsumerRtpParameters.encodings[0].ssrc).toBeType('number');
	expect(pipeConsumerRtpParameters.encodings[0].rtx).toBe(undefined);
	expect(pipeConsumerRtpParameters.encodings[0].maxBitrate).toBeType('number');
	expect(pipeConsumerRtpParameters.encodings[0].scalabilityMode).toBe('L1T3');
	expect(pipeConsumerRtpParameters.encodings[1].ssrc).toBeType('number');
	expect(pipeConsumerRtpParameters.encodings[1].rtx).toBe(undefined);
	expect(pipeConsumerRtpParameters.encodings[1].maxBitrate).toBeType('number');
	expect(pipeConsumerRtpParameters.encodings[1].scalabilityMode).toBe('L1T3');
	expect(pipeConsumerRtpParameters.encodings[2].ssrc).toBeType('number');
	expect(pipeConsumerRtpParameters.encodings[2].rtx).toBe(undefined);
	expect(pipeConsumerRtpParameters.encodings[2].maxBitrate).toBeType('number');
	expect(pipeConsumerRtpParameters.encodings[2].scalabilityMode).toBe('L1T3');

	expect(pipeConsumerRtpParameters.rtcp).toEqual(
		{
			cname       : rtpParameters.rtcp.cname,
			reducedSize : true,
			mux         : true
		});
});

test('getProducerRtpParametersMapping() with incompatible params throws UnsupportedError', () =>
{
	const mediaCodecs =
	[
		{
			kind      : 'audio',
			mimeType  : 'audio/opus',
			clockRate : 48000,
			channels  : 2
		},
		{
			kind       : 'video',
			mimeType   : 'video/H264',
			clockRate  : 90000,
			parameters :
			{
				'packetization-mode' : 1,
				'profile-level-id'   : '640032'
			}
		}
	];

	const routerRtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);

	const rtpParameters =
	{
		codecs :
		[
			{
				mimeType     : 'video/VP8',
				payloadType  : 120,
				clockRate    : 90000,
				rtcpFeedback :
				[
					{ type: 'nack' },
					{ type: 'nack', parameter: 'fir' }
				]
			}
		],
		headerExtensions : [],
		encodings        :
		[
			{ ssrc: 11111111 }
		],
		rtcp :
		{
			cname : 'qwerty1234'
		}
	};

	expect(
		() => ortc.getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities))
		.toThrow(UnsupportedError);
});
