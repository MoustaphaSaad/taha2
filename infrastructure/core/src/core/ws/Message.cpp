#include "core/ws/Message.h"
#include "core/Intrinsics.h"

namespace core::ws
{
	static HumanError readBytes(Stream* source, Span<std::byte> bytes, Allocator* allocator)
	{
		auto readBytes = bytes;
		while (readBytes.sizeInBytes() > 0)
		{
			auto readSize = source->read(readBytes.data(), readBytes.sizeInBytes());
			if (readSize == 0)
			{
				break;
			}
			readBytes = readBytes.sliceRight(readSize);
		}

		if (readBytes.sizeInBytes() > 0)
		{
			return errf(
				allocator,
				"failed to read '{}' bytes, only read '{}' bytes"_sv,
				bytes.sizeInBytes(),
				bytes.sizeInBytes() - readBytes.sizeInBytes());
		}
		return {};
	}

	Result<Frame> Frame::read(Stream* source, size_t maxPayloadSize, Allocator* allocator)
	{
		Frame frame{allocator};

		std::byte pre[2] = {};
		auto err = readBytes(source, Span<std::byte>{pre, sizeof(pre)}, allocator);
		if (err)
		{
			return err;
		}

		frame.m_opcode = (Frame::OPCODE)(uint8_t(pre[0]) & uint8_t(0b0000'1111));
		switch (frame.m_opcode)
		{
		case Frame::OPCODE_CONTINUATION:
		case Frame::OPCODE_TEXT:
		case Frame::OPCODE_BINARY:
		case Frame::OPCODE_CLOSE:
		case Frame::OPCODE_PING:
		case Frame::OPCODE_PONG:
			// ok
			break;
		default:
			return errf(allocator, "unsupported frame opcode, {}"_sv, (int)frame.m_opcode);
		}

		frame.m_isFin = (uint8_t(pre[0]) & 0b1000'0000) == 0b1000'0000;

		if ((uint8_t(pre[0]) & 112) != 0)
		{
			return errf(allocator, "reserved bits are set"_sv);
		}

		frame.m_isMasked = (uint8_t(pre[1]) & 128) == 128;
		auto length = uint8_t(pre[1]) & 127;
		size_t payloadLengthSize = 0;
		switch (length)
		{
		case 126:
			payloadLengthSize = 2;
			break;
		case 127:
			payloadLengthSize = 8;
			break;
		default:
			payloadLengthSize = 0;
			frame.m_payloadLength = length;
			break;
		}

		if (Frame::isControlOpcode(frame.m_opcode) && (payloadLengthSize != 0 || frame.m_isFin == false))
		{
			return errf(
				allocator,
				"All control frames MUST have a payload length of 125 or less and MUST NOT be fragmented."_sv);
		}

		std::byte header[8 + 4];
		auto requiredHeaderSize = payloadLengthSize;
		if (frame.m_isMasked)
		{
			requiredHeaderSize += 4;
		}
		err = readBytes(source, Span<std::byte>{header, requiredHeaderSize}, allocator);
		if (err)
		{
			return err;
		}

		if (payloadLengthSize == 2)
		{
			auto payloadLength = *(uint16_t*)header;
			if (systemEndianness() == Endianness::Big)
			{
				frame.m_payloadLength = payloadLength;
			}
			else
			{
				frame.m_payloadLength = byteswap_uint16(payloadLength);
			}
		}
		else if (payloadLengthSize == 8)
		{
			auto payloadLength = *(uint64_t*)header;
			if (systemEndianness() == Endianness::Big)
			{
				frame.m_payloadLength = payloadLength;
			}
			else
			{
				frame.m_payloadLength = byteswap_uint64(payloadLength);
			}
		}

		if (frame.m_payloadLength > maxPayloadSize)
		{
			return errf(
				allocator,
				"Payload size is too large, max payload size is {} bytes, but {} bytes is needed"_sv,
				maxPayloadSize,
				frame.m_payloadLength);
		}

		std::byte mask[4];
		if (frame.m_isMasked)
		{
			::memcpy(mask, header + payloadLengthSize, sizeof(mask));
		}

		frame.m_payload.resize(frame.m_payloadLength);
		err = readBytes(source, Span<std::byte>{frame.m_payload}, allocator);
		if (err)
		{
			return err;
		}

		if (frame.m_isMasked)
		{
			for (size_t i = 0; i < frame.m_payload.count(); ++i)
			{
				frame.m_payload[i] = (std::byte)(((uint8_t)frame.m_payload[i]) ^ ((uint8_t)mask[i % 4]));
			}
		}

		return frame;
	}

	Result<Message> MessageParser::read(Stream* source, Allocator* allocator)
	{
		while (true)
		{
			auto remainingPayloadSize = m_maxPayloadSize - m_fragmentedMessage.payload.count();
			auto frameResult = Frame::read(source, m_maxPayloadSize, allocator);
			if (frameResult.isError())
			{
				return frameResult.releaseError();
			}
			auto frame = frameResult.releaseValue();

			if (m_fragmentedMessage.kind != Message::KIND_NONE && frame.opcode() != Frame::OPCODE_CONTINUATION &&
				Frame::isControlOpcode(frame.opcode()) == false)
			{
				return errf(allocator, "all data frames after the initial data frame must be continuation"_sv);
			}

			if (frame.isFin() && frame.opcode() != Frame::OPCODE_CONTINUATION)
			{
				Message message{allocator};
				switch (frame.opcode())
				{
				case Frame::OPCODE_TEXT:
					message.kind = Message::KIND_TEXT;
					break;
				case Frame::OPCODE_BINARY:
					message.kind = Message::KIND_BINARY;
					break;
				case Frame::OPCODE_CLOSE:
					message.kind = Message::KIND_CLOSE;
					break;
				case Frame::OPCODE_PING:
					message.kind = Message::KIND_PING;
					break;
				case Frame::OPCODE_PONG:
					message.kind = Message::KIND_PONG;
					break;
				default:
					unreachable();
					return errf(allocator, "invalid opcode"_sv);
				}

				message.payload = frame.releasePayload();
				return message;
			}
			else
			{
				if (m_fragmentedMessage.kind == Message::KIND_NONE)
				{
					if (Frame::isControlOpcode(frame.opcode()))
					{
						return errf(allocator, "control opcode can't be fragmented"_sv);
					}
					else if (frame.opcode() == Frame::OPCODE_CONTINUATION)
					{
						return errf(allocator, "invalid continuation frame because there is no message to continue"_sv);
					}

					switch (frame.opcode())
					{
					case Frame::OPCODE_TEXT:
						m_fragmentedMessage.kind = Message::KIND_TEXT;
						break;
					case Frame::OPCODE_BINARY:
						m_fragmentedMessage.kind = Message::KIND_BINARY;
						break;
					default:
						unreachable();
						return errf(allocator, "invalid opcode"_sv);
					}
				}

				m_fragmentedMessage.payload.push(frame.releasePayload());
				if (frame.isFin())
				{
					return m_fragmentedMessage;
				}
			}
		}
	}
}