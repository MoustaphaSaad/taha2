#include "core/Thread.h"
#include "core/Assert.h"

#include <Windows.h>

namespace core
{
	struct Thread::IThread
	{
		HANDLE handle = INVALID_HANDLE_VALUE;
	};

	struct ThreadFuncData
	{
		Allocator* allocator = nullptr;
		Func<void()> func;
	};

	Thread::Thread(Allocator* allocator, Func<void()> func, size_t stackSize)
	{
		auto thread_start = +[](void* userData) -> DWORD {
			auto funcData = (ThreadFuncData*)userData;
			Unique threadFuncData{funcData->allocator, funcData};
			threadFuncData->func();
			return 0;
		};

		auto threadFuncData = unique_from<ThreadFuncData>(allocator);
		threadFuncData->allocator = allocator;
		threadFuncData->func = std::move(func);

		m_thread = unique_from<IThread>(allocator);
		m_thread->handle = CreateThread(nullptr, stackSize, thread_start, threadFuncData.leak(), 0, nullptr);
	}

	Thread::Thread(Thread&& other) = default;
	Thread& Thread::operator=(Thread&& other) = default;

	Thread::~Thread()
	{
		if (m_thread && m_thread->handle != INVALID_HANDLE_VALUE)
		{
			[[maybe_unused]] auto res = CloseHandle(m_thread->handle);
			assert(res == TRUE);
		}
	}

	void Thread::join()
	{
		[[maybe_unused]] auto res = WaitForSingleObject(m_thread->handle, INFINITE);
		assert(res == WAIT_OBJECT_0);
	}

	void Thread::detach()
	{
		if (m_thread->handle != INVALID_HANDLE_VALUE)
		{
			[[maybe_unused]] auto res = CloseHandle(m_thread->handle);
			assert(res == TRUE);
		}
		m_thread->handle = INVALID_HANDLE_VALUE;
	}

	int Thread::hardware_concurrency()
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
	}
}