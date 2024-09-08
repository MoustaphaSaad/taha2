#pragma once

#include "core/Assert.h"

#include <cstddef>

namespace core
{
	template<typename T>
	class Span
	{
		T* m_ptr = nullptr;
		size_t m_count = 0;
	public:
		Span() = default;
		Span(T* ptr, size_t size)
			: m_ptr(ptr)
			, m_count(size)
		{}

		operator Span<const T>() const { return Span<const T>{m_ptr, m_count}; }

		T* begin() { return m_ptr; }
		const T* begin() const { return m_ptr; }

		T* end() { return m_ptr + m_count; }
		const T* end() const { return m_ptr + m_count; }

		T& front() { return *m_ptr; }
		const T& front() const { return *m_ptr; }

		T& back() { return *(m_ptr + m_count - 1); }
		const T& back() const { return *(m_ptr + m_count - 1); }

		T& operator[](size_t index)
		{
			coreAssert(index < m_count);
			return m_ptr[index];
		}

		const T& operator[](size_t index) const
		{
			coreAssert(index < m_count);
			return m_ptr[index];
		}

		T* data() { return m_ptr; }
		const T* data() const { return m_ptr; }

		size_t count() const { return m_count; }
		size_t sizeInBytes() const { return m_count * sizeof(T); }

		bool empty() const { return m_ptr == nullptr || m_count == 0; }

		Span<T> slice(size_t start, size_t end) const
		{
			coreAssert(start <= end && end - start <= m_count);
			return Span<T>(m_ptr + start, end - start);
		}

		Span<T> sliceRight(size_t start) const
		{
			return slice(start, count());
		}

		Span<T> sliceLeft(size_t start) const
		{
			return slice(0, start);
		}

		Span<const std::byte> asBytes() const
		{
			return {(const std::byte*)(m_ptr), m_count * sizeof(T)};
		}

		Span<std::byte> asBytes()
		{
			return {(std::byte*)(m_ptr), m_count * sizeof(T)};
		}
	};
}