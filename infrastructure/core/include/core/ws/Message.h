#pragma once

#include "core/Exports.h"
#include "core/Buffer.h"
#include "core/Result.h"
#include "core/Stream.h"

namespace core::ws
{
	class Frame
	{
	public:
		enum OPCODE
		{
			OPCODE_CONTINUATION = 0,
			OPCODE_TEXT = 1,
			OPCODE_BINARY = 2,
			OPCODE_CLOSE = 8,
			OPCODE_PING = 9,
			OPCODE_PONG = 10,
		};

		CORE_EXPORT static Result<Frame> read(Stream* source, size_t maxPayloadSize, Allocator* allocator);

		static bool isControlOpcode(OPCODE op)
		{
			return (op & 0b1000);
		}

		OPCODE opcode() const { return m_opcode; }
		bool isFin() const { return m_isFin; }
		Buffer releasePayload() { return std::move(m_payload); }
	private:
		explicit Frame(Allocator* allocator)
			: m_payload(allocator)
		{}

		OPCODE m_opcode = OPCODE_CONTINUATION;
		bool m_isMasked = false;
		bool m_isFin = false;
		size_t m_payloadLength = 0;
		Buffer m_payload;
	};

	struct Message
	{
		enum KIND
		{
			KIND_NONE,
			KIND_TEXT,
			KIND_BINARY,
			KIND_CLOSE,
			KIND_PING,
			KIND_PONG,
		};

		KIND kind;
		Buffer payload;

		explicit Message(Allocator* allocator)
			: payload(allocator)
		{}
	};

	class MessageParser
	{
		size_t m_maxPayloadSize = 0;
		Message m_fragmentedMessage;
	public:
		MessageParser(size_t maxPayloadSize, Allocator* allocator)
			: m_maxPayloadSize(maxPayloadSize),
			  m_fragmentedMessage(allocator)
		{}

		CORE_EXPORT Result<Message> read(Stream* source, Allocator* allocator);
	};
}