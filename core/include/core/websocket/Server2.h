#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/EventLoop.h"
#include "core/Func.h"
#include "core/websocket/Message.h"

namespace core::websocket
{
	struct ServerConfig2
	{
		StringView host = "127.0.0.1"_sv;
		StringView port = "8080"_sv;
		size_t maxHandshakeSize = 1ULL * 1024ULL;
	};

	class Server2;
	class Conn;

	struct ServerHandler2
	{
		bool handlePing = false;
		bool handlePong = false;
		bool handleClose = false;
		size_t maxPayloadSize = 16ULL * 1024ULL * 1024ULL;

		Func<HumanError(const Message&)> onMsg;
	};

	class Server2
	{
	public:
		static CORE_EXPORT Result<Unique<Server2>> create(Log* log, Allocator* allocator);

		virtual ~Server2() = default;
		virtual HumanError start(ServerConfig2 config, EventLoop* loop, ServerHandler2* handler) = 0;
	};
}