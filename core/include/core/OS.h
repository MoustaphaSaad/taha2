#pragma once

#include "core/Exports.h"

#include <cstdint>

namespace core
{
	class OS
	{
	public:
		CORE_EXPORT static uint64_t pageSize();
	};
}