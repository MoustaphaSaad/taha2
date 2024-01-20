#pragma once

#include <cstddef>

namespace core
{
	class Allocator
	{
	public:
		virtual ~Allocator() = default;

		virtual void* alloc(size_t size, size_t alignment) = 0;
		virtual void commit(void* ptr, size_t size) = 0;
		virtual void release(void* ptr, size_t size) = 0;
		virtual void free(void* ptr, size_t size) = 0;
	};
}
