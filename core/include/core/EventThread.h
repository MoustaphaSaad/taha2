#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Allocator.h"

namespace core
{
	class EventThreadPool
	{
	public:
		CORE_EXPORT static Result<Unique<EventThreadPool>> create(Log* log, Allocator* allocator);
	};
}