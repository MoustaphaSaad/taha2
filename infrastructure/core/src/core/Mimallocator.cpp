#include "core/Mimallocator.h"

#include <tracy/Tracy.hpp>

#include <mimalloc.h>

namespace core
{
	void* Mimallocator::alloc(size_t size, size_t)
	{
		auto res = mi_malloc(size);
		TracyAllocS(res, size, 10);
		return res;
	}

	void Mimallocator::commit(void* ptr, size_t size)
	{
		// do nothing
	}

	void Mimallocator::release(void* ptr, size_t size)
	{
		// do nothing
	}

	void Mimallocator::free(void* ptr, size_t)
	{
		TracyFreeS(ptr, 10);
		mi_free(ptr);
	}
}