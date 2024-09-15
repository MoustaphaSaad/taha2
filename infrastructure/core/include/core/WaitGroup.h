#pragma once

#include "core/Mutex.h"
#include "core/Lock.h"
#include "core/ConditionVariable.h"
#include "core/Assert.h"

namespace core
{
	class WaitGroup
	{
		Mutex m_mutex;
		ConditionVariable m_condition_variable;
		int m_count = 0;
	public:
		WaitGroup(Allocator* allocator)
			: m_mutex(allocator)
			, m_condition_variable(allocator)
		{}

		WaitGroup(WaitGroup&& other) = default;
		WaitGroup& operator=(WaitGroup&& other) = default;
		~WaitGroup() = default;

		void add(int count)
		{
			validate(count > 0);

			auto lock = lockGuard(m_mutex);
			m_count += count;
		}

		void done()
		{
			auto lock = lockGuard(m_mutex);
			m_count--;
			validate(m_count >= 0);
			if (m_count == 0)
			{
				m_condition_variable.notify_all();
			}
		}

		void wait()
		{
			auto lock = lockGuard(m_mutex);
			while (m_count > 0)
				m_condition_variable.wait(m_mutex);
			validate(m_count == 0);
		}
	};
}