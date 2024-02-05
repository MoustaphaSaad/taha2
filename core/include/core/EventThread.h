#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Allocator.h"

namespace core
{
	class Event2
	{
	public:
		virtual ~Event2() = default;
	};

	class StartEvent: public Event2
	{};

	class EventThread
	{
	public:
		virtual ~EventThread() = default;

		virtual void handle(Event2* event) = 0;
	};

	class EventThreadPool
	{
	public:
		CORE_EXPORT static Result<Unique<EventThreadPool>> create(Log* log, Allocator* allocator);

		virtual ~EventThreadPool() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;

		virtual HumanError sendEvent(Unique<Event2> event, EventThread* thread) = 0;

		template<typename T, typename ... TArgs>
		T* startThread(TArgs&& ... args)
		{
			auto thread = unique_from<T>(allocator(), std::forward<TArgs>(args)...);
			auto res = thread.get();
			addThread(std::move(thread));
			return res;
		}
	private:
		virtual void addThread(Unique<EventThread> thread) = 0;

		virtual Allocator* allocator() = 0;
	};
}