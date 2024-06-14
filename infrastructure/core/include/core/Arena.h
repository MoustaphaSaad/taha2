#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"

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
		Arena(Arena&& other)
			: m_heap(other.m_heap)
		{
			other.m_heap = nullptr;
		}

		Arena& operator=(const Arena&) = delete;
		Arena& operator=(Arena&& other)
		{
			if (m_heap)
				mi_heap_destroy(m_heap);
			m_heap = other.m_heap;
			other.m_heap = nullptr;
			return *this;
		}

		~Arena() override
		{
			if (m_heap)
				mi_heap_destroy(m_heap);
			m_heap = nullptr;
		}

		CORE_EXPORT void* alloc(size_t size, size_t alignment) override;
		CORE_EXPORT void commit(void* ptr, size_t size) override;
		CORE_EXPORT void release(void* ptr, size_t size) override;
		CORE_EXPORT void free(void* ptr, size_t size) override;
	};
}