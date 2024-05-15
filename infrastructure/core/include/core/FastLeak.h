#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"

#include <atomic>

namespace core
{
	class FastLeak: public Allocator
	{
		std::atomic<size_t> atomic_size = 0;
		std::atomic<size_t> atomic_count = 0;
	public:
		CORE_EXPORT FastLeak() = default;
		CORE_EXPORT ~FastLeak() override;

		CORE_EXPORT void* alloc(size_t size, size_t alignment) override;
		CORE_EXPORT void commit(void* ptr, size_t size) override;
		CORE_EXPORT void release(void* ptr, size_t size) override;
		CORE_EXPORT void free(void* ptr, size_t size) override;
	};
}
