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
}