#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Socket.h"

namespace core
{
	class EventSource;

	class EventLoop
	{
	public:
		CORE_EXPORT static Result<Unique<EventLoop>> create(Log* log, Allocator* allocator);

		virtual ~EventLoop() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;

		virtual Unique<EventSource> createEventSource(Unique<Socket> socket) = 0;
		virtual HumanError read(EventSource* source) = 0;
	};
}