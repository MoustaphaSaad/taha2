#pragma once

#include "core/Span.h"

#include <cstddef>

namespace core
{
	class Allocator
	{
	public:
		virtual ~Allocator() = default;

		virtual Span<std::byte> alloc(size_t size, size_t alignment) = 0;
		virtual void commit(Span<std::byte> bytes) = 0;
		virtual void release(Span<std::byte> bytes) = 0;
		virtual void free(Span<std::byte> bytes) = 0;
	};
}
