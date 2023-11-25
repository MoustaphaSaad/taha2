#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"
#include "core/Log.h"
#include "core/Hash.h"
#include "core/Func.h"

namespace core::websocket
{
	class Handshake
	{
		String m_key;
	public:
		Handshake(Allocator* allocator)
			: m_key(allocator)
		{}

		Handshake(String key)
			: m_key(key)
		{}

		StringView key() const { return m_key; }

		static Result<Handshake> parse(StringView request, Allocator* allocator)
		{
			size_t findIndex = 0;
			auto lineEnd = request.find(Rune{'\r'}, findIndex);
			if (lineEnd == SIZE_MAX) return errf(allocator, "cannot find request line end"_sv);
			auto requestLine = request.slice(findIndex, lineEnd);

			if (requestLine.endsWithIgnoreCase("http/1.1"_sv) == false)
				return errf(allocator, "unsupported http version"_sv);

			int requiredHeaders = 0;
			StringView key;

			findIndex = lineEnd + 2;
			while (request.count() - findIndex > 4)
			{
				lineEnd = request.find(Rune{'\r'}, findIndex);
				if (lineEnd == SIZE_MAX) return errf(allocator, "cannot find header line end"_sv);
				auto headerLine = request.slice(findIndex, lineEnd);

				auto separatorIndex = headerLine.find(Rune{':'});
				if (separatorIndex == SIZE_MAX) return errf(allocator, "cannot find header separator"_sv);

				auto headerName = headerLine.slice(0, separatorIndex).trim();
				auto headerValue = headerLine.slice(separatorIndex + 1, headerLine.count()).trim();

				if (headerName.equalsIgnoreCase("upgrade"_sv))
				{
					if (headerValue.equalsIgnoreCase("websocket"_sv) == false)
						return errf(allocator, "unsupported upgrade value"_sv);
					++requiredHeaders;
				}
				else if (headerName.equalsIgnoreCase("sec-websocket-version"_sv))
				{
					if (headerValue.equalsIgnoreCase("13"_sv) == false)
						return errf(allocator, "unsupported websocket version"_sv);
					++requiredHeaders;
				}
				else if (headerName.equalsIgnoreCase("connection"_sv))
				{
					if (headerValue.findIgnoreCase("upgrade"_sv) == SIZE_MAX)
						return errf(allocator, "unsupported connection value"_sv);
					++requiredHeaders;
				}
				else if (headerName.equalsIgnoreCase("sec-websocket-key"_sv))
				{
					key = headerValue;
					++requiredHeaders;
				}

				findIndex = lineEnd + 2;
			}

			if (requiredHeaders != 4)
				return errf(allocator, "missing required headers"_sv);

			return Handshake{String{key, allocator}};
		}
	};

	struct FrameHeader
	{
		enum OPCODE: uint8_t
		{
			OPCODE_CONTINUATION = 0,
			OPCODE_TEXT = 1,
			OPCODE_BINARY = 2,
			OPCODE_CLOSE = 8,
			OPCODE_PING = 9,
			OPCODE_PONG = 10,
		};

		OPCODE opcode = OPCODE_CONTINUATION;
		bool isMasked = false;
		bool isFin = false;
		uint64_t payloadLength = 0;
	};

	struct Frame
	{
		FrameHeader header;
		Buffer payload;

		explicit Frame(Allocator* allocator)
			: payload(allocator)
		{}
	};

	struct Msg
	{
		enum TYPE
		{
			TYPE_TEXT,
			TYPE_BINARY,
			TYPE_CLOSE,
			TYPE_PING,
			TYPE_PONG,
		};

		TYPE type;
		Span<const std::byte> payload;
	};

	class FrameParser
	{
		enum STATE
		{
			STATE_PRE,
			STATE_HEADER,
			STATE_PAYLOAD,
			STATE_END,
		};

		STATE m_state = STATE_PRE;
		size_t m_neededBytes = 2;
		size_t m_payloadLengthSize = 0;
		size_t m_payloadOffset = 0;
		Span<const std::byte> m_mask;
		FrameHeader m_header;
		Buffer m_payload;
		bool m_isFragmented = false;
		Allocator* m_allocator = nullptr;

		void pushPayload(Span<const std::byte> payload)
		{
			auto initCount = m_payload.count();
			m_payload.resize(initCount + payload.count());
			for (size_t i = 0; i < payload.count(); ++i)
				m_payload[initCount + i] = payload[i] ^ m_mask[i % 4];
		}

		Result<bool> step(const std::byte* ptr, size_t size)
		{
			// if we found the end of the frame, we are done
			if (m_state == STATE_END)
				return true;

			// wait for more data to be received
			if (size < m_neededBytes)
				return false;

			switch (m_state)
			{
			case STATE_PRE:
			{
				auto byte1 = (uint8_t)ptr[0];
				auto byte2 = (uint8_t)ptr[1];

				// check masked
				m_header.isMasked = (byte2 & 128) == 128;

				// get payload length size
				if ((byte2 & 127) == 126)
					m_payloadLengthSize = 2;
				else if ((byte2 & 127) == 127)
					m_payloadLengthSize = 8;
				else
					m_payloadLengthSize = 0;

				m_state = STATE_HEADER;
				m_neededBytes += m_payloadLengthSize;
				m_mask = Span<const std::byte>{ptr + m_neededBytes, 4};
				if (m_header.isMasked)
					m_neededBytes += 4;
				m_payloadOffset = m_neededBytes;

				auto opcode = byte1 & 15;
				if (opcode == FrameHeader::OPCODE_CONTINUATION)
				{
					m_header.opcode = FrameHeader::OPCODE_CONTINUATION;
				}
				else if (opcode == FrameHeader::OPCODE_TEXT)
				{
					m_header.opcode = FrameHeader::OPCODE_TEXT;
				}
				else if (opcode == FrameHeader::OPCODE_BINARY)
				{
					m_header.opcode = FrameHeader::OPCODE_BINARY;
				}
				else if (opcode == FrameHeader::OPCODE_CLOSE)
				{
					m_header.opcode = FrameHeader::OPCODE_CLOSE;
				}
				else if (opcode == FrameHeader::OPCODE_PING)
				{
					m_header.opcode = FrameHeader::OPCODE_PING;
				}
				else if (opcode == FrameHeader::OPCODE_PONG)
				{
					m_header.opcode = FrameHeader::OPCODE_PONG;
				}
				else
				{
					return errf(m_allocator, "unsupported opcode"_sv);
				}

				// check that reserved bits are not set
				if ((byte1 & 12) != 0)
					return errf(m_allocator, "reserved bits are set"_sv);

				if (m_header.opcode != FrameHeader::OPCODE_CONTINUATION &&
					m_payloadLengthSize != 0 &&
					(m_header.opcode == FrameHeader::OPCODE_PING ||
					 m_header.opcode == FrameHeader::OPCODE_PONG ||
					 m_header.opcode == FrameHeader::OPCODE_CLOSE))
				{
					return errf(m_allocator, "control frame cannot have payload length"_sv);
				}
				return false;
			}
			case STATE_HEADER:
			{
				if (m_payloadLengthSize == 2)
				{
					m_header.payloadLength = (uint16_t)ptr[3] | ((uint16_t)ptr[2] << 8);
				}
				else if (m_payloadLengthSize == 8)
				{
					m_header.payloadLength = (uint64_t)ptr[9] |
						((uint64_t)ptr[8] << 8) |
						((uint64_t)ptr[7] << 16) |
						((uint64_t)ptr[6] << 24) |
						((uint64_t)ptr[5] << 32) |
						((uint64_t)ptr[4] << 40) |
						((uint64_t)ptr[3] << 48) |
						((uint64_t)ptr[2] << 56);
				}
				else
				{
					m_header.payloadLength = (uint8_t)ptr[1] & 127;
				}

				m_neededBytes += m_header.payloadLength;
				m_state = STATE_PAYLOAD;
				return false;
			}
			case STATE_PAYLOAD:
			{
				m_header.isFin = ((uint8_t)ptr[0] & 128) == 128;
				Span<const std::byte> payload{ptr + m_payloadOffset, m_header.payloadLength};

				if (m_header.isFin)
				{
					if (m_header.opcode == FrameHeader::OPCODE_CONTINUATION)
					{
						if (m_isFragmented == false)
							return errf(m_allocator, "continuation frame without previous frames"_sv);
					}

					if (m_isFragmented && (m_header.opcode == FrameHeader::OPCODE_TEXT || m_header.opcode == FrameHeader::OPCODE_BINARY))
					{
						return errf(m_allocator, "nested fragments"_sv);
					}

					pushPayload(payload);
					m_state = STATE_END;
					return true;
				}

				if (m_header.opcode == FrameHeader::OPCODE_CONTINUATION)
				{
					if (m_isFragmented)
					{
						pushPayload(payload);
						return false;
					}
					else
					{
						return errf(m_allocator, "continuation frame without previous frames"_sv);
					}
				}
				else if (m_header.opcode != FrameHeader::OPCODE_TEXT && m_header.opcode != FrameHeader::OPCODE_BINARY)
				{
					return errf(m_allocator, "fragmented control messages"_sv);
				}

				if (m_isFragmented)
					return errf(m_allocator, "nested fragmented messages"_sv);

				m_isFragmented = true;
				return false;
			}
			default:
			{
				assert(false);
				return errf(m_allocator, "invalid state"_sv);
			}
			}
		}

	public:
		explicit FrameParser(Allocator* allocator)
			: m_payload(allocator),
			  m_allocator(allocator)
		{}

		size_t neededBytes() const { return m_neededBytes; }

		Result<bool> consume(const std::byte* ptr, size_t size)
		{
			while (size >= m_neededBytes)
			{
				auto result = step(ptr, m_neededBytes);
				if (result.isError())
					return result;

				if (result.value())
					return true;
			}
			return false;
		}

		Msg msg() const
		{
			assert(m_state == STATE_END);

			Msg result{};
			switch (m_header.opcode)
			{
			case FrameHeader::OPCODE_TEXT:
				result.type = Msg::TYPE_TEXT;
				break;
			case FrameHeader::OPCODE_BINARY:
				result.type = Msg::TYPE_BINARY;
				break;
			case FrameHeader::OPCODE_CLOSE:
				result.type = Msg::TYPE_CLOSE;
				break;
			case FrameHeader::OPCODE_PING:
				result.type = Msg::TYPE_PING;
				break;
			case FrameHeader::OPCODE_PONG:
				result.type = Msg::TYPE_PONG;
				break;
			case FrameHeader::OPCODE_CONTINUATION:
			default:
				assert(false);
				return result;
			}
			result.payload = Span<const std::byte>{m_payload};
			return result;
		}
	};

	class Connection
	{
	public:
		virtual ~Connection() = default;
		virtual HumanError write(Buffer&& bytes) = 0;
		virtual HumanError write(StringView str) = 0;
		virtual HumanError write(Span<const std::byte> bytes) = 0;
	};

	struct Handler
	{
		Func<void(const Msg&, Connection*)> onMsg;
	};

	class Server
	{
	public:
		static CORE_EXPORT Result<Unique<Server>> open(StringView ip, StringView port, Log* log, Allocator* allocator);

		virtual ~Server() = default;
		virtual HumanError run(Handler* handler) = 0;
		virtual void stop() = 0;
	};
}
