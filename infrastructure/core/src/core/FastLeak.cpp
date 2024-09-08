#include "core/FastLeak.h"

#include <cstdlib>
#include <cstdio>

namespace core
{
	FastLeak::~FastLeak()
	{
		if (atomic_count > 0)
		{
			::fprintf(
				stderr,
				"Leaks count: %zu, Leaks size(bytes): %zu\n",
				atomic_count.load(),
				atomic_size.load()
			);
		}
	}

	Span<std::byte> FastLeak::alloc(size_t size, size_t)
	{
		if (size == 0)
			return Span<std::byte>{};

		atomic_count.fetch_add(1);
		atomic_size.fetch_add(size);

		auto ptr = (std::byte*)::malloc(size);
		return Span<std::byte>{ptr, size};
	}

	void FastLeak::commit(Span<std::byte>)
	{
		// do nothing
	}

	void FastLeak::release(Span<std::byte>)
	{
		// do nothing
	}

	void FastLeak::free(Span<std::byte> bytes)
	{
		if (bytes.sizeInBytes() == 0)
			return;

		atomic_count.fetch_sub(1);
		atomic_size.fetch_sub(bytes.sizeInBytes());

		::free(bytes.data());
	}
}