#include "core/OS.h"

#include <unistd.h>

namespace core
{
	uint64_t OS::pageSize()
	{
		return uint64_t(sysconf(_SC_PAGESIZE));
	}
}