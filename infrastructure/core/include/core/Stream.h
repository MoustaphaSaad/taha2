#pragma once

#include "core/Exports.h"
#include "core/StringView.h"

#include <cstdint>
#include <cstddef>

namespace core
{
	class Stream
	{
	public:

		enum SEEK_MODE
		{
			// seek from the beginning of the file
			SEEK_MODE_BEGIN,
			// seek from the current position
			SEEK_MODE_CURRENT,
			// seek from the end of the file
			SEEK_MODE_END,
		};

		virtual ~Stream() = default;
		virtual size_t read(void* buffer, size_t size) = 0;
		virtual size_t write(const void* buffer, size_t size) = 0;
		virtual int64_t seek(int64_t offset, SEEK_MODE whence) = 0;
		virtual int64_t tell() = 0;
	};

	template<typename ... Args>
	inline void strf(Stream* stream, StringView format, Args&& ... args)
	{
		auto out = fmt::memory_buffer();
		fmt::format_to(std::back_inserter(out), fmt::runtime(std::string_view{format.begin(), format.count()}), std::forward<Args>(args)...);
		stream->write(out.data(), out.size());
	}
}