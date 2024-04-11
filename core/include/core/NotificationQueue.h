#pragma once

#include "core/Queue.h"
#include "core/Func.h"
#include "core/Mutex.h"
#include "core/Lock.h"
#include "core/ConditionVariable.h"
#include "core/Shared.h"

namespace core
{
	class ExecutionQueue;

	struct NotificationQueueEntry
	{
		Func<void()> func;
		Weak<ExecutionQueue> executionQueue;
	};

	class NotificationQueue
	{
		Queue<NotificationQueueEntry> m_queue;
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
			auto lock = lockGuard(m_mutex);
			m_done = true;
			m_condition.notify_all();
		}

		bool pop(NotificationQueueEntry& func)
		{
			auto lock = lockGuard(m_mutex);
			while (m_queue.count() == 0 && m_done == false)
				m_condition.wait(m_mutex);
			if (m_queue.count() == 0)
				return false;
			func = std::move(m_queue.front());
			m_queue.pop_front();
			return true;
		}

		bool tryPop(NotificationQueueEntry& func)
		{
			auto lock = tryLockGuard(m_mutex);
			if (lock.isLocked() == false || m_queue.count() == 0)
				return false;
			func = std::move(m_queue.front());
			m_queue.pop_front();
			return true;
		}

		bool tryPush(Func<void()>&& func, const Weak<ExecutionQueue>& execQueue)
		{
			auto lock = tryLockGuard(m_mutex);
			if (lock.isLocked() == false)
				return false;
			m_queue.push_back(NotificationQueueEntry{.func = std::move(func), .executionQueue = execQueue});
			m_condition.notify_one();
			return true;
		}

		void push(Func<void()>&& func, const Weak<ExecutionQueue>& execQueue)
		{
			auto lock = lockGuard(m_mutex);
			m_queue.push_back(NotificationQueueEntry{.func = std::move(func), .executionQueue = execQueue});
			m_condition.notify_one();
		}
	};
}