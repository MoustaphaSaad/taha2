#pragma once

#include "core/Exports.h"
#include "core/Buffer.h"
#include "core/Span.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Func.h"
#include "core/websocket/Message.h"

namespace core::websocket
{
	struct Connection;
	class Server;

	struct ServerHandler
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
		virtual HumanError run(ServerHandler* handler) = 0;
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