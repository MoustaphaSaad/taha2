#include "core/Arena.h"

#include <tracy/Tracy.hpp>

namespace core
{
	Span<std::byte> Arena::alloc(size_t size, size_t)
	{
		auto res = (std::byte*)mi_heap_malloc(m_heap, size);
		TracyAllocS(res, size, 10);
		return Span<std::byte>{res, size};
	}

	void Arena::commit(Span<std::byte>)
	{
		// do nothing
	}

	void Arena::release(Span<std::byte>)
	{
		// do nothing
	}

	void Arena::free(Span<std::byte> bytes)
	{
		TracyFreeS(bytes.data(), 10);
		// do nothing
	}
}