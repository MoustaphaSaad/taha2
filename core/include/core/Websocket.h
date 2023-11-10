#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"

namespace core
{
	class Server
	{
	public:
		static CORE_EXPORT Unique<Server> open(Allocator* allocator);

		virtual ~Server() = default;
		virtual HumanError run() = 0;
		virtual void stop() = 0;
	};
}