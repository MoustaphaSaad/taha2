#pragma once

#include "core/websocket/Server.h"

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