#pragma once

#include "core/Allocator.h"

#include <atomic>

namespace core
{
	class FastLeak: public Allocator
	{
		std::atomic<size_t> atomic_size = 0;
		std::atomic<size_t> atomic_count = 0;
	public:
		FastLeak() = default;
		~FastLeak();

		void* alloc(size_t size, size_t alignment) override;
		void commit(void* ptr, size_t size) override;
		void release(void* ptr, size_t size) override;
		void free(void* ptr, size_t size) override;
	};
}
