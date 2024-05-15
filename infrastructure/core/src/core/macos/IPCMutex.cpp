#include "core/IPCMutex.h"
#include "core/String.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>

namespace core
{
	struct IPCMutex::IIPCMutex
	{
		int handle = -1;
	};

	IPCMutex::IPCMutex(StringView name, Allocator* allocator)
	{
		int flags = O_WRONLY | O_CREAT | O_APPEND;
		auto osName = String{name, allocator};
		auto handle = ::open(osName.data(), flags, S_IRWXU);
		coreAssert(handle != -1);

		m_mutex = unique_from<IIPCMutex>(allocator);
		m_mutex->handle = handle;
	}

	IPCMutex::IPCMutex(IPCMutex&& other) = default;
	IPCMutex& IPCMutex::operator=(IPCMutex&& other) = default;

	IPCMutex::~IPCMutex()
	{
		if (m_mutex)
			::close(m_mutex->handle);
	}

	void IPCMutex::lock()
	{
		[[maybe_unused]] auto res = flock(m_mutex->handle, LOCK_EX);
		coreAssert(res != -1);
	}

	bool IPCMutex::tryLock()
	{
		auto res = flock(m_mutex->handle, LOCK_EX | LOCK_NB);
		return res != -1;
	}

	void IPCMutex::unlock()
	{
		[[maybe_unused]] auto res = flock(m_mutex->handle, LOCK_UN);
		coreAssert(res != -1);
	}
}