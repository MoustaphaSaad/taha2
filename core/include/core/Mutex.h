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
		CORE_EXPORT Mutex(Mutex&& other);
		CORE_EXPORT Mutex& operator=(Mutex&& other);
		CORE_EXPORT ~Mutex();

		CORE_EXPORT void lock();
		CORE_EXPORT bool try_lock();
		CORE_EXPORT void unlock();
	};

	template<typename T>
	class Lock;

	template<typename T>
	inline Lock<T> lockGuard(T& lockable);

	template<typename T>
	inline Lock<T> tryLockGuard(T& lockable);

	template<typename T>
	class Lock
	{
		template<typename U>
		friend inline Lock<U> lockGuard(U& lockable);
		template<typename U>
		friend inline Lock<U> tryLockGuard(U& lockable);

		T& m_lockable;
		bool m_locked = false;

		Lock(T& lockable, bool locked)
			: m_lockable(lockable),
			  m_locked(locked)
		{}
	public:
		~Lock()
		{
			if (m_locked)
				m_lockable.unlock();
		}

		bool is_locked() const { return m_locked; }
	};

	template<typename T>
	inline Lock<T> lockGuard(T& lockable)
	{
		lockable.lock();
		return Lock<T>{lockable, true};
	}

	template<typename T>
	inline Lock<T> tryLockGuard(T& lockable)
	{
		if (lockable.try_lock())
			return Lock{lockable, true};
		return Lock<T>{lockable, false};
	}
}