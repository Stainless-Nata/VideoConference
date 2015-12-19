#define MS_CLASS "Channel::UnixStreamSocket"

#include "Channel/UnixStreamSocket.h"
#include "Channel/Request.h"
#include "Logger.h"
#include "MediaSoupError.h"
#include <netstring.h>
#include <cstring>  // std::memmove()
#include <cmath>  // std::ceil()
#include <cstdio>  // sprintf()

#define MESSAGE_MAX_SIZE 65536

namespace Channel
{
	/* Static variables. */

	MS_BYTE UnixStreamSocket::writeBuffer[MESSAGE_MAX_SIZE];

	/* Instance methods. */

	UnixStreamSocket::UnixStreamSocket(Listener* listener, int fd) :
		::UnixStreamSocket::UnixStreamSocket(fd, MESSAGE_MAX_SIZE),
		listener(listener)
	{
		MS_TRACE();
	}

	UnixStreamSocket::~UnixStreamSocket()
	{
		MS_TRACE();
	}

	void UnixStreamSocket::Send(Json::Value &msg)
	{
		MS_TRACE();

		// TODO: check MESSAGE_MAX_SIZE.

		Json::FastWriter fastWriter;

		fastWriter.dropNullPlaceholders();
		fastWriter.omitEndingLineFeed();

		size_t ns_num_len;
		std::string ns_payload = fastWriter.write(msg);
		size_t ns_payload_len = ns_payload.length();
		size_t ns_len;

		if (ns_payload_len == 0)
		{
			ns_num_len = 1;
			UnixStreamSocket::writeBuffer[0] = '0';
			UnixStreamSocket::writeBuffer[1] = ':';
			UnixStreamSocket::writeBuffer[2] = ',';
		}
		else
		{
		  ns_num_len = (size_t)std::ceil(std::log10((double)ns_payload_len + 1));
		  std::sprintf((char*)UnixStreamSocket::writeBuffer, "%lu:", (unsigned long)ns_payload_len);
		  std::memcpy(UnixStreamSocket::writeBuffer + ns_num_len + 1, ns_payload.c_str(), ns_payload_len);
		  UnixStreamSocket::writeBuffer[ns_num_len + ns_payload_len + 1] = ',';
		}

		ns_len = ns_num_len + ns_payload_len + 2;

		Write(UnixStreamSocket::writeBuffer, ns_len);
	}

	void UnixStreamSocket::userOnUnixStreamRead()
	{
		MS_TRACE();

		// Be ready to parse more than a single message in a single TCP chunk.
		while (true)
		{
			size_t read_len = this->bufferDataLen - this->msgStart;
			char* json_start = nullptr;
			size_t json_len;
			int ns_ret = netstring_read((char*)(this->buffer + this->msgStart), read_len,
				&json_start, &json_len);

			if (ns_ret != 0)
			{
				switch (ns_ret)
				{
					case NETSTRING_ERROR_TOO_SHORT:
						MS_DEBUG("received netstring is too short, need more data");

						// Check if the buffer is full.
						if (this->bufferDataLen == this->bufferSize)
						{
							// First case: the incomplete message does not begin at position 0 of
							// the buffer, so move the incomplete message to the position 0.
							if (this->msgStart != 0)
							{
								MS_DEBUG("no more space in the buffer, moving parsed bytes to the beginning of the buffer and waiting for more data");

								std::memmove(this->buffer, this->buffer + this->msgStart, read_len);
								this->msgStart = 0;
								this->bufferDataLen = read_len;
							}
							// Second case: the incomplete message begins at position 0 of the buffer.
							// The message is too big, so discard it.
							else
							{
								MS_ERROR("no more space in the buffer for the unfinished message being parsed, discarding it");

								this->msgStart = 0;
								this->bufferDataLen = 0;
							}
						}
						// Otherwise the buffer is not full, just wait.

						// Exit the parsing loop.
						return;

					case NETSTRING_ERROR_TOO_LONG:
						MS_THROW_ERROR("NETSTRING_ERROR_TOO_LONG");
						break;

					case NETSTRING_ERROR_NO_COLON:
						MS_THROW_ERROR("NETSTRING_ERROR_NO_COLON");
						break;

					case NETSTRING_ERROR_NO_COMMA:
						MS_THROW_ERROR("NETSTRING_ERROR_NO_COMMA");
						break;

					case NETSTRING_ERROR_LEADING_ZERO:
						MS_THROW_ERROR("NETSTRING_ERROR_LEADING_ZERO");
						break;

					case NETSTRING_ERROR_NO_LENGTH:
						MS_THROW_ERROR("NETSTRING_ERROR_NO_LENGTH");
						break;
				}

				// Error, so reset and exit the parsing loop.
				this->msgStart = 0;
				this->bufferDataLen = 0;

				return;
			}

			// If here it means that json_start points to the beginning of a JSON string
			// with json_len bytes length, so recalculate read_len.
			read_len = (const MS_BYTE*)json_start - (this->buffer + this->msgStart) + json_len + 1;

			Json::Value json;
			Json::Reader reader;

			if (!reader.parse((const char*)json_start, (const char*)json_start + json_len, json))
			{
				MS_THROW_ERROR("invalid JSON");
			}

			Channel::Request* request = Request::Factory(this, json);

			if (request)
			{
				// TODO: JEJE

				char ch = request->id.back();

				if (ch > 'm')
				{
					// jsonResponse["data"] = "🐮🐷🐣😡";
					request->Reply(200);
				}
				else
				{
					request->Reply(500);
				}

				// Delete the Request.
				delete request;
			}
			else
			{
				// TODO: throw?
				MS_ERROR("discarding wrong Channel request");
			}

			// If there is no more space available in the buffer and that is because
			// the latest parsed message filled it, then empty the full buffer.
			if ((this->msgStart + read_len) == this->bufferSize)
			{
				MS_DEBUG("no more space in the buffer, emptying the buffer data");

				this->msgStart = 0;
				this->bufferDataLen = 0;
			}
			// If there is still space in the buffer, set the beginning of the next
			// parsing to the next position after the parsed message.
			else
			{
				this->msgStart += read_len;
			}

			// If there is more data in the buffer after the parsed message
			// then parse again. Otherwise break here and wait for more data.
			if (this->bufferDataLen > this->msgStart)
			{
				MS_DEBUG("there is more data after the parsed message, continue parsing");
				continue;
			}
			else
			{
				break;
			}
		}
	}

	void UnixStreamSocket::userOnUnixStreamSocketClosed(bool is_closed_by_peer)
	{
		MS_TRACE();

		if (is_closed_by_peer)
		{
			// Notify the listener.
			this->listener->onChannelUnixStreamSocketRemotelyClosed(this);
		}
	}
}
