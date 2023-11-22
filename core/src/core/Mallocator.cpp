#include "core/Mallocator.h"

#include <tracy/Tracy.hpp>

#include <cstdlib>

namespace core
{
	void* Mallocator::alloc(size_t size, size_t)
	{
		auto res = malloc(size);
		TracyAllocS(res, size, 10);
		return res;
	}

	void Mallocator::commit(void* ptr, size_t size)
	{
		// do nothing
	}

	void Mallocator::release(void* ptr, size_t size)
	{
		// do nothing
	}

	void Mallocator::free(void* ptr, size_t)
	{
		TracyFreeS(ptr, 10);
		::free(ptr);
	}
}