#include "core/Arena.h"

#include <tracy/Tracy.hpp>

namespace core
{
	void* Arena::alloc(size_t size, size_t)
	{
		auto res = mi_heap_malloc(m_heap, size);
		TracyAllocS(res, size, 10);
		return res;
	}

	void Arena::commit(void* ptr, size_t size)
	{
		// do nothing
	}

	void Arena::release(void* ptr, size_t size)
	{
		// do nothing
	}

	void Arena::free(void* ptr, size_t)
	{
		TracyFreeS(ptr, 10);
		// do nothing
	}
}