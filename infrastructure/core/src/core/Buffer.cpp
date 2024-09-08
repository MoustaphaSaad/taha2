#include "core/Buffer.h"
#include "core/String.h"

#include <cstring>

namespace core
{
	void Buffer::destroy()
	{
		if (m_allocator == nullptr || m_memory.empty())
			return;

		m_allocator->release(m_memory);
		m_allocator->free(m_memory);
	}

	void Buffer::copyFrom(const core::Buffer& other)
	{
		m_allocator = other.m_allocator;
		m_count = other.m_count;

		m_memory = m_allocator->alloc(other.m_memory.count(), alignof(std::byte));
		m_allocator->commit(m_memory);

		::memcpy(m_memory.data(), other.m_memory.data(), m_memory.count());
	}

	void Buffer::moveFrom(core::Buffer&& other)
	{
		m_allocator = other.m_allocator;
		m_memory = other.m_memory;
		m_count = other.m_count;

		other.m_allocator = nullptr;
		other.m_memory = Span<std::byte>{};
		other.m_count = 0;
	}

	void Buffer::grow(size_t new_capacity)
	{
		auto new_memory = m_allocator->alloc(new_capacity, alignof(std::byte));
		m_allocator->commit(new_memory.sliceLeft(m_count));

		::memcpy(new_memory.data(), m_memory.data(), m_count);

		m_allocator->release(m_memory);
		m_allocator->free(m_memory);

		m_memory = new_memory;
	}

	void Buffer::ensureSpaceExists(size_t count)
	{
		if (m_count + count > m_memory.count())
		{
			auto new_capacity = m_memory.count() * 2;
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
		m_memory = Span<std::byte>{(std::byte*)str.m_memory.data(), str.m_memory.count()};
		m_count = str.m_count;

		str.m_memory = Span<char>{};
		str.m_count = 0;
	}

	void Buffer::push(const std::byte* ptr, size_t size)
	{
		ensureSpaceExists(size);
		::memcpy(m_memory.sliceRight(m_count).data(), ptr, size);
		m_count += size;
	}

	void Buffer::resize(size_t new_count)
	{
		if (new_count > m_memory.count())
		{
			grow(new_count);
			m_allocator->commit(m_memory.slice(m_count, new_count));
		}
		else if (new_count < m_memory.count())
		{
			m_allocator->release(m_memory.slice(new_count, m_count));
		}
		else
		{
			m_allocator->commit(m_memory.slice(m_count, new_count));
		}

		m_count = new_count;
	}

	void Buffer::clear()
	{
		m_allocator->release(m_memory.sliceLeft(m_count));
		m_count = 0;
	}
}