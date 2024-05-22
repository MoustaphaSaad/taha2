#pragma once

#include "core/Stream.h"
#include "core/Buffer.h"

namespace core
{
	class BufferedReader: public Stream
	{
		Stream* m_source = nullptr;
		Buffer m_buffer;
		size_t m_readCursor = 0;
	public:
		BufferedReader(Stream* source, Allocator* allocator)
			: m_source(source),
			  m_buffer(allocator)
		{}

		Span<const std::byte> peek(size_t size)
		{
			auto availableSize = m_buffer.count() - m_readCursor;
			if (availableSize < size)
			{
				auto requiredSize = size - availableSize;
				auto offset = m_buffer.count();
				m_buffer.resize(m_buffer.count() + requiredSize);
				auto readSize = m_source->read(m_buffer.data() + offset, requiredSize);
				m_buffer.resize(offset + readSize);
			}

			availableSize = m_buffer.count() - m_readCursor;
			auto resSize = size;
			if (resSize > availableSize)
				resSize = availableSize;
			return Span<const std::byte>{m_buffer}.slice(m_readCursor, resSize);
		}

		size_t read(void* buffer, size_t size) override
		{
			auto remainingSize = size;
			auto availableSize = m_buffer.count() - m_readCursor;
			if (availableSize > 0)
			{
				auto readSize = size > availableSize ? availableSize : size;
				memcpy(buffer, m_buffer.data() + m_readCursor, readSize);
				remainingSize -= readSize;
				m_readCursor += readSize;
			}

			if (m_readCursor == m_buffer.count())
			{
				m_buffer.clear();
				m_readCursor = 0;
			}

			if (remainingSize > 0)
			{
				auto offset = size - remainingSize;
				auto readSize = m_source->read((std::byte*)buffer + offset, remainingSize);
				remainingSize -= readSize;
			}
			return size - remainingSize;
		}

		size_t write(const void* buffer, size_t size) override
		{
			coreUnreachable();
			return 0;
		}

		int64_t seek(int64_t offset, SEEK_MODE whence) override
		{
			coreUnreachable();
			return 0;
		}

		int64_t tell() override
		{
			coreUnreachable();
			return 0;
		}
	};
}