#include "core/ThreadPool.h"

namespace core
{
	ThreadPool::ThreadPool(Allocator* allocator, size_t threads_count)
		: m_threads_count(threads_count),
		  m_threads(allocator),
		  m_queue(allocator)
	{
		m_queue.reserve(threads_count);
		for (size_t i = 0; i < threads_count; ++i)
			m_queue.push(NotificationQueue{allocator});

		m_threads.reserve(threads_count);
		for (size_t i = 0; i < threads_count; ++i)
		{
			Thread thread(allocator, [this, n = i]()
			{
				while (true)
				{
					Func<void()> func;
					for (size_t i = 0; i < m_threads_count; ++i)
						if (m_queue[(i + n) % m_threads_count].try_pop(func))
							break;
					if (!func && !m_queue[n].pop(func))
						break;
					func();
				}
			});
			m_threads.push(std::move(thread));
		}
	}

	ThreadPool::~ThreadPool()
	{
		for (auto& queue : m_queue)
			queue.done();
		for (auto& thread : m_threads)
			thread.join();
	}
}