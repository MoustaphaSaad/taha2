#include "core/Mutex.h"
#include "core/winos/IMutex.h"

#include <Windows.h>

namespace core
{
	Mutex::Mutex(Allocator* allocator)
	{
		m_mutex = unique_from<IMutex>(allocator);
		InitializeCriticalSectionAndSpinCount(&m_mutex->cs, 1 << 14);
	}

	Mutex::Mutex(Mutex&& other) noexcept = default;
	Mutex& Mutex::operator=(Mutex&& other) noexcept = default;

	Mutex::~Mutex()
	{
		if (m_mutex)
		{
			DeleteCriticalSection(&m_mutex->cs);
		}
	}

	void Mutex::lock()
	{
		EnterCriticalSection(&m_mutex->cs);
	}

	bool Mutex::tryLock()
	{
		return TryEnterCriticalSection(&m_mutex->cs);
	}

	void Mutex::unlock()
	{
		LeaveCriticalSection(&m_mutex->cs);
	}
}