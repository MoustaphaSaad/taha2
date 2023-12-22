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

		bool isControlOpcode() const { return (opcode & 0b1000); }
	};

	struct Frame
	{
		FrameHeader header;
		Buffer payload;
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

	struct Message
	{
		enum TYPE
		{
			TYPE_NONE,
			TYPE_TEXT,
			TYPE_BINARY,
			TYPE_CLOSE,
			TYPE_PING,
			TYPE_PONG,
		};

		TYPE type = TYPE_NONE;
		Buffer payload;

		explicit Message(Allocator* allocator)
			: payload(allocator)
		{}
	};

	struct Connection;
	class Server;

	struct Handler
	{
		bool handlePing = false;
		bool handlePong = false;
		bool handleClose = false;
		size_t maxPayloadSize = 16ULL * 1024ULL * 1024ULL;

		Func<HumanError(const Message&, Server*, Connection*)> onMsg;
	};

	class Server
	{
	public:
		static CORE_EXPORT Result<Unique<Server>> open(StringView ip, StringView port, Log* log, Allocator* allocator);

		virtual ~Server() = default;
		virtual HumanError run(Handler* handler) = 0;
		virtual void stop() = 0;
		virtual HumanError writeText(Connection* conn, StringView str) = 0;
		virtual HumanError writeText(Connection* conn, Buffer&& str) = 0;
		virtual HumanError writeBinary(Connection* conn, Span<const std::byte> bytes) = 0;
		virtual HumanError writeBinary(Connection* conn, Buffer&& bytes) = 0;
		virtual HumanError writePing(Connection* conn, Span<const std::byte> bytes) = 0;
		virtual HumanError writePing(Connection* conn, Buffer&& bytes) = 0;
		virtual HumanError writePong(Connection* conn, Span<const std::byte> bytes) = 0;
		virtual HumanError writePong(Connection* conn, Buffer&& bytes) = 0;
		virtual HumanError writeClose(Connection* conn) = 0;
		virtual HumanError writeClose(Connection* conn, uint16_t code) = 0;
	};
}