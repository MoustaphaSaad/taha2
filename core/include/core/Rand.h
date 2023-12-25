#pragma once

#include "core/Exports.h"
#include "core/Span.h"

namespace core
{
	class Rand
	{
	public:
		CORE_EXPORT static bool cryptoRand(Span<std::byte> buffer);
	};
}