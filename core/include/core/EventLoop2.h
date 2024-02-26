#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"

namespace core
{
	class EventLoop2
	{
	public:
		CORE_EXPORT static Result<Unique<EventLoop2>> create(Log* log, Allocator* allocator);

		virtual ~EventLoop2() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;
	};
}