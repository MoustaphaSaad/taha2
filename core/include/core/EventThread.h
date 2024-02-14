#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/Log.h"
#include "core/Allocator.h"
#include "core/ExecutionQueue.h"
#include "core/Socket.h"

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

	class EventThread: public SharedFromThis<EventThread>
	{
		EventThreadPool* m_eventThreadPool = nullptr;
		core::Shared<ExecutionQueue> m_queue;
	public:
		EventThread(EventThreadPool* pool);
		virtual ~EventThread() = default;

		virtual void handle(Event2* event) = 0;

		EventThreadPool* eventThreadPool() const { return m_eventThreadPool; }
		ExecutionQueue* executionQueue() const { return m_queue.get(); }
		HumanError sendEvent(Unique<Event2> event);
	};

	class EventThreadPool
	{
		friend class EventThread;
	protected:
		Allocator* m_allocator = nullptr;

		virtual HumanError sendEvent(Unique<Event2> event, EventThread* thread) = 0;

	public:
		CORE_EXPORT static Result<Unique<EventThreadPool>> create(ThreadPool* threadPool, Log* log, Allocator* allocator);

		explicit EventThreadPool(Allocator* allocator)
			: m_allocator(allocator)
		{}

		virtual ~EventThreadPool() = default;

		virtual HumanError run() = 0;
		virtual void stop() = 0;
		virtual HumanError registerSocket(const Unique<Socket>& socket) = 0;

		template<typename T, typename ... TArgs>
		Shared<T> startThread(TArgs&& ... args)
		{
			auto thread = shared_from<T>(m_allocator, std::forward<TArgs>(args)...);
			addThread(thread);
			return thread;
		}
	private:
		virtual void addThread(const Shared<EventThread>& thread) = 0;
	};

	EventThread::EventThread(EventThreadPool* pool)
		: m_eventThreadPool(pool)
	{
		m_queue = ExecutionQueue::create(pool->m_allocator);
	}

	HumanError EventThread::sendEvent(Unique<core::Event2> event)
	{
		return m_eventThreadPool->sendEvent(std::move(event), this);
	}
}