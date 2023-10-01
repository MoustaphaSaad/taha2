#include "core/Thread.h"

#include <Windows.h>
#include <cassert>

namespace core
{
	struct Thread::IThread
	{
		HANDLE handle;
		Func<void()> func;
	};

	Thread::Thread(Allocator* allocator, Func<void()> func)
	{
		auto thread_start = +[](void* user_data) -> DWORD
		{
			auto thread = (Thread::IThread*)(user_data);
			thread->func();
			return 0;
		};

		m_thread = unique_from<IThread>(allocator);
		m_thread->func = std::move(func);
		m_thread->handle = CreateThread(nullptr, 0, thread_start, m_thread.get(), 0, nullptr);
	}

	Thread::Thread(Thread&& other) = default;
	Thread& Thread::operator=(Thread&& other) = default;

	Thread::~Thread()
	{
		if (m_thread)
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

	int Thread::hardware_concurrency()
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
	}
}