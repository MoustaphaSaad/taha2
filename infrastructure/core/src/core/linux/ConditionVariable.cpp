#include "core/ConditionVariable.h"
#include "core/linux/IMutex.h"
#include "core/Assert.h"

#include <pthread.h>

namespace core
{
	struct ConditionVariable::IConditionVariable
	{
		pthread_cond_t cv;
	};

	ConditionVariable::ConditionVariable(Allocator* allocator)
	{
		m_condition_variable = unique_from<IConditionVariable>(allocator);
		[[maybe_unused]] auto res = pthread_cond_init(&m_condition_variable->cv, nullptr);
		validate(res == 0);
	}

	ConditionVariable::ConditionVariable(ConditionVariable&& other) noexcept = default;
	ConditionVariable& ConditionVariable::operator=(ConditionVariable&& other) noexcept = default;

	ConditionVariable::~ConditionVariable()
	{
		if (m_condition_variable)
		{
			[[maybe_unused]] auto res = pthread_cond_destroy(&m_condition_variable->cv);
			validate(res == 0);
		}
	}

	void ConditionVariable::wait(Mutex& mutex)
	{
		[[maybe_unused]] auto res = pthread_cond_wait(&m_condition_variable->cv, &mutex.m_mutex->handle);
	}

	void ConditionVariable::notify_one()
	{
		pthread_cond_signal(&m_condition_variable->cv);
	}

	void ConditionVariable::notify_all()
	{
		pthread_cond_broadcast(&m_condition_variable->cv);
	}
}