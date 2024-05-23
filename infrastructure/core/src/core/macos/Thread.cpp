#include "core/Thread.h"
#include "core/Assert.h"

#include <pthread.h>
#include <unistd.h>

namespace core
{
	struct Thread::IThread
	{
		pthread_t handle;
		bool detached = false;
	};

	struct ThreadFuncData
	{
		Allocator* allocator = nullptr;
		Func<void()> func;
	};

	Thread::Thread(Allocator* allocator, Func<void()> func, size_t stackSize)
	{
		auto thread_start = +[](void* userData) -> void*
		{
			auto funcData = (ThreadFuncData*)userData;
			Unique threadFuncData{funcData->allocator, funcData};
			threadFuncData->func();
			return 0;
		};

		auto threadFuncData = unique_from<ThreadFuncData>(allocator);
		threadFuncData->allocator = allocator;
		threadFuncData->func = std::move(func);

		pthread_attr_t attr{};
		[[maybe_unused]] auto res = pthread_attr_init(&attr);
		coreAssert(res == 0);
		if (stackSize != 0)
		{
			res = pthread_attr_setstacksize(&attr, stackSize);
			coreAssert(res == 0);
		}

		m_thread = unique_from<IThread>(allocator);
		res = pthread_create(&m_thread->handle, &attr, thread_start, threadFuncData.leak());
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

	void Thread::detach()
	{
		if (m_thread->detached == false)
		{
			[[maybe_unused]] auto res = pthread_detach(m_thread->handle);
			coreAssert(res == 0);
		}
		m_thread->detached = true;
	}

	int Thread::hardware_concurrency()
	{
		return sysconf(_SC_NPROCESSORS_ONLN);
	}
}