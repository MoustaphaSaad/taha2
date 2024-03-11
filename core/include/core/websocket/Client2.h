#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/EventLoop2.h"

namespace core::websocket
{
	struct Client2Config
	{
		StringView url = "127.0.0.1:8080"_sv;
		size_t maxMessageSize = 64ULL * 1024ULL * 1024ULL;
	};

	class Client2
	{
	public:
		CORE_EXPORT static Result<Unique<Client2>> connect(const Client2Config& config, EventLoop2* loop, Log* log, Allocator* allocator);

		virtual ~Client2() = default;
	};
}