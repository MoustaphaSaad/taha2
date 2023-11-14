#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"
#include "core/Log.h"

namespace core
{
	class WebSocketServer
	{
	public:
		static CORE_EXPORT Result<Unique<WebSocketServer>> open(StringView ip, StringView port, Log* log, Allocator* allocator);

		virtual ~WebSocketServer() = default;
		virtual HumanError run() = 0;
		virtual void stop() = 0;
	};
}