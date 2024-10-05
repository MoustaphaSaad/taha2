#pragma once

namespace core
{
	template <typename T>
	class Lock;

	template <typename T>
	inline Lock<T> lockGuard(T& lockable);

	template <typename T>
	inline Lock<T> tryLockGuard(T& lockable);

	template <typename T>
	class Lock
	{
		template <typename U>
		friend inline Lock<U> lockGuard(U& lockable);
		template <typename U>
		friend inline Lock<U> tryLockGuard(U& lockable);

		T* m_lockable = nullptr;
		bool m_locked = false;

		Lock(T& lockable, bool locked)
			: m_lockable(&lockable),
			  m_locked(locked)
		{}

		void destroy()
		{
			if (m_lockable && m_locked)
			{
				m_lockable->unlock();
			}
		}

		void moveFrom(Lock& other)
		{
			m_lockable = other.m_lockable;
			m_locked = other.m_locked;

			other.m_lockable = nullptr;
			other.m_locked = false;
		}

	public:
		Lock(const Lock&) = delete;

		Lock(Lock&& other) noexcept
		{
			moveFrom(other);
		}

		Lock& operator=(const Lock&) = delete;

		Lock& operator=(Lock&& other) noexcept
		{
			destroy();
			moveFrom(other);
			return *this;
		}

		~Lock()
		{
			destroy();
		}

		bool isLocked() const
		{
			return m_locked;
		}
	};

	template <typename T>
	inline Lock<T> lockGuard(T& lockable)
	{
		lockable.lock();
		return Lock<T>{lockable, true};
	}

	template <typename T>
	inline Lock<T> tryLockGuard(T& lockable)
	{
		if (lockable.tryLock())
		{
			return Lock{lockable, true};
		}
		return Lock<T>{lockable, false};
	}
}