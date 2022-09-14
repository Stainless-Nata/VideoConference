# mediasoup v3

## Design Goals

mediasoup and its client side libraries are designed to accomplish with the following goals:

* Be a [SFU](https://webrtcglossary.com/sfu/) (Selective Forwarding Unit).
* Support both WebRTC and plain RTP input and output.
* Be a Node.js module/Rust crate in server side.
* Be a tiny JavaScript and C++ libraries in client side.
* Be minimalist: just handle the media layer.
* Be signaling agnostic: do not mandate any signaling protocol.
* Be super low level API.
* Support all existing WebRTC endpoints.
* Enable integration with well known multimedia libraries/tools.


## Architecture

![][mediasoup-architecture]


## Use Cases

mediasoup and its client side libraries provide a super low level API. They are intended to enable different use cases and scenarios, without any constraint or assumption. Some of these use cases are:

* Group video chat applications.
* One-to-many (or few-to-many) broadcasting applications in real-time.
* RTP streaming.


## Features

* ECMAScript 6/Idiomatic Rust low level API.
* Multi-stream: multiple audio/video streams over a single ICE + DTLS transport.
* IPv6 ready.
* ICE / DTLS / RTP / RTCP over UDP and TCP.
* Simulcast and SVC support.
* Congestion control.
* Sender and receiver bandwidth estimation with spatial/temporal layers distribution algorithm.
* Data message exchange (via WebRTC DataChannels, SCTP over plain UDP, and direct termination in Node.js/Rust).
* Extremely powerful (media worker thread/subprocess coded in C++ on top of [libuv](https://libuv.org)).


## Sponsor

You can support mediasoup by [sponsoring][sponsor] it. Thanks!


## License

[ISC](./LICENSE)
