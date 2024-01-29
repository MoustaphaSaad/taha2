#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/Array.h"
#include "core/Thread.h"
#include "core/NotificationQueue.h"
#include "core/WaitGroup.h"

#include <atomic>

namespace core
{
	class ExecutionQueue;

	class ThreadPool
	{
		size_t m_threads_count = 0;
		Array<Thread> m_threads;
		Array<NotificationQueue> m_queue;
		WaitGroup m_wait_group;
		std::atomic<size_t> m_next_queue = 0;

		template<typename TFunc>
		void pushFunc(TFunc&& func, const Weak<ExecutionQueue>& execQueue)
		{
			m_wait_group.add(1);

			constexpr size_t K = 4;
			auto n = m_next_queue.fetch_add(1);
			for (size_t i = 0; i < m_threads_count * K; ++i)
				if (m_queue[(i + n) % m_threads_count].tryPush(std::forward<TFunc>(func), execQueue))
					return;
			m_queue[n % m_threads_count].push(std::forward<TFunc>(func), execQueue);
		}

	public:
		CORE_EXPORT ThreadPool(Allocator* allocator, size_t threads_count = Thread::hardware_concurrency());
		CORE_EXPORT ThreadPool(ThreadPool&& other) = default;
		CORE_EXPORT ThreadPool& operator=(ThreadPool&& other) = default;
		CORE_EXPORT ~ThreadPool();

		template<typename TFunc>
		void run(TFunc&& func)
		{
			pushFunc(std::forward<TFunc>(func), nullptr);
		}

		template<typename TFunc>
		void runFromExecutionQueue(TFunc&& func, const Weak<ExecutionQueue>& execQueue)
		{
			pushFunc(std::forward<TFunc>(func), execQueue);
		}

		void flush()
		{
			m_wait_group.wait();
		}
	};
}
