#pragma once

#include "core/Exports.h"
#include "core/Mutex.h"
#include "core/Unique.h"

namespace core
{
	class ConditionVariable
	{
		struct IConditionVariable;
		Unique<IConditionVariable> m_condition_variable;

	public:
		CORE_EXPORT explicit ConditionVariable(Allocator* allocator);
		CORE_EXPORT ConditionVariable(ConditionVariable&& other) noexcept;
		CORE_EXPORT ConditionVariable& operator=(ConditionVariable&& other) noexcept;
		CORE_EXPORT ~ConditionVariable();

		CORE_EXPORT void wait(Mutex& mutex);

		template <typename Predicate>
		void wait(Mutex& mutex, Predicate&& predicate)
		{
			while (!predicate())
			{
				wait(mutex);
			}
		}

		CORE_EXPORT void notify_one();
		CORE_EXPORT void notify_all();
	};
}