#include "core/Mutex.h"
#include "core/Assert.h"
#include "core/linux/IMutex.h"

#include <pthread.h>

namespace core
{
	Mutex::Mutex(Allocator* allocator)
	{
		m_mutex = unique_from<IMutex>(allocator);
		[[maybe_unused]] auto res = pthread_mutex_init(&m_mutex->handle, nullptr);
		assertTrue(res == 0);
	}

	Mutex::Mutex(Mutex&& other) noexcept = default;
	Mutex& Mutex::operator=(Mutex&& other) noexcept = default;

	Mutex::~Mutex()
	{
		if (m_mutex)
		{
			[[maybe_unused]] auto res = pthread_mutex_destroy(&m_mutex->handle);
			assertTrue(res == 0);
		}
	}

	void Mutex::lock()
	{
		[[maybe_unused]] auto res = pthread_mutex_lock(&m_mutex->handle);
		assertTrue(res == 0);
	}

	bool Mutex::tryLock()
	{
		auto res = pthread_mutex_trylock(&m_mutex->handle);
		return res == 0;
	}

	void Mutex::unlock()
	{
		[[maybe_unused]] auto res = pthread_mutex_unlock(&m_mutex->handle);
		assertTrue(res == 0);
	}
}