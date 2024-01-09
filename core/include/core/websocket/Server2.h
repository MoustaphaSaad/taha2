#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Log.h"

namespace core::websocket
{
	class Server2
	{
	public:
		static CORE_EXPORT Result<Unique<Server2>> create(Log* log, Allocator* allocator);

		virtual ~Server2() = default;
	};
}