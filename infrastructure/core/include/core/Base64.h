#pragma once

#include "core/Buffer.h"
#include "core/Exports.h"
#include "core/Span.h"
#include "core/String.h"

namespace core
{
	class Base64
	{
	public:
		CORE_EXPORT static String encode(Span<std::byte> bytes, Allocator* allocator);
		CORE_EXPORT static Buffer decode(StringView str, Allocator* allocator);
	};
}