#include "core/File.h"

#define _LARGEFILE64_SOURCE 1
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace core
{
	class LinuxFile: public File
	{
		int m_handle = -1;

		void close()
		{
			if (m_handle != -1)
			{
				[[maybe_unused]] auto res = ::close(m_handle);
				assert(res == 0);
				m_handle = -1;
			}
		}
	public:
		LinuxFile(int handle)
			: m_handle(handle)
		{}

		~LinuxFile() override
		{
			close();
		}

		size_t read(void* buffer, size_t size) override
		{
			auto res = ::read(m_handle, buffer, size);
			if (res == -1)
				return SIZE_MAX;
			return res;
		}

		size_t write(const void* buffer, size_t size) override
		{
			auto res = ::write(m_handle, buffer, size);
			if (res == -1)
				return SIZE_MAX;
			return res;
		}

		int64_t seek(int64_t offset, SEEK_MODE seek_mode) override
		{
			int whence = 0;
			switch (seek_mode)
			{
			case SEEK_MODE_BEGIN:
				whence = SEEK_SET;
				break;

			case SEEK_MODE_CURRENT:
				whence = SEEK_CUR;
				break;

			case SEEK_MODE_END:
				whence = SEEK_END;
				break;
			}

			return ::lseek64(m_handle, offset, whence);
		}

		int64_t tell() override
		{
			return ::lseek64(m_handle, 0, SEEK_CUR);
		}
	};

	Unique<File> File::open(Allocator* allocator, StringView name, IO_MODE io_mode, OPEN_MODE open_mode, SHARE_MODE share_mode)
	{
		int flags = 0;

		// translate the io mode
		switch(io_mode)
		{
		case File::IO_MODE_READ:
			flags |= O_RDONLY;
			break;

		case File::IO_MODE_WRITE:
			flags |= O_WRONLY;
			break;

		case File::IO_MODE_READ_WRITE:
		default:
			flags |= O_RDWR;
			break;
		}

		// translate the open mode
		switch(open_mode)
		{
		case OPEN_MODE_CREATE_ONLY:
			flags |= O_CREAT;
			flags |= O_EXCL;
			break;

		case OPEN_MODE_CREATE_APPEND:
			flags |= O_CREAT;
			flags |= O_APPEND;
			break;

		case OPEN_MODE_OPEN_ONLY:
			//do nothing
			break;

		case OPEN_MODE_OPEN_OVERWRITE:
			flags |= O_TRUNC;
			break;

		case OPEN_MODE_OPEN_APPEND:
			flags |= O_APPEND;
			break;

		case OPEN_MODE_CREATE_OVERWRITE:
		default:
			flags |= O_CREAT;
			flags |= O_TRUNC;
			break;
		}

		// translate the share mode
		// unix doesn't support the granularity of file sharing like windows so we only support
		// NONE which is available only in O_CREAT mode
		switch (share_mode)
		{
		case SHARE_MODE_NONE:
			if(flags & O_CREAT)
				flags |= O_EXCL;
			break;

		default:
			break;
		}

		auto handle = ::open(name.data(), flags, S_IRWXU);
		if (handle == -1)
			return nullptr;

		return core::unique_from<LinuxFile>(allocator, handle);
	}
}