#include "core/Mutex.h"
#include "core/macos/IMutex.h"

#include <pthread.h>
#include <cassert>

namespace core
{
	Mutex::Mutex(Allocator* allocator)
	{
		m_mutex = unique_from<IMutex>(allocator);
		[[maybe_unused]] auto res = pthread_mutex_init(&m_mutex->handle, nullptr);
		assert(res == 0);
	}

	Mutex::~Mutex()
	{
		[[maybe_unused]] auto res = pthread_mutex_destroy(&m_mutex->handle);
		assert(res == 0);
	}

	void Mutex::lock()
	{
		[[maybe_unused]] auto res = pthread_mutex_lock(&m_mutex->handle);
		assert(res == 0);
	}

	bool Mutex::try_lock()
	{
		auto res = pthread_mutex_trylock(&m_mutex->handle);
		return res == 0;
	}

	void Mutex::unlock()
	{
		[[maybe_unused]] auto res = pthread_mutex_unlock(&m_mutex->handle);
		assert(res == 0);
	}
}