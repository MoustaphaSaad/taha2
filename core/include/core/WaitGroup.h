#pragma once

#include "core/Mutex.h"
#include "core/ConditionVariable.h"

#include <cassert>

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
			assert(count > 0);

			auto lock = Lock<Mutex>::lock(m_mutex);
			m_count += count;
		}

		void done()
		{
			auto lock = Lock<Mutex>::lock(m_mutex);
			m_count--;
			assert(m_count >= 0);
			if (m_count == 0)
			{
				m_condition_variable.notify_all();
			}
		}

		void wait()
		{
			auto lock = Lock<Mutex>::lock(m_mutex);
			while (m_count > 0)
				m_condition_variable.wait(m_mutex);
			assert(m_count == 0);
		}
	};
}