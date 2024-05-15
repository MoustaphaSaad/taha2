#include "core/OS.h"

#include <Windows.h>

namespace core
{
	uint64_t OS::pageSize()
	{
		SYSTEM_INFO info{};
		GetSystemInfo(&info);
		return uint64_t(info.dwPageSize);
	}
}