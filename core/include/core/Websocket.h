#pragma once

#include "core/Exports.h"
#include "core/Unique.h"

namespace core
{
	class Server
	{
	public:
		CORE_EXPORT Unique<Server> open(Allocator* allocator);

		virtual ~Server() = default;
		virtual void run() = 0;
		virtual void stop() = 0;
	};
}