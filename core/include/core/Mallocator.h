#pragma once

#include "core/Allocator.h"

namespace core
{
	class Mallocator: public Allocator
	{
	public:
		void* alloc(size_t size, size_t alignment) override;
		void commit(void* ptr, size_t size) override;
		void release(void* ptr, size_t size) override;
		void free(void* ptr, size_t size) override;
	};
}