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
		CORE_EXPORT Mutex(Allocator* allocator);
		CORE_EXPORT ~Mutex();

		CORE_EXPORT void lock();
		CORE_EXPORT bool try_lock();
		CORE_EXPORT void unlock();
	};

	template<typename T>
	class Lock
	{
		T& m_lockable;
		bool m_locked = false;

		Lock(T& lockable, bool locked)
			: m_lockable(lockable),
			  m_locked(locked)
		{}
	public:
		static Lock lock(T& lockable)
		{
			lockable.lock();
			return Lock{lockable, true};
		}

		static Lock try_lock(T& lockable)
		{
			if (lockable.try_lock())
				return Lock{lockable, true};
			return Lock(lockable, false);
		}

		~Lock()
		{
			if (m_locked)
				m_lockable.unlock();
		}

		bool is_locked() const { return m_locked; }
	};
}