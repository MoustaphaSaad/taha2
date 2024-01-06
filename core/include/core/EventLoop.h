#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"

namespace core
{
	class EventLoop
	{
	public:
		CORE_EXPORT static Result<Unique<EventLoop>> create(Allocator* allocator);

		virtual ~EventLoop() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;
	};
}