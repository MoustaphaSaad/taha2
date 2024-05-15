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

	void* FastLeak::alloc(size_t size, size_t)
	{
		if (size == 0)
			return nullptr;

		atomic_count.fetch_add(1);
		atomic_size.fetch_add(size);

		return ::malloc(size);
	}

	void FastLeak::commit(void*, size_t)
	{
		// do nothing
	}

	void FastLeak::release(void*, size_t)
	{
		// do nothing
	}

	void FastLeak::free(void* ptr, size_t size)
	{
		if (ptr == nullptr)
			return;

		atomic_count.fetch_sub(1);
		atomic_size.fetch_sub(size);

		::free(ptr);
	}
}