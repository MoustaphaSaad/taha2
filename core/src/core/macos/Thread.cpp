#include "core/Thread.h"
#include "core/Assert.h"

#include <pthread.h>
#include <unistd.h>

namespace core
{
	struct Thread::IThread
	{
		pthread_t handle;
		Func<void()> func;
	};

	Thread::Thread(Allocator* allocator, Func<void()> func, size_t stackSize)
	{
		auto thread_start = +[](void* user_data) -> void*
		{
			auto thread = (Thread::IThread*)(user_data);
			thread->func();
			return 0;
		};

		pthread_attr_t attr{};
		[[maybe_unused]] auto res = pthread_attr_init(&attr);
		coreAssert(res == 0);
		if (stackSize != 0)
		{
			res = pthread_attr_setstacksize(&attr, stackSize);
			coreAssert(res == 0);
		}

		m_thread = unique_from<IThread>(allocator);
		m_thread->func = std::move(func);
		res = pthread_create(&m_thread->handle, &attr, thread_start, m_thread.get());
		coreAssert(res == 0);
	}

	Thread::Thread(Thread&& other) = default;
	Thread& Thread::operator=(Thread&& other) = default;
	Thread::~Thread() = default;

	void Thread::join()
	{
		[[maybe_unused]] auto res = pthread_join(m_thread->handle, nullptr);
		coreAssert(res == 0);
	}

	int Thread::hardware_concurrency()
	{
		return sysconf(_SC_NPROCESSORS_ONLN);
	}
}