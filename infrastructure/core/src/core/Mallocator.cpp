#include "core/Mallocator.h"

#include <tracy/Tracy.hpp>

#include <cstdlib>

namespace core
{
	Span<std::byte> Mallocator::alloc(size_t size, size_t)
	{
		auto res = (std::byte*)malloc(size);
		TracyAllocS(res, size, 10);
		return Span<std::byte>{res, size};
	}

	void Mallocator::commit(Span<std::byte>)
	{
		// do nothing
	}

	void Mallocator::release(Span<std::byte>)
	{
		// do nothing
	}

	void Mallocator::free(Span<std::byte> bytes)
	{
		TracyFreeS(ptr, 10);
		::free(bytes.data());
	}
}