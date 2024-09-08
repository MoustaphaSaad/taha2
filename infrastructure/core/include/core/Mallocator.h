#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"

namespace core
{
	class Mallocator: public Allocator
	{
	public:
		CORE_EXPORT Span<std::byte> alloc(size_t size, size_t alignment) override;
		CORE_EXPORT void commit(Span<std::byte> bytes) override;
		CORE_EXPORT void release(Span<std::byte> bytes) override;
		CORE_EXPORT void free(Span<std::byte> bytes) override;
	};
}