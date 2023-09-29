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

	Mutex::~Mutex()
	{
		DeleteCriticalSection(&m_mutex->cs);
	}

	void Mutex::lock()
	{
		EnterCriticalSection(&m_mutex->cs);
	}

	bool Mutex::try_lock()
	{
		return TryEnterCriticalSection(&m_mutex->cs);
	}

	void Mutex::unlock()
	{
		LeaveCriticalSection(&m_mutex->cs);
	}
}