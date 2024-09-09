#pragma once

#include "core/Exports.h"
#include "core/Unique.h"

namespace core
{
	class ConditionVariable;
	class Mutex
	{
		friend class ConditionVariable;
		struct IMutex;
		Unique<IMutex> m_mutex;
	public:
		CORE_EXPORT explicit Mutex(Allocator* allocator);
		CORE_EXPORT Mutex(Mutex&& other) noexcept;
		CORE_EXPORT Mutex& operator=(Mutex&& other) noexcept;
		CORE_EXPORT ~Mutex();

		CORE_EXPORT void lock();
		CORE_EXPORT bool tryLock();
		CORE_EXPORT void unlock();
	};
}