#pragma once

#include "core/websocket/Websocket.h"

namespace core::websocket
{
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
				if ((byte1 & 112) != 0)
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
}