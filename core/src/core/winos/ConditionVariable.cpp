#include "core/ConditionVariable.h"
#include "core/winos/IMutex.h"

#include <Windows.h>

namespace core
{
	struct ConditionVariable::IConditionVariable
	{
		CONDITION_VARIABLE cv;
	};

	ConditionVariable::ConditionVariable(Allocator* allocator)
	{
		m_condition_variable = unique_from<IConditionVariable>(allocator);
		InitializeConditionVariable(&m_condition_variable->cv);
	}

	ConditionVariable::~ConditionVariable()
	{
	}

	void ConditionVariable::wait(Mutex& mutex)
	{
		SleepConditionVariableCS(&m_condition_variable->cv, &mutex.m_mutex->cs, INFINITE);
	}

	void ConditionVariable::notify_one()
	{
		WakeConditionVariable(&m_condition_variable->cv);
	}

	void ConditionVariable::notify_all()
	{
		WakeAllConditionVariable(&m_condition_variable->cv);
	}
}