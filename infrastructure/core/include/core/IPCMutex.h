#pragma once

#include "core/Exports.h"
#include "core/StringView.h"
#include "core/Unique.h"

namespace core
{
	class IPCMutex
	{
		struct IIPCMutex;
		Unique<IIPCMutex> m_mutex;

	public:
		CORE_EXPORT IPCMutex(StringView name, Allocator* allocator);
		CORE_EXPORT IPCMutex(IPCMutex&& other);
		CORE_EXPORT IPCMutex& operator=(IPCMutex&& other);
		CORE_EXPORT ~IPCMutex();

		CORE_EXPORT void lock();
		CORE_EXPORT bool tryLock();
		CORE_EXPORT void unlock();
	};
}