#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/Array.h"
#include "core/Thread.h"
#include "core/NotificationQueue.h"

#include <atomic>

namespace core
{
	class ThreadPool
	{
		size_t m_threads_count = 0;
		Array<Thread> m_threads;
		Array<NotificationQueue> m_queue;
		std::atomic<size_t> m_next_queue = 0;
	public:
		CORE_EXPORT ThreadPool(Allocator* allocator, size_t threads_count = Thread::hardware_concurrency());
		CORE_EXPORT ThreadPool(ThreadPool&& other) = default;
		CORE_EXPORT ThreadPool& operator=(ThreadPool&& other) = default;
		CORE_EXPORT ~ThreadPool();

		template<typename TFunc>
		void run(TFunc&& func)
		{
			constexpr size_t K = 4;
			auto n = m_next_queue.fetch_add(1);
			for (size_t i = 0; i < m_threads_count * K; ++i)
				if (m_queue[(i + n) % m_threads_count].try_push(std::forward<TFunc>(func)))
					return;
			m_queue[n % m_threads_count].push(std::forward<TFunc>(func));
		}
	};
}
