#pragma once

#include "core/Assert.h"
#include "core/Func.h"
#include "core/Lock.h"
#include "core/Mutex.h"
#include "core/Queue.h"
#include "core/ThreadPool.h"

namespace core
{
	class ExecutionQueue: public SharedFromThis<ExecutionQueue>
	{
		template <typename TPtr, typename... TArgs>
		friend inline Shared<TPtr> shared_from(Allocator* allocator, TArgs&&... args);

		Queue<Func<void()>> m_queue;
		Mutex m_mutex;
		bool m_scheduled = false;

		explicit ExecutionQueue(Allocator* allocator)
			: m_queue(allocator),
			  m_mutex(allocator)
		{}

	public:
		static Shared<ExecutionQueue> create(Allocator* allocator)
		{
			return shared_from<ExecutionQueue>(allocator, allocator);
		}

		template <typename TFunc>
		void push(ThreadPool* pool, TFunc&& func)
		{
			auto lock = lockGuard(m_mutex);
			if (m_scheduled)
			{
				m_queue.push_back(std::forward<TFunc>(func));
			}
			else
			{
				m_scheduled = true;
				pool->runFromExecutionQueue(std::forward<TFunc>(func), weakFromThis());
			}
		}

		bool signalFuncExecutionFinishedAndTryPop(Func<void()>& func)
		{
			auto lock = lockGuard(m_mutex);
			validate(m_scheduled == true);
			if (m_queue.count() > 0)
			{
				func = std::move(m_queue.front());
				m_queue.pop_front();
				return true;
			}
			else
			{
				m_scheduled = false;
				return false;
			}
		}
	};
}