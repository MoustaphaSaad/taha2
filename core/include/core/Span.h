#pragma once

#include <cassert>
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
			assert(index < m_count);
			return m_ptr[index];
		}

		const T& operator[](size_t index) const
		{
			assert(index < m_count);
			return m_ptr[index];
		}

		T* data() { return m_ptr; }

		size_t count() const { return m_count; }
		size_t sizeInBytes() const { return m_count * sizeof(T); }

		Span<T> slice(size_t offset, size_t count)
		{
			assert(offset + count <= m_count);
			return Span<T>(m_ptr + offset, count);
		}

		Span<std::byte> asBytes() const
		{
			return {reinterpret_cast<std::byte*>(m_ptr), m_count * sizeof(T)};
		}
	};
}