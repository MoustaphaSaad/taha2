#pragma once

#include "core/Exports.h"
#include "core/Stream.h"
#include "core/Buffer.h"
#include "core/String.h"

namespace core
{
	class MemoryStream: public Stream
	{
		Buffer m_buffer;
		int64_t m_cursor = 0;
	public:
		explicit MemoryStream(Allocator* allocator)
			: m_buffer(allocator)
		{}

		CORE_EXPORT size_t read(void* buffer, size_t size) override;
		CORE_EXPORT size_t write(const void* buffer, size_t size) override;
		CORE_EXPORT int64_t seek(int64_t offset, Stream::SEEK_MODE whence) override;
		CORE_EXPORT int64_t tell() override;

		CORE_EXPORT String releaseString();
		CORE_EXPORT Buffer releaseBuffer();
	};
}