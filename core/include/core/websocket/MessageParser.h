#pragma once

#include "core/websocket/Websocket.h"

namespace core::websocket
{
	class FrameParser3
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
		Buffer& m_payload;
		Allocator* m_allocator;

		bool isControlOpcode(FrameHeader::OPCODE opcode);

	public:
		explicit FrameParser3(Allocator* allocator, Buffer& payload)
			: m_allocator(allocator),
			  m_payload(payload)
		{}

		FrameHeader frameHeader() const { assert(m_state == STATE_END); return m_header; }

		CORE_EXPORT Result<size_t> consume(Span<const std::byte> data);
	};

	class FrameParser2
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
		size_t m_maskOffset = 0;
		size_t m_payloadOffset = 0;
		FrameHeader m_header;
		Buffer m_payload;
		Allocator* m_allocator = nullptr;

		Result<bool> step(Span<const std::byte> data);

	public:
		explicit FrameParser2(Allocator* allocator)
			: m_payload(allocator),
			  m_allocator(allocator)
		{}

		size_t neededBytes() const { return m_neededBytes; }

		Result<bool> consume(Span<const std::byte> data);

		Frame frame() const;
	};

	class MessageParser
	{
		Allocator* m_allocator = nullptr;
		FrameParser2 m_frameParser;
		Message m_readyMessage;
		Message m_fragmentedMessage;
		size_t m_consumedBytes = 0;
	public:
		explicit MessageParser(Allocator* allocator)
			: m_allocator(allocator),
			  m_frameParser(allocator),
			  m_readyMessage(allocator),
			  m_fragmentedMessage(allocator)
		{}

		size_t neededBytes() const { return m_frameParser.neededBytes(); }
		size_t consumedBytes() const { return m_consumedBytes; }

		CORE_EXPORT Result<bool> consume(Span<const std::byte> data);

		Message message() const
		{
			return m_readyMessage;
		}
	};
}