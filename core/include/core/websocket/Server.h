#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/EventLoop.h"
#include "core/Func.h"
#include "core/websocket/Message.h"

namespace core::websocket
{
	struct ServerConfig
	{
		StringView host = "127.0.0.1"_sv;
		StringView port = "8080"_sv;
		size_t maxHandshakeSize = 1ULL * 1024ULL;
	};

	class Server;
	class Conn;

	struct ServerHandler
	{
		bool handlePing = false;
		bool handlePong = false;
		bool handleClose = false;
		size_t maxPayloadSize = 16ULL * 1024ULL * 1024ULL;

		Func<HumanError(const Message&, Server*, Conn*)> onMsg;
	};

	class Server
	{
	public:
		static CORE_EXPORT Result<Unique<Server>> create(Log* log, Allocator* allocator);

		virtual ~Server() = default;
		virtual HumanError start(ServerConfig config, EventLoop* loop, ServerHandler* handler) = 0;
		virtual void stop() = 0;
		virtual HumanError writeText(Conn* conn, StringView str) = 0;
		virtual HumanError writeBinary(Conn* conn, Span<const std::byte> bytes) = 0;
		virtual HumanError writePing(Conn* conn, Span<const std::byte> bytes) = 0;
		virtual HumanError writePong(Conn* conn, Span<const std::byte> bytes) = 0;
		virtual HumanError writeClose(Conn* conn) = 0;
		virtual HumanError writeClose(Conn* conn, uint16_t code, StringView optionalReason) = 0;
	};
}