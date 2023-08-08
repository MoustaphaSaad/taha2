#include "core/File.h"

#define _LARGEFILE64_SOURCE 1
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace core
{
	class MacOSFile: public File
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
		MacOSFile(int handle)
			: m_handle(handle)
		{}

		~MacOSFile() override
		{
			close();
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

		return core::unique_from<MacOSFile>(allocator, handle);
	}
}