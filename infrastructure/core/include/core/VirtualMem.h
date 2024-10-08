#pragma once

#include "core/Allocator.h"
#include "core/Exports.h"

namespace core
{
	class VirtualMem: public Allocator
	{
	public:
		CORE_EXPORT Span<std::byte> alloc(size_t size, size_t alignment) override;
		CORE_EXPORT void commit(Span<std::byte> bytes) override;
		CORE_EXPORT void release(Span<std::byte> bytes) override;
		CORE_EXPORT void free(Span<std::byte> bytes) override;
	};
}