#include "core/IPCMutex.h"
#include "core/OSString.h"

#include <Windows.h>

namespace core
{
	struct IPCMutex::IIPCMutex
	{
		HANDLE handle = INVALID_HANDLE_VALUE;
	};

	IPCMutex::IPCMutex(StringView name, Allocator* allocator)
	{
		auto osName = OSString{name, allocator};
		auto handle = CreateMutex(0, false, (LPCWSTR)osName.data());
		coreAssert(handle != INVALID_HANDLE_VALUE);

		m_mutex = unique_from<IIPCMutex>(allocator);
		m_mutex->handle = handle;
	}

	IPCMutex::IPCMutex(IPCMutex&& other) = default;
	IPCMutex& IPCMutex::operator=(IPCMutex&& other) = default;

	IPCMutex::~IPCMutex()
	{
		if (m_mutex)
		{
			[[maybe_unused]] auto res = CloseHandle(m_mutex->handle);
			coreAssert(res == TRUE);
		}
	}

	void IPCMutex::lock()
	{
		WaitForSingleObject(m_mutex->handle, INFINITE);
	}

	bool IPCMutex::tryLock()
	{
		auto res = WaitForSingleObject(m_mutex->handle, 0);
		return (
			res == WAIT_OBJECT_0 ||
			res == WAIT_ABANDONED
		);
	}

	void IPCMutex::unlock()
	{
		[[maybe_unused]] auto res = ReleaseMutex(m_mutex->handle);
		coreAssert(res == TRUE);
	}
}