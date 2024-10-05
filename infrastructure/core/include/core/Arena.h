#pragma once

#include "core/Allocator.h"
#include "core/Exports.h"

#include <mimalloc.h>

namespace core
{
	class Arena: public Allocator
	{
		mi_heap_t* m_heap = nullptr;

	public:
		Arena()
			: m_heap(mi_heap_new())
		{}

		Arena(const Arena&) = delete;
		Arena(Arena&& other) noexcept
			: m_heap(other.m_heap)
		{
			other.m_heap = nullptr;
		}

		Arena& operator=(const Arena&) = delete;
		Arena& operator=(Arena&& other) noexcept
		{
			if (m_heap)
			{
				mi_heap_destroy(m_heap);
			}
			m_heap = other.m_heap;
			other.m_heap = nullptr;
			return *this;
		}

		~Arena() override
		{
			if (m_heap)
			{
				mi_heap_destroy(m_heap);
			}
			m_heap = nullptr;
		}

		CORE_EXPORT Span<std::byte> alloc(size_t size, size_t alignment) override;
		CORE_EXPORT void commit(Span<std::byte> bytes) override;
		CORE_EXPORT void release(Span<std::byte> bytes) override;
		CORE_EXPORT void free(Span<std::byte> bytes) override;
	};
}