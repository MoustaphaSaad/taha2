#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Func.h"

namespace core
{
	class Thread
	{
		struct IThread;
		Unique<IThread> m_thread;
	public:
		CORE_EXPORT Thread(Allocator* allocator, Func<void()> func, size_t stackSize = 0);
		CORE_EXPORT Thread(Thread&& other);
		CORE_EXPORT Thread& operator=(Thread&& other);
		CORE_EXPORT ~Thread();
		CORE_EXPORT void join();

		CORE_EXPORT static int hardware_concurrency();
	};
}