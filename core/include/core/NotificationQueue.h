#pragma once

#include "core/Queue.h"
#include "core/Func.h"
#include "core/Mutex.h"
#include "core/ConditionVariable.h"

namespace core
{
	class NotificationQueue
	{
		Queue<Func<void()>> m_queue;
		Mutex m_mutex;
		ConditionVariable m_condition;
		bool done = false;
	public:
		NotificationQueue(Allocator* allocator)
			: m_queue(allocator),
			  m_mutex(allocator),
			  m_condition(allocator)
		{}

		void done()
		{
			{
				Lock lock{m_mutex};
				done = true;
			}
			m_condition.notify_all();
		}

		bool pop(Func<void()>& func)
		{
			{
				Lock lock{m_mutex};
				while (m_queue.count() == 0 && !done)
					m_condition.wait(m_mutex);
				if (m_queue.count() == 0)
				{
					return false;
				}
				func = std::move(m_queue.front());
				m_queue.pop_front();
			}
			return true;
		}

		void push(Func<void()>&& func)
		{
			{
				Lock lock{m_mutex};
				m_queue.push_back(std::move(func));
			}
			m_condition.notify_one();
		}
	};
}