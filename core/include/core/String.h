#pragma once

#include "core/Allocator.h"

#include <cstring>
#include <cassert>
#include <utility>

namespace core
{
	class String
	{
		Allocator* m_allocator = nullptr;
		char* m_ptr = nullptr;
		size_t m_capacity = 0;
		size_t m_count = 0;

		void destroy();

		void copyFrom(const String& other);

		void moveFrom(String&& other);

		void grow(size_t new_capacity);

	public:
		explicit String(Allocator* allocator)
			: m_allocator(allocator)
		{}

		String(const char* ptr, Allocator* allocator);
		String(const char* begin, const char* end, Allocator* allocator);

		String(const String& other)
		{
			copyFrom(other);
		}

		String(String&& other)
		{
			moveFrom(std::move(other));
		}

		String& operator=(const String& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		String& operator=(String&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~String()
		{
			destroy();
		}

		char& operator[](size_t i)
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		const char& operator[](size_t i) const
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		size_t count() const { return m_count; }
		size_t capacity() const { return m_capacity; }
	};
}