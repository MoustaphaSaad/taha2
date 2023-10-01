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
		bool m_done = false;
	public:
		NotificationQueue(Allocator* allocator)
			: m_queue(allocator),
			  m_mutex(allocator),
			  m_condition(allocator)
		{}

		void done()
		{
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_done = true;
			}
			m_condition.notify_all();
		}

		bool pop(Func<void()>& func)
		{
			auto lock = Lock<Mutex>::lock(m_mutex);
			while (m_queue.count() == 0 && m_done == false)
				m_condition.wait(m_mutex);
			if (m_queue.count() == 0)
				return false;
			func = std::move(m_queue.front());
			m_queue.pop_front();
			return true;
		}

		bool try_pop(Func<void()>& func)
		{
			auto lock = Lock<Mutex>::try_lock(m_mutex);
			if (lock.is_locked() == false || m_queue.count() == 0)
				return false;
			func = std::move(m_queue.front());
			m_queue.pop_front();
			return true;
		}

		bool try_push(Func<void()>&& func)
		{
			{
				auto lock = Lock<Mutex>::try_lock(m_mutex);
				if (lock.is_locked() == false)
					return false;
				m_queue.push_back(std::move(func));
			}
			m_condition.notify_one();
			return true;
		}

		void push(Func<void()>&& func)
		{
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_queue.push_back(std::move(func));
			}
			m_condition.notify_one();
		}
	};
}