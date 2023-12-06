#pragma once

#include "core/Exports.h"
#include "core/Buffer.h"
#include "core/Span.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Func.h"

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

	class Connection
	{
	public:
		virtual ~Connection() = default;
		virtual HumanError writeText(StringView str) = 0;
		virtual HumanError writeText(Buffer&& str) = 0;
		virtual HumanError writeBinary(Span<const std::byte> bytes) = 0;
		virtual HumanError writeBinary(Buffer&& bytes) = 0;
		virtual HumanError writePing(Span<const std::byte> bytes) = 0;
		virtual HumanError writePing(Buffer&& bytes) = 0;
		virtual HumanError writePong(Span<const std::byte> bytes) = 0;
		virtual HumanError writePong(Buffer&& bytes) = 0;
		virtual HumanError writeClose() = 0;
		virtual HumanError writeClose(uint16_t code) = 0;
	};

	struct Handler
	{
		bool handlePing = false;
		bool handlePong = false;
		bool handleClose = false;
		size_t maxPayloadSize = 16ULL * 1024ULL;

		Func<HumanError(const Msg&, Connection*)> onMsg;
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