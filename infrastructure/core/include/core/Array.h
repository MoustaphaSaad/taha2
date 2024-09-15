#pragma once

#include "core/Allocator.h"
#include "core/Assert.h"

#include <new>
#include <type_traits>
#include <utility>

namespace core
{
	template <typename T> class Array
	{
		Allocator* m_allocator = nullptr;
		Span<T> m_memory;
		size_t m_count = 0;

		void destroy()
		{
			if (m_memory.empty())
				return;

			for (size_t i = 0; i < m_count; ++i)
				m_memory[i].~T();
			m_allocator->releaseT(m_memory);
			m_allocator->freeT(m_memory);
		}

		void copyFrom(const Array& other)
		{
			m_allocator = other.m_allocator;
			m_count = other.m_count;
			m_memory = m_allocator->allocT<T>(m_count);
			m_allocator->commitT(m_memory);
			for (size_t i = 0; i < m_count; ++i)
				::new (&m_memory[i]) T(other.m_memory[i]);
		}

		void moveFrom(Array& other)
		{
			m_allocator = other.m_allocator;
			m_memory = other.m_memory;
			m_count = other.m_count;

			other.m_allocator = nullptr;
			other.m_memory = Span<T>{};
			other.m_count = 0;
		}

		void grow(size_t new_capacity)
		{
			auto new_memory = m_allocator->allocT<T>(new_capacity);
			m_allocator->commitT(new_memory.sliceLeft(m_count));
			for (size_t i = 0; i < m_count; ++i)
			{
				if constexpr (std::is_move_constructible_v<T>)
					::new (&new_memory[i]) T(std::move(m_memory[i]));
				else
					::new (&new_memory[i]) T(m_memory[i]);
				m_memory[i].~T();
			}

			m_allocator->releaseT(m_memory);
			m_allocator->freeT(m_memory);

			m_memory = new_memory;
		}

		void ensureSpaceExists(size_t i = 1)
		{
			if (m_count + i > m_memory.count())
			{
				size_t new_capacity = m_memory.count() * 2;
				if (new_capacity == 0)
					new_capacity = 8;

				if (new_capacity < m_memory.count() + i)
					new_capacity = m_memory.count() + i;

				grow(new_capacity);
			}
		}

	public:
		explicit Array(Allocator* a): m_allocator(a) {}

		Array(const Array& other) { copyFrom(other); }

		Array(Array&& other) noexcept { moveFrom(other); }

		Array& operator=(const Array& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Array& operator=(Array&& other) noexcept
		{
			destroy();
			moveFrom(other);
			return *this;
		}

		~Array() { destroy(); }

		T& operator[](size_t i)
		{
			validate(i < m_count);
			return m_memory[i];
		}

		const T& operator[](size_t i) const
		{
			validate(i < m_count);
			return m_memory[i];
		}

		Allocator* allocator() const { return m_allocator; }
		size_t count() const { return m_count; }
		size_t capacity() const { return m_memory.count(); }

		void push(const T& v)
		{
			ensureSpaceExists();
			m_allocator->commitT(m_memory.slice(m_count, m_count + 1));
			::new (&m_memory[m_count]) T(v);
			++m_count;
		}

		void push(T&& v)
		{
			ensureSpaceExists();
			m_allocator->commitT(m_memory.slice(m_count, m_count + 1));
			::new (&m_memory[m_count]) T(std::move(v));
			++m_count;
		}

		template <typename... TArgs> void emplace(TArgs&&... args)
		{
			ensureSpaceExists();
			m_allocator->commitT(m_memory.slice(m_count, m_count + 1));
			::new (&m_memory[m_count]) T(std::forward<TArgs>(args)...);
			++m_count;
		}

		void pop()
		{
			validate(m_count > 0);
			m_memory[m_count - 1].~T();
			m_allocator->releaseT(m_memory.slice(m_count - 1, m_count));
			--m_count;
		}

		void clear()
		{
			for (size_t i = 0; i < m_count; ++i)
				m_memory[i].~T();
			m_allocator->releaseT(m_memory.sliceLeft(m_count));
			m_count = 0;
		}

		void reserve(size_t added_count) { ensureSpaceExists(added_count); }

		void resize(size_t new_count)
		{
			if (new_count > m_count)
			{
				ensureSpaceExists(new_count - m_count);
				m_allocator->commitT(m_memory.slice(m_count, new_count));
				for (; m_count < new_count; ++m_count)
					::new (&m_memory[m_count]) T();
			}
			else if (new_count < m_count)
			{
				for (size_t i = new_count; i < m_count; ++i)
					m_memory[i].~T();
				m_allocator->releaseT(m_memory.slice(new_count, m_count));
				m_count = new_count;
			}
		}

		void resize_fill(size_t new_count, const T& value)
		{
			if (new_count > m_count)
			{
				ensureSpaceExists(new_count - m_count);
				m_allocator->commitT(m_memory.slice(m_count, new_count));
				for (; m_count < new_count; ++m_count)
					::new (&m_memory[m_count]) T(value);
			}
			else
			{
				for (size_t i = new_count; i < m_count; ++i)
					m_memory[i].~T();
				m_allocator->releaseT(m_memory.slice(new_count, m_count));
				m_count = new_count;
			}
		}

		void shrink_to_fit()
		{
			if (m_memory.count() == m_count)
				return;

			auto new_memory = m_allocator->allocT<T>(m_count);
			m_allocator->commitT(new_memory);
			for (size_t i = 0; i < m_count; ++i)
			{
				if constexpr (std::is_move_constructible_v<T>)
					::new (&new_memory[i]) T(std::move(m_memory[i]));
				else
					::new (&new_memory[i]) T(m_memory[i]);
				m_memory[i].~T();
			}

			m_allocator->releaseT(m_memory);
			m_allocator->freeT(m_memory);

			m_memory = new_memory;
		}

		template<typename TFunc>
		void removeIf(TFunc&& func)
		{
			auto beginIt = begin();
			auto endIt = end();
			auto frontIt = beginIt;

			for (auto it = beginIt; it != endIt; ++it)
			{
				if (func(*it) == false)
				{
					*frontIt = *it;
					++frontIt;
				}
			}

			auto removedCount = endIt - frontIt;
			m_count -= removedCount;

			for (auto it = frontIt; it != endIt; ++it)
				it->~T();

			m_allocator->releaseT(m_memory.slice(m_count, m_count + removedCount));
		}

		T* data() { return m_memory.data(); }
		const T* data() const { return m_memory.data(); }

		T* begin() { return m_memory.begin(); }

		const T* begin() const { return m_memory.begin(); }

		T* end() { return m_memory.sliceLeft(m_count).end(); }

		const T* end() const { return m_memory.sliceLeft(m_count).end(); }
	};
}