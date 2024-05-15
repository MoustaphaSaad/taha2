#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"

namespace core
{
	class Mallocator: public Allocator
	{
	public:
		CORE_EXPORT void* alloc(size_t size, size_t alignment) override;
		CORE_EXPORT void commit(void* ptr, size_t size) override;
		CORE_EXPORT void release(void* ptr, size_t size) override;
		CORE_EXPORT void free(void* ptr, size_t size) override;
	};
}