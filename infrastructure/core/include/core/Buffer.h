#pragma once

#include "core/Exports.h"
#include "core/Assert.h"
#include "core/Allocator.h"
#include "core/StringView.h"
#include "core/Span.h"

#include <cstddef>
#include <utility>

namespace core
{
	class String;

	class Buffer
	{
		friend class String;

		Allocator* m_allocator = nullptr;
		Span<std::byte> m_memory;
		size_t m_count = 0;

		CORE_EXPORT void destroy();

		CORE_EXPORT void copyFrom(const Buffer& other);

		CORE_EXPORT void moveFrom(Buffer& other);

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

		Buffer(Buffer&& other) noexcept
		{
			moveFrom(other);
		}

		CORE_EXPORT explicit Buffer(String&& str);

		Buffer& operator=(const Buffer& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Buffer& operator=(Buffer&& other) noexcept
		{
			destroy();
			moveFrom(other);
			return *this;
		}

		~Buffer()
		{
			destroy();
		}

		std::byte& operator[](size_t i)
		{
			validate(i < m_count);
			return m_memory[i];
		}

		const std::byte& operator[](size_t i) const
		{
			validate(i < m_count);
			return m_memory[i];
		}

		explicit operator StringView() const { return StringView{m_memory.sliceLeft(m_count)}; }
		explicit operator Span<const std::byte>() const { return m_memory.sliceLeft(m_count); }
		explicit operator Span<std::byte>() { return m_memory.sliceLeft(m_count); }

		void push(std::byte b)
		{
			ensureSpaceExists(1);
			m_memory[m_count] = b;
			++m_count;
		}

		void push(StringView v)
		{
			push((const std::byte*)v.data(), v.count());
		}

		void push(Span<const std::byte> v)
		{
			push(v.data(), v.count());
		}

		void push(const Buffer& other)
		{
			push(other.data(), other.count());
		}

		CORE_EXPORT void push(const std::byte* ptr, size_t size);

		CORE_EXPORT void resize(size_t new_count);
		void reserve(size_t extra_count) { ensureSpaceExists(extra_count); }
		CORE_EXPORT void clear();

		size_t count() const { return m_count; }
		size_t capacity() const { return m_memory.count(); }
		std::byte* data() { return m_memory.data(); }
		const std::byte* data() const { return m_memory.data(); }
		Allocator* allocator() const { return m_allocator; }
	};
}