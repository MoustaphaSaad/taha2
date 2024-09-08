#include "core/MiMallocator.h"

#include <tracy/Tracy.hpp>

#include <mimalloc.h>

namespace core
{
	Span<std::byte> Mimallocator::alloc(size_t size, size_t)
	{
		auto res = (std::byte*)mi_malloc(size);
		TracyAllocS(res, size, 10);
		return Span<std::byte>{res, size};
	}

	void Mimallocator::commit(Span<std::byte>)
	{
		// do nothing
	}

	void Mimallocator::release(Span<std::byte>)
	{
		// do nothing
	}

	void Mimallocator::free(Span<std::byte> bytes)
	{
		TracyFreeS(bytes.data(), 10);
		mi_free(bytes.data());
	}
}