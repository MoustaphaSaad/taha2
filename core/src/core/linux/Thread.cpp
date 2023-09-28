#include "core/Thread.h"

#include <pthread.h>
#include <assert.h>

namespace core
{
	struct Thread::IThread
	{
		pthread_t handle;
		Func<void()> func;
	};

	Thread::Thread(Allocator* allocator, Func<void()> func)
	{
		auto thread_start = +[](void* user_data) -> void*
		{
			auto thread = (Thread::IThread*)(user_data);
			thread->func();
			return 0;
		};

		m_thread = unique_from<IThread>(allocator);
		m_thread->func = std::move(func);
		[[maybe_unused]] auto res = pthread_create(&m_thread->handle, nullptr, thread_start, m_thread.get());
		assert(res == 0);
	}

	Thread::~Thread()
	{}

	void Thread::join()
	{
		[[maybe_unused]] auto res = pthread_join(m_thread->handle, nullptr);
		assert(res == 0);
	}
}