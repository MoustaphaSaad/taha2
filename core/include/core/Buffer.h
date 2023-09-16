#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/StringView.h"

#include <cstddef>
#include <utility>
#include <cassert>

namespace core
{
	class Buffer
	{
		Allocator* m_allocator = nullptr;
		std::byte* m_ptr = nullptr;
		size_t m_capacity = 0;
		size_t m_count = 0;

		CORE_EXPORT void destroy();

		CORE_EXPORT void copyFrom(const Buffer& other);

		CORE_EXPORT void moveFrom(Buffer&& other);

		void grow(size_t new_capacity);

		CORE_EXPORT void ensureSpaceExists(size_t count);

	public:
		explicit Buffer(Allocator* allocator)
			: m_allocator(allocator)
		{}

		Buffer(const Buffer& other)
		{
			copyFrom(other);
		}

		Buffer(Buffer&& other)
		{
			moveFrom(std::move(other));
		}

		Buffer& operator=(const Buffer& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Buffer& operator=(Buffer&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~Buffer()
		{
			destroy();
		}

		std::byte& operator[](size_t i)
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		const std::byte& operator[](size_t i) const
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		void push(std::byte b)
		{
			ensureSpaceExists(1);
			m_ptr[m_count++] = b;
		}

		void push(StringView v)
		{
			push((const std::byte*)v.data(), v.count());
		}

		CORE_EXPORT void push(const std::byte* ptr, size_t size);

		CORE_EXPORT void resize(size_t new_count);
		void reserve(size_t extra_count) { ensureSpaceExists(extra_count); }
		CORE_EXPORT void clear();

		size_t count() const { return m_count; }
		size_t capacity() const { return m_capacity; }
		std::byte* data() { return m_ptr; }
		const std::byte* data() const { return m_ptr; }
	};
}