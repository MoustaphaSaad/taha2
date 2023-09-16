#include "core/Buffer.h"
#include "core/String.h"

#include <cstring>

namespace core
{
	void Buffer::destroy()
	{
		if (m_allocator == nullptr || m_ptr == nullptr)
			return;

		m_allocator->release(m_ptr, m_capacity);
		m_allocator->free(m_ptr, m_capacity);
	}

	void Buffer::copyFrom(const core::Buffer& other)
	{
		m_allocator = other.m_allocator;
		m_count = other.m_count;
		m_capacity = m_count;

		m_ptr = (std::byte*)m_allocator->alloc(m_capacity, alignof(std::byte));
		m_allocator->commit(m_ptr, m_capacity);

		::memcpy(m_ptr, other.m_ptr, m_capacity);
	}

	void Buffer::moveFrom(core::Buffer&& other)
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

	void Buffer::grow(size_t new_capacity)
	{
		auto new_ptr = (std::byte*)m_allocator->alloc(new_capacity, alignof(std::byte));
		m_allocator->commit(new_ptr, m_count);

		::memcpy(new_ptr, m_ptr, m_count);

		m_allocator->release(m_ptr, m_capacity);
		m_allocator->free(m_ptr, m_capacity);

		m_ptr = new_ptr;
		m_capacity = new_capacity;
	}

	void Buffer::ensureSpaceExists(size_t count)
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

	Buffer::Buffer(String&& str)
	{
		m_allocator = str.m_allocator;
		m_ptr = (std::byte*)str.m_ptr;
		m_capacity = str.m_capacity;
		m_count = str.m_count;

		str.m_ptr = nullptr;
		str.m_capacity = 0;
		str.m_count = 0;
	}

	void Buffer::push(const std::byte* ptr, size_t size)
	{
		ensureSpaceExists(size);
		::memcpy(m_ptr + m_count, ptr, size);
		m_count += size;
	}

	void Buffer::resize(size_t new_count)
	{
		if (new_count > m_capacity)
		{
			grow(new_count);
			m_allocator->commit(m_ptr + m_count, new_count - m_count);
		}
		else if (new_count < m_capacity)
		{
			m_allocator->release(m_ptr + new_count, m_count - new_count);
		}
		else
		{
			m_allocator->commit(m_ptr + m_count, new_count - m_count);
		}

		m_count = new_count;
	}

	void Buffer::clear()
	{
		m_allocator->release(m_ptr, m_count);
		m_count = 0;
	}
}