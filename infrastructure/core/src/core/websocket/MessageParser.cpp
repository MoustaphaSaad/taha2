#include "core/websocket/MessageParser.h"
#include "core/Intrinsics.h"

namespace core::websocket
{
	Result<size_t> FrameParser::consume(Span<const std::byte> src)
	{
		size_t offset = 0;
		while (m_state != STATE_END)
		{
			auto data = src.sliceRight(offset);

			if (m_state == STATE_PRE)
			{
				// in the pre state we should have at least 2 bytes available
				size_t minLen = 2;
				if (data.count() < minLen)
					break;

				auto byte1 = (uint8_t)data[0];

				// first figure out the opcode
				auto opcode = byte1 & 15;
				switch (opcode)
				{
				case FrameHeader::OPCODE_CONTINUATION:
				case FrameHeader::OPCODE_TEXT:
				case FrameHeader::OPCODE_BINARY:
				case FrameHeader::OPCODE_CLOSE:
				case FrameHeader::OPCODE_PING:
				case FrameHeader::OPCODE_PONG:
					m_header.opcode = (FrameHeader::OPCODE)opcode;
					break;
				default:
					return errf(m_allocator, "unsupported opcode"_sv);
				}

				m_header.isFin = (byte1 & 128) == 128;

				// check that reserved bits are not set
				if ((byte1 & 112) != 0)
					return errf(m_allocator, "reserved bits are set"_sv);

				auto byte2 = (uint8_t)data[1];
				m_header.isMasked = (byte2 & 128) == 128;
				auto length = byte2 & 127;
				switch (length)
				{
				case 126: m_payloadLengthSize = 2; break;
				case 127: m_payloadLengthSize = 8; break;
				default:
					m_payloadLengthSize = 0;
					m_header.payloadLength = length;
					break;
				}

				if (m_header.isControlOpcode() && m_payloadLengthSize != 0)
				{
					return errf(m_allocator,
								"All control frames MUST have a payload length of 125 bytes or less and MUST NOT be fragmented."_sv);
				}

				offset += 2;
				m_state = STATE_HEADER;
			}
			else if (m_state == STATE_HEADER)
			{
				auto minLen = m_payloadLengthSize;
				if (m_header.isMasked)
					minLen += 4;

				if (data.count() < minLen)
					break;

				if (m_payloadLengthSize == 2)
				{
					uint16_t payloadSize = (uint16_t)data[1] | ((uint16_t)data[0] << 8);
					if (core::systemEndianness() == core::Endianness::Little)
						m_header.payloadLength = payloadSize;
					else
						m_header.payloadLength = core::byteswap_uint16(payloadSize);
				}
				else if (m_payloadLengthSize == 8)
				{
					uint64_t payloadSize = (uint64_t)data[7] |
										   ((uint64_t)data[6] << 8) |
										   ((uint64_t)data[5] << 16) |
										   ((uint64_t)data[4] << 24) |
										   ((uint64_t)data[3] << 32) |
										   ((uint64_t)data[2] << 40) |
										   ((uint64_t)data[1] << 48) |
										   ((uint64_t)data[0] << 56);

					if (core::systemEndianness() == core::Endianness::Little)
						m_header.payloadLength = payloadSize;
					else
						m_header.payloadLength = core::byteswap_uint64(payloadSize);
				}

				if (m_header.payloadLength > m_maxPayloadSize)
				{
					return errf(m_allocator,
								"Payload size is too large, max payload size is {} bytes, but {} bytes is needed"_sv,
								m_maxPayloadSize, m_header.payloadLength);
				}

				// preserve the mask
				if (m_header.isMasked)
					::memcpy(m_mask, data.data() + m_payloadLengthSize, sizeof(m_mask));

				offset += minLen;
				m_state = STATE_PAYLOAD;
			}
			else if (m_state == STATE_PAYLOAD)
			{
				auto minLen = m_header.payloadLength;
				if (data.count() < minLen)
					break;

				auto payload = data.sliceLeft(m_header.payloadLength);

				auto initCount = m_payload.count();
				m_payload.push(payload);
				if (m_header.isMasked)
				{
					for (size_t i = 0; i < payload.count(); ++i)
						m_payload[initCount + i] ^= m_mask[i % 4];
				}
				offset += minLen;
				m_state = STATE_END;
			}
		}

		return offset;
	}

	Result<size_t> MessageParser::consume(Span<const std::byte> data)
	{
		// reset the message before working with it
		m_readyMessage = Message{m_allocator};

		auto frameResult = m_frameParser.consume(data);
		if (frameResult.isError())
			return frameResult;

		auto consumedBytes = frameResult.value();

		if (m_frameParser.hasFrame() == false)
			return consumedBytes;

		auto frame = m_frameParser.frame();

		if (m_fragmentedMessage.type != Message::TYPE_NONE &&
			frame.header.opcode != FrameHeader::OPCODE_CONTINUATION &&
			frame.header.isControlOpcode() == false)
		{
			return errf(m_allocator, "all data frames after the initial data frame must be continuation"_sv);
		}

		// single frame message, most common type of messages
		if (frame.header.isFin && frame.header.opcode != FrameHeader::OPCODE_CONTINUATION)
		{
			Message result{m_allocator};
			switch (frame.header.opcode)
			{
			case FrameHeader::OPCODE_TEXT:
				result.type = Message::TYPE_TEXT;
				break;
			case FrameHeader::OPCODE_BINARY:
				result.type = Message::TYPE_BINARY;
				break;
			case FrameHeader::OPCODE_CLOSE:
				result.type = Message::TYPE_CLOSE;
				break;
			case FrameHeader::OPCODE_PING:
				result.type = Message::TYPE_PING;
				break;
			case FrameHeader::OPCODE_PONG:
				result.type = Message::TYPE_PONG;
				break;
			default:
				coreUnreachable();
				return errf(m_allocator, "invalid opcode"_sv);
			}
			result.payload = std::move(frame.payload);
			m_readyMessage = std::move(result);
			m_frameParser = FrameParser{m_allocator, m_maxPayloadSize};
		}
		else
		{
			// first fragment
			if (m_fragmentedMessage.type == Message::TYPE_NONE)
			{
				if (frame.header.isControlOpcode())
					return errf(m_allocator, "control opcode can't be fragmented"_sv);
				else if (frame.header.opcode == FrameHeader::OPCODE_CONTINUATION)
					return errf(m_allocator, "invalid continuation frame because there is no message to continue"_sv);

				switch (frame.header.opcode)
				{
				case FrameHeader::OPCODE_TEXT:
					m_fragmentedMessage.type = Message::TYPE_TEXT;
					break;
				case FrameHeader::OPCODE_BINARY:
					m_fragmentedMessage.type = Message::TYPE_BINARY;
					break;
				default:
					coreUnreachable();
					return errf(m_allocator, "invalid opcode"_sv);
				}
			}

			m_fragmentedMessage.payload.push(frame.payload);

			if (frame.header.isFin)
			{
				m_readyMessage = std::move(m_fragmentedMessage);
				m_fragmentedMessage = Message{m_allocator};
			}
			m_frameParser = FrameParser{m_allocator, m_maxPayloadSize};
		}

		return consumedBytes;
	}
}