#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/ThreadPool.h"
#include "core/Log.h"
#include "core/Allocator.h"

namespace core
{
	class ReactorLoop
	{
	public:
		CORE_EXPORT static Result<Unique<ReactorLoop>> create(ThreadPool* pool, Log* log, Allocator* allocator);

		virtual ~ReactorLoop() = default;
	};
}