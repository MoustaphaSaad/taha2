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

	class EventThreadPool;

	class EventThread
	{
		EventThreadPool* m_eventThreadPool = nullptr;
	public:
		EventThread(EventThreadPool* pool)
			: m_eventThreadPool(pool)
		{}

		virtual ~EventThread() = default;

		virtual void handle(Event2* event) = 0;

		EventThreadPool* eventThreadPool() const { return m_eventThreadPool; }
	};

	class EventThreadPool
	{
	protected:
		Allocator* m_allocator = nullptr;

	public:
		CORE_EXPORT static Result<Unique<EventThreadPool>> create(Log* log, Allocator* allocator);

		explicit EventThreadPool(Allocator* allocator)
			: m_allocator(allocator)
		{}

		virtual ~EventThreadPool() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;

		virtual HumanError sendEvent(Unique<Event2> event, EventThread* thread) = 0;

		template<typename T, typename ... TArgs>
		T* startThread(TArgs&& ... args)
		{
			auto thread = unique_from<T>(m_allocator, std::forward<TArgs>(args)...);
			auto res = thread.get();
			addThread(std::move(thread));
			return res;
		}
	private:
		virtual void addThread(Unique<EventThread> thread) = 0;
	};
}