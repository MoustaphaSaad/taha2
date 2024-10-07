#pragma once

#include "core/Allocator.h"
#include "core/Exports.h"

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

		CORE_EXPORT Span<std::byte> alloc(size_t size, size_t alignment) override;
		CORE_EXPORT void commit(Span<std::byte> bytes) override;
		CORE_EXPORT void release(Span<std::byte> bytes) override;
		CORE_EXPORT void free(Span<std::byte> bytes) override;
	};
}
