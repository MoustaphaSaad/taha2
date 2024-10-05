#include "core/ThreadPool.h"
#include "core/ExecutionQueue.h"

namespace core
{
	ThreadPool::ThreadPool(Allocator* allocator, size_t threads_count)
		: m_threads_count(threads_count),
		  m_threads(allocator),
		  m_queue(allocator),
		  m_wait_group(allocator)
	{
		m_queue.reserve(threads_count);
		for (size_t i = 0; i < threads_count; ++i)
		{
			m_queue.push(NotificationQueue{allocator});
		}

		m_threads.reserve(threads_count);
		for (size_t i = 0; i < threads_count; ++i)
		{
			Thread thread(allocator, [this, n = i]() {
				while (true)
				{
					NotificationQueueEntry entry;
					for (size_t i = 0; i < m_threads_count; ++i)
					{
						if (m_queue[(i + n) % m_threads_count].tryPop(entry))
						{
							break;
						}
					}
					if (!entry.func && !m_queue[n].pop(entry))
					{
						break;
					}
					entry.func();
					if (auto executionQueue = entry.executionQueue.lock())
					{
						Func<void()> nextFunc;
						if (executionQueue->signalFuncExecutionFinishedAndTryPop(nextFunc))
						{
							this->runFromExecutionQueue(std::move(nextFunc), executionQueue);
						}
					}
					m_wait_group.done();
				}
			});
			m_threads.push(std::move(thread));
		}
	}

	ThreadPool::~ThreadPool()
	{
		for (auto& queue: m_queue)
		{
			queue.done();
		}
		for (auto& thread: m_threads)
		{
			thread.join();
		}
	}
}