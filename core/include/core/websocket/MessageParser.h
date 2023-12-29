#pragma once

#include "core/websocket/Server.h"

namespace core::websocket
{
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

		bool isControlOpcode() const { return (opcode & 0b1000); }
	};

	constexpr static uint8_t EMPTY_PONG[] = {128 | FrameHeader::OPCODE_PONG, 0x00};
	// code: 1000
	constexpr static uint8_t CLOSE_NORMAL[] = {128 | FrameHeader::OPCODE_CLOSE, 0x02, 0x03, 0xE8};
	// code: 1002
	constexpr static uint8_t CLOSE_PROTOCOL_ERROR[] = {128 | FrameHeader::OPCODE_CLOSE, 0x02, 0x03, 0xEA};

	struct Frame
	{
		FrameHeader header;
		Buffer payload;
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
		FrameHeader m_header;
		size_t m_payloadLengthSize = 0;
		std::byte m_mask[4];
		Buffer m_payload;
		size_t m_maxPayloadSize = 0;
		Allocator* m_allocator;

	public:
		explicit FrameParser(Allocator* allocator, size_t maxPayloadSize)
			: m_allocator(allocator),
			  m_payload(allocator),
			  m_maxPayloadSize(maxPayloadSize)
		{}

		bool hasFrame() const { return m_state == STATE_END; }

		Frame frame()
		{
			assert(m_state == STATE_END);
			return Frame{.header = m_header, .payload = std::move(m_payload)};
		}

		CORE_EXPORT Result<size_t> consume(Span<const std::byte> data);
	};

	class MessageParser
	{
		Allocator* m_allocator = nullptr;
		size_t m_maxPayloadSize = 0;
		FrameParser m_frameParser;
		Message m_readyMessage;
		Message m_fragmentedMessage;
	public:
		explicit MessageParser(Allocator* allocator, size_t maxPayloadSize)
			: m_allocator(allocator),
			  m_maxPayloadSize(maxPayloadSize),
			  m_frameParser(allocator, maxPayloadSize),
			  m_readyMessage(allocator),
			  m_fragmentedMessage(allocator)
		{}

		CORE_EXPORT Result<size_t> consume(Span<const std::byte> data);

		bool hasMessage() const { return m_readyMessage.type != Message::TYPE_NONE; }

		Message message()
		{
			return std::move(m_readyMessage);
		}
	};
}