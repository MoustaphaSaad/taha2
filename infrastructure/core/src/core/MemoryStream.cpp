#include "core/MemoryStream.h"
#include "core/Assert.h"

namespace core
{
	size_t MemoryStream::read(void* buffer, size_t size)
	{
		validate(m_cursor >= 0);
		validate((size_t)m_cursor <= m_buffer.count());
		auto remaining_size = m_buffer.count() - m_cursor;
		auto read_size = size < remaining_size ? size : remaining_size;
		::memcpy(buffer, m_buffer.data() + m_cursor, read_size);
		m_cursor += (int64_t)read_size;
		return read_size;
	}

	size_t MemoryStream::write(const void* buffer, size_t size)
	{
		validate(m_cursor >= 0);
		m_buffer.resize(m_cursor + size);
		memcpy(m_buffer.data() + m_cursor, buffer, size);
		m_cursor += (int64_t)size;
		return size;
	}

	int64_t MemoryStream::seek(int64_t offset, Stream::SEEK_MODE whence)
	{
		switch (whence)
		{
		case SEEK_MODE_BEGIN:
			m_cursor = offset;
			break;
		case SEEK_MODE_CURRENT:
			m_cursor += offset;
			break;
		case SEEK_MODE_END:
			m_cursor = (int64_t)m_buffer.count() - offset;
			break;
		}

		if (m_cursor < 0)
		{
			m_cursor = 0;
		}
		if (m_cursor > (int64_t)m_buffer.count())
		{
			m_cursor = (int64_t)m_buffer.count();
		}
		return m_cursor;
	}

	int64_t MemoryStream::tell()
	{
		return m_cursor;
	}

	String MemoryStream::releaseString()
	{
		static_assert(sizeof(char) == sizeof(std::byte));

		return String{releaseBuffer()};
	}

	Buffer MemoryStream::releaseBuffer()
	{
		auto res = std::move(m_buffer);
		m_buffer = Buffer{res.allocator()};
		m_cursor = 0;
		return res;
	}
}