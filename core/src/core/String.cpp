#include "core/String.h"

#include <utf8proc.h>

namespace core
{
	void String::destroy()
	{
		if (m_allocator == nullptr || m_ptr == nullptr)
			return;

		m_allocator->release(m_ptr, m_capacity);
		m_allocator->free(m_ptr, m_capacity);
	}

	void String::copyFrom(const String& other)
	{
		m_allocator = other.m_allocator;
		m_count = other.m_count;
		m_capacity = m_count;

		m_ptr = (char*)m_allocator->alloc(m_capacity, alignof(char));
		m_allocator->commit(m_ptr, m_capacity);

		::memcpy(m_ptr, other.m_ptr, m_count);
	}

	void String::moveFrom(String&& other)
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

	void String::grow(size_t new_capacity)
	{
		auto new_ptr = (char*)m_allocator->alloc(new_capacity, alignof(char));
		m_allocator->commit(new_ptr, m_count);

		::memcpy(new_ptr, m_ptr, m_count);

		m_allocator->release(m_ptr, m_capacity);
		m_allocator->free(m_ptr, m_capacity);

		m_ptr = new_ptr;
		m_capacity = new_capacity;
	}

	void String::ensureSpaceExists(size_t count)
	{
		if (m_count + count > m_capacity)
		{
			auto new_capacity = m_capacity * 2;
			if (new_capacity == 0)
				new_capacity = 8;

			if (new_capacity < m_count + count)
				new_capacity = m_count + count;

			grow(new_capacity);
		}
	}

	void String::resize(size_t new_count)
	{
		if (new_count > m_capacity)
			grow(new_count);

		m_count = new_count;
		m_ptr[m_count] = '\0';
	}

	String::String(const char* ptr, Allocator* allocator)
		: m_allocator(allocator)
	{
		if (ptr != nullptr)
		{
			m_count = ::strlen(ptr);
			m_capacity = m_count;

			m_ptr = (char*)m_allocator->alloc(m_capacity, alignof(char));
			m_allocator->commit(m_ptr, m_capacity);

			::memcpy(m_ptr, ptr, m_count);
		}
	}

	String::String(const char* begin, const char* end, Allocator* allocator)
		: m_allocator(allocator)
	{
		if (begin != nullptr && end != nullptr)
		{
			m_count = end - begin;
			m_capacity = m_count;

			m_ptr = (char*)m_allocator->alloc(m_capacity, alignof(char));
			m_allocator->commit(m_ptr, m_capacity);

			::memcpy(m_ptr, begin, m_count);
		}
	}

	void String::push(const char* begin, const char* end)
	{
		auto len = m_count;
		resize(m_count + (end - begin + 1));
		--m_count;
		::memcpy(m_ptr + len, begin, end - begin);
		m_ptr[m_count] = '\0';
	}

	void String::push(Rune r)
	{
		auto old_count = m_count;
		// +5 = 4 for the rune + 1 for the null termination
		ensureSpaceExists(m_count + 5);

		auto width = Rune::encode(r, m_ptr + m_count);
		assert(width > 0 && width <= 4);
		m_count -= (4 - width) + 1;
		m_ptr[m_count] = '\0';
	}
}