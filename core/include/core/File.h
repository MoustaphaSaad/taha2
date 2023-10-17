#pragma once

#include "core/Exports.h"
#include "core/Stream.h"
#include "core/Unique.h"
#include "core/StringView.h"
#include "core/Allocator.h"
#include "core/String.h"
#include "core/Result.h"

#include <fmt/core.h>
#include <fmt/format.h>

namespace core
{
	class File: public Stream
	{
	public:
		enum OPEN_MODE
		{
			// creates the file if it doesn't exist, if it exists the function will fail
			OPEN_MODE_CREATE_ONLY,
			// creates the file if it doesn't exist, if it exists it will be overwritten
			OPEN_MODE_CREATE_OVERWRITE,
			// creates the file if it doesn't exist, if it exists it will be append to its end
			OPEN_MODE_CREATE_APPEND,
			// opens the file if it exists, fails otherwise
			OPEN_MODE_OPEN_ONLY,
			// opens the file if it exists and its content will be overwritten, if it doesn't exist the function will fail
			OPEN_MODE_OPEN_OVERWRITE,
			// opens the file if it exists and new writes will append to the file, if it doesn't exist the function will fail
			OPEN_MODE_OPEN_APPEND,
		};

		enum SHARE_MODE
		{
			// enables subsequent open operations on a file or device to request read access
			SHARE_MODE_READ,
			// enables subsequent open operations on a file or device to request write access
			SHARE_MODE_WRITE,
			// enables subsequent open operations on a file or device to request delete access
			SHARE_MODE_DELETE,
			// combines the read write options from above
			SHARE_MODE_READ_WRITE,
			// combines the read delete options from above
			SHARE_MODE_READ_DELETE,
			// combines the write delete options from above
			SHARE_MODE_WRITE_DELETE,
			// combines all the options
			SHARE_MODE_ALL,
			// no sharing is allowed so any subsequent opens will fail
			SHARE_MODE_NONE,
		};

		enum IO_MODE
		{
			// only read operations are allowed
			IO_MODE_READ,
			// only write operations are allowed
			IO_MODE_WRITE,
			// read and write operations are allowed
			IO_MODE_READ_WRITE,
		};

		CORE_EXPORT static Unique<File> open(Allocator* allocator, StringView name, IO_MODE io_mode, OPEN_MODE open_mode, SHARE_MODE share_mode = SHARE_MODE_ALL);
		CORE_EXPORT static Result<String> content(Allocator* allocator, StringView name);
		CORE_EXPORT static File* STDOUT;
		CORE_EXPORT static File* STDERR;
		CORE_EXPORT static File* STDIN;

		int64_t size()
		{
			auto cursor = tell();
			auto size = seek(0, Stream::SEEK_MODE_END);
			seek(cursor, Stream::SEEK_MODE_BEGIN);
			return size;
		}
	};
}