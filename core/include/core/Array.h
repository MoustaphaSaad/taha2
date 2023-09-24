#pragma once

#include "core/Allocator.h"

#include <type_traits>
#include <new>
#include <cassert>
#include <utility>

namespace core
{
	template<typename T>
	class Array
	{
		Allocator* m_allocator = nullptr;
		T* m_ptr = nullptr;
		size_t m_capacity = 0;
		size_t m_count = 0;

		void destroy()
		{
			for (size_t i = 0; i < m_count; ++i)
				m_ptr[i].~T();
			m_allocator->release(m_ptr, m_capacity * sizeof(T));
			m_allocator->free(m_ptr, m_capacity * sizeof(T));
		}

		void copyFrom(const Array& other)
		{
			m_allocator = other.m_allocator;
			m_count = other.m_count;
			m_capacity = m_count;
			m_ptr = (T*)m_allocator->alloc(sizeof(T) * m_capacity, alignof(T));
			m_allocator->commit(m_ptr, sizeof(T) * m_capacity);
			for (size_t i = 0; i < m_count; ++i)
				::new (m_ptr + i) T(other.m_ptr[i]);
		}

		void moveFrom(Array&& other)
		{
			m_allocator = other.m_allocator;
			m_ptr = other.m_ptr;
			m_count = other.m_count;
			m_capacity = other.m_capacity;

			other.m_allocator = nullptr;
			other.m_ptr = nullptr;
			other.m_count = 0;
			other.m_capacity = 0;
		}

		void grow(size_t new_capacity)
		{
			auto new_ptr = (T*)m_allocator->alloc(sizeof(T) * new_capacity, alignof(T));
			m_allocator->commit(new_ptr, sizeof(T) * m_count);
			for (size_t i = 0; i < m_count; ++i)
			{
				if constexpr (std::is_move_constructible_v<T>)
					::new (new_ptr + i) T(std::move(m_ptr[i]));
				else
					::new (new_ptr + i) T(m_ptr[i]);
				m_ptr[i].~T();
			}

			m_allocator->release(m_ptr, sizeof(T) * m_capacity);
			m_allocator->free(m_ptr, sizeof(T) * m_capacity);

			m_ptr = new_ptr;
			m_capacity = new_capacity;
		}

		void ensureSpaceExists(size_t i = 1)
		{
			if (m_count + i > m_capacity)
			{
				size_t new_capacity = m_capacity * 2;
				if (new_capacity == 0)
					new_capacity = 8;

				if (new_capacity < m_capacity + i)
					new_capacity = m_capacity + i;

				grow(new_capacity);
			}
		}

	public:
		explicit Array(Allocator* a)
			: m_allocator(a)
		{}

		Array(const Array& other)
		{
			copyFrom(other);
		}

		Array(Array&& other)
		{
			moveFrom(std::move(other));
		}

		Array& operator=(const Array& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Array& operator=(Array&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~Array()
		{
			destroy();
		}

		T& operator[](size_t i)
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		const T& operator[](size_t i) const
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		Allocator* allocator() const { return m_allocator; }
		size_t count() const { return m_count; }
		size_t capacity() const { return m_capacity; }

		void push(const T& v)
		{
			ensureSpaceExists();
			m_allocator->commit(m_ptr + m_count, sizeof(T));
			::new (m_ptr + m_count) T(v);
			++m_count;
		}

		void push(T&& v)
		{
			ensureSpaceExists();
			m_allocator->commit(m_ptr + m_count, sizeof(T));
			::new (m_ptr + m_count) T(std::move(v));
			++m_count;
		}

		template<typename ... TArgs>
		void emplace(TArgs&& ... args)
		{
			ensureSpaceExists();
			m_allocator->commit(m_ptr + m_count, sizeof(T));
			::new (m_ptr + m_count) T(std::forward<TArgs>(args)...);
			++m_count;
		}

		void clear()
		{
			for (size_t i = 0; i < m_count; ++i)
				m_ptr[i].~T();
			m_allocator->release(m_ptr, sizeof(T) * m_count);
			m_count = 0;
		}

		void reserve(size_t added_count)
		{
			ensureSpaceExists(added_count);
		}

		void resize(size_t new_count)
		{
			if (new_count > m_count)
			{
				ensureSpaceExists(new_count - m_count);
				m_allocator->commit(m_ptr + m_count, (new_count - m_count) * sizeof(T));
				for (;m_count < new_count; ++m_count)
					::new (m_ptr + m_count) T();
			}
			else if (new_count < m_count)
			{
				for (size_t i = new_count; i < m_count; ++i)
					m_ptr[i].~T();
				m_allocator->release(m_ptr + new_count, (m_count - new_count) * sizeof(T));
				m_count = new_count;
			}
		}

		void resize_fill(size_t new_count, const T& value)
		{
			if (new_count > m_count)
			{
				ensureSpaceExists(new_count - m_count);
				m_allocator->commit(m_ptr + m_count, (new_count - m_count) * sizeof(T));
				for (;m_count < new_count; ++m_count)
					::new (m_ptr + m_count) T(value);
			}
			else
			{
				for (size_t i = new_count; i < m_count; ++i)
					m_ptr[i].~T();
				m_allocator->release(m_ptr + new_count, (m_count - new_count) * sizeof(T));
				m_count = new_count;
			}
		}

		void shrink_to_fit()
		{
			if (m_capacity == m_count)
				return;

			auto new_ptr = (T*)m_allocator->alloc(sizeof(T) * m_count, alignof(T));
			m_allocator->commit(new_ptr, sizeof(T) * m_count);
			for (size_t i = 0; i < m_count; ++i)
			{
				if constexpr (std::is_move_constructible_v<T>)
					::new (new_ptr + i) T(std::move(m_ptr[i]));
				else
					::new (new_ptr + i) T(m_ptr[i]);
				m_ptr[i].~T();
			}

			m_allocator->release(m_ptr, sizeof(T) * m_capacity);
			m_allocator->free(m_ptr, sizeof(T) * m_capacity);

			m_capacity = m_count;
			m_ptr = new_ptr;
		}

		T* begin()
		{
			return m_ptr;
		}

		const T* begin() const
		{
			return m_ptr;
		}

		T* end()
		{
			return m_ptr + m_count;
		}

		const T* end() const
		{
			return m_ptr + m_count;
		}
	};
}