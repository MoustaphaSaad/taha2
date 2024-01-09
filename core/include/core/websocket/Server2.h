#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/EventLoop.h"

namespace core::websocket
{
	struct ServerConfig2
	{
		StringView host = "127.0.0.1"_sv;
		StringView port = "8080"_sv;
	};

	class Server2
	{
	public:
		static CORE_EXPORT Result<Unique<Server2>> create(Log* log, Allocator* allocator);

		virtual ~Server2() = default;
		virtual HumanError start(ServerConfig2 config, EventLoop* loop) = 0;
	};
}