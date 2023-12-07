#include "core/websocket/MessageParser.h"

namespace core::websocket
{
	Result<bool> FrameParser2::step(Span<const std::byte> data)
	{
		// if we found the end of the frame, we are done
		if (m_state == STATE_END)
			return true;

		// wait for more data to be received
		if (data.sizeInBytes() < m_neededBytes)
			return false;

		switch (m_state)
		{
		case STATE_PRE:
		{
			auto byte1 = (uint8_t)data[0];
			auto byte2 = (uint8_t)data[1];

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
			m_maskOffset = m_neededBytes;
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
				m_header.payloadLength = (uint16_t)data[3] | ((uint16_t)data[2] << 8);
			}
			else if (m_payloadLengthSize == 8)
			{
				m_header.payloadLength = (uint64_t)data[9] |
										 ((uint64_t)data[8] << 8) |
										 ((uint64_t)data[7] << 16) |
										 ((uint64_t)data[6] << 24) |
										 ((uint64_t)data[5] << 32) |
										 ((uint64_t)data[4] << 40) |
										 ((uint64_t)data[3] << 48) |
										 ((uint64_t)data[2] << 56);
			}
			else
			{
				m_header.payloadLength = (uint8_t)data[1] & 127;
			}

			m_neededBytes += m_header.payloadLength;
			m_state = STATE_PAYLOAD;
			return false;
		}
		case STATE_PAYLOAD:
		{
			m_header.isFin = ((uint8_t)data[0] & 128) == 128;
			auto mask = data.slice(m_maskOffset, 4);
			auto payload = data.slice(m_payloadOffset, m_header.payloadLength);

			auto initCount = m_payload.count();
			m_payload.resize(initCount + payload.count());
			for (size_t i = 0; i < payload.count(); ++i)
				m_payload[initCount + i] = payload[i] ^ mask[i % 4];

			m_state = STATE_END;
			return true;
		}
		default:
		{
			assert(false);
			return errf(m_allocator, "invalid state"_sv);
		}
		}
	}

	Result<bool> FrameParser2::consume(Span<const std::byte> data)
	{
		while (data.sizeInBytes() >= m_neededBytes)
		{
			auto result = step(data);
			if (result.isError())
				return result;

			if (result.value())
				return true;
		}
		return false;
	}

	Frame FrameParser2::frame() const
	{
		assert(m_state == STATE_END);

		Frame result{m_allocator};
		result.header = m_header;
		result.payload = std::move(m_payload);
		return result;
	}

	Result<bool> MessageParser::consume(Span<const std::byte> data)
	{
		auto frameResult = m_frameParser.consume(data);
		if (frameResult.isError())
			return frameResult;

		if (frameResult.value() == false)
			return false;

		auto frame = m_frameParser.frame();

		if (m_isFirstFrame)
		{
			switch (frame.header.opcode)
			{
			case FrameHeader::OPCODE_TEXT:
				m_message.type = Message::TYPE_TEXT;
				break;
			case FrameHeader::OPCODE_BINARY:
				m_message.type = Message::TYPE_BINARY;
				break;
			case FrameHeader::OPCODE_CLOSE:
				m_message.type = Message::TYPE_CLOSE;
				break;
			case FrameHeader::OPCODE_PING:
				m_message.type = Message::TYPE_PING;
				break;
			case FrameHeader::OPCODE_PONG:
				m_message.type = Message::TYPE_PONG;
				break;
			case FrameHeader::OPCODE_CONTINUATION:
				return errf(m_allocator, "continuation frame without previous frames"_sv);
			default:
				assert(false);
				return errf(m_allocator, "invalid opcode"_sv);
			}

			m_message.payload.push(frame.payload);

			m_isFirstFrame = false;
		}
		else if (m_isFragmented)
		{
			if (frame.header.opcode != FrameHeader::OPCODE_CONTINUATION)
				return errf(m_allocator, "nested message frames"_sv);

			m_message.payload.push(frame.payload);
		}
		else
		{
			assert(false && "unknown state");
			return errf(m_allocator, "unknown state, frame is not the first one and yet it is not a fragment as well"_sv);
		}

		m_isFin = frame.header.isFin;
		m_consumedBytes += m_frameParser.neededBytes();
		m_frameParser = FrameParser2{m_allocator};

		if (m_isFin)
		{
			return true;
		}
		else
		{
			m_isFragmented = true;
			return false;
		}
	}
}