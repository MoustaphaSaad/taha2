#pragma once

#include "core/EventLoop2.h"
#include "core/Array.h"
#include "core/Thread.h"
#include "core/Log.h"
#include "core/Allocator.h"
#include "core/Shared.h"

namespace core
{
	class ThreadedEventLoop2
	{
		template<typename TPtr, typename... TArgs>
		friend inline Shared<TPtr>
		shared_from(Allocator* allocator, TArgs&&... args);

		Allocator* m_allocator = nullptr;
		size_t m_threadsCount = 0;
		Array<Unique<EventLoop2>> m_eventLoops;
		std::atomic<size_t> m_nextIndex = 0;

		ThreadedEventLoop2(Allocator* allocator, size_t threadsCount)
			: m_allocator(allocator),
			  m_threadsCount(threadsCount),
			  m_eventLoops(allocator)
		{}
	public:
		static Result<Shared<ThreadedEventLoop2>> create(Log* log, Allocator* allocator, size_t threadsCount = Thread::hardware_concurrency())
		{
			auto res = shared_from<ThreadedEventLoop2>(allocator, allocator, threadsCount);
			res->m_eventLoops.reserve(threadsCount);
			for (size_t i = 0; i < threadsCount; ++i)
			{
				auto loopResult = EventLoop2::create(res.get(), log, allocator);
				if (loopResult.isError())
					return loopResult.releaseError();
				res->m_eventLoops.push(loopResult.releaseValue());
			}
			return res;
		}

		HumanError run()
		{
			constexpr int MAX_EVENT_LOOPS = 128;
			HumanError errors[MAX_EVENT_LOOPS];
			assert(m_threadsCount <= MAX_EVENT_LOOPS);

			Array<Thread> threads{m_allocator};
			threads.reserve(m_eventLoops.count());
			for (size_t i = 1; i < m_eventLoops.count(); ++i)
			{
				Thread thread{m_allocator, [this, i, &errors]{
					errors[i] = m_eventLoops[i]->run();
				}};
				threads.push(std::move(thread));
			}

			errors[0] = m_eventLoops[0]->run();

			for (auto& thread: threads)
				thread.join();

			for (size_t i = 0; i < m_eventLoops.count(); ++i)
				if (errors[i])
					return errors[i];
			return {};
		}

		void stop()
		{
			for (auto& event: m_eventLoops)
				event->stop();
		}

		EventLoop2* next()
		{
			return m_eventLoops[m_nextIndex.fetch_add(1) % m_threadsCount].get();
		}
	};
}