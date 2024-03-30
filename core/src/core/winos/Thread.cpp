#include "core/Thread.h"
#include "core/Assert.h"

#include <Windows.h>

namespace core
{
	struct Thread::IThread
	{
		HANDLE handle = INVALID_HANDLE_VALUE;
		Func<void()> func;
	};

	Thread::Thread(Allocator* allocator, Func<void()> func, size_t stackSize)
	{
		auto thread_start = +[](void* user_data) -> DWORD
		{
			auto thread = (Thread::IThread*)(user_data);
			thread->func();
			return 0;
		};

		m_thread = unique_from<IThread>(allocator);
		m_thread->func = std::move(func);
		m_thread->handle = CreateThread(nullptr, stackSize, thread_start, m_thread.get(), 0, nullptr);
	}

	Thread::Thread(Thread&& other) = default;
	Thread& Thread::operator=(Thread&& other) = default;

	Thread::~Thread()
	{
		if (m_thread->handle != INVALID_HANDLE_VALUE)
		{
			[[maybe_unused]] auto res = CloseHandle(m_thread->handle);
			coreAssert(res == TRUE);
		}
	}

	void Thread::join()
	{
		[[maybe_unused]] auto res = WaitForSingleObject(m_thread->handle, INFINITE);
		coreAssert(res == WAIT_OBJECT_0);
	}

	void Thread::detach()
	{
		if (m_thread->handle != INVALID_HANDLE_VALUE)
		{
			[[maybe_unused]] auto res = CloseHandle(m_thread->handle);
			coreAssert(res == TRUE);
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