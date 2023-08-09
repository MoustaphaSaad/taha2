#include "core/File.h"
#include "core/OSString.h"

#include <assert.h>

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

namespace core
{
	class WinOSFile: public File
	{
		HANDLE m_handle = INVALID_HANDLE_VALUE;

		void close()
		{
			if (m_handle != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_handle);
				assert(SUCCEEDED(res));
				m_handle = INVALID_HANDLE_VALUE;
			}
		}
	public:
		WinOSFile(HANDLE handle)
			: m_handle(handle)
		{}

		~WinOSFile() override
		{
			close();
		}

		size_t read(void* buffer, size_t size) override
		{
			DWORD dwNumberOfBytesRead = 0;
			auto res = ReadFile(m_handle, buffer, size, &dwNumberOfBytesRead, nullptr);
			assert(SUCCEEDED(res));
			return dwNumberOfBytesRead;
		}

		size_t write(const void* buffer, size_t size) override
		{
			DWORD dwNumberOfBytesWritten = 0;
			auto res = WriteFile(m_handle, buffer, DWORD(size), &dwNumberOfBytesWritten, nullptr);
			assert(SUCCEEDED(res));
			return dwNumberOfBytesWritten;
		}

		int64_t seek(int64_t offset, SEEK_MODE seek_mode) override
		{
			DWORD dwMoveMethod = 0;
			switch (seek_mode)
			{
			case SEEK_MODE_BEGIN:
				dwMoveMethod = FILE_BEGIN;
				break;

			case SEEK_MODE_CURRENT:
				dwMoveMethod = FILE_CURRENT;
				break;

			case SEEK_MODE_END:
				dwMoveMethod = FILE_END;
				break;
			}

			LARGE_INTEGER liDistanceToMove{};
			liDistanceToMove.QuadPart = offset;
			LARGE_INTEGER liNewFilePointer{};
			auto res = SetFilePointerEx(m_handle, liDistanceToMove, &liNewFilePointer, dwMoveMethod);
			assert(SUCCEEDED(res));
			return liNewFilePointer.QuadPart;
		}

		int64_t tell() override
		{
			LARGE_INTEGER liDistanceToMove{};
			liDistanceToMove.QuadPart = 0;
			LARGE_INTEGER liNewFilePointer{};
			auto res = SetFilePointerEx(m_handle, liDistanceToMove, &liNewFilePointer, FILE_CURRENT);
			assert(SUCCEEDED(res));
			return liNewFilePointer.QuadPart;
		}
	};

	Unique<File> File::open(Allocator* allocator, StringView name, IO_MODE io_mode, OPEN_MODE open_mode, SHARE_MODE share_mode)
	{
		// translate the io mode
		DWORD dwDesiredAccess = 0;
		switch(io_mode)
		{
		case File::IO_MODE_READ:
			dwDesiredAccess = GENERIC_READ;
			break;

		case File::IO_MODE_WRITE:
			dwDesiredAccess = GENERIC_WRITE;
			break;

		case File::IO_MODE_READ_WRITE:
		default:
			dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			break;
		}

		// translate the open mode
		DWORD dwCreationDisposition = 0;
		switch(open_mode)
		{
		case OPEN_MODE_CREATE_ONLY:
			dwCreationDisposition = CREATE_NEW;
			break;

		case OPEN_MODE_OPEN_OVERWRITE:
			dwCreationDisposition = TRUNCATE_EXISTING;
			break;

		case OPEN_MODE_OPEN_ONLY:
		case OPEN_MODE_OPEN_APPEND:
			dwCreationDisposition = OPEN_EXISTING;
			break;

		case OPEN_MODE_CREATE_APPEND:
			dwCreationDisposition = OPEN_ALWAYS;
			break;

		case OPEN_MODE_CREATE_OVERWRITE:
		default:
			dwCreationDisposition = CREATE_ALWAYS;
			break;
		}

		// translate the share mode
		DWORD dwShareMode = 0;
		switch (share_mode)
		{
		case SHARE_MODE_READ:
			dwShareMode = FILE_SHARE_READ;
			break;
		case SHARE_MODE_WRITE:
			dwShareMode = FILE_SHARE_WRITE;
			break;
		case SHARE_MODE_DELETE:
			dwShareMode = FILE_SHARE_DELETE;
			break;
		case SHARE_MODE_READ_WRITE:
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			break;
		case SHARE_MODE_READ_DELETE:
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
			break;
		case SHARE_MODE_WRITE_DELETE:
			dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			break;
		case SHARE_MODE_ALL:
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			break;
		case SHARE_MODE_NONE:
		default:
			dwShareMode = NULL;
			break;
		}

		auto osName = core::OSString{ name, allocator };
		auto handle = CreateFile(
			(LPWSTR)osName.data(),
			dwDesiredAccess,
			dwShareMode,
			NULL,
			dwCreationDisposition,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (handle == INVALID_HANDLE_VALUE)
			return nullptr;

		if (open_mode == OPEN_MODE_CREATE_APPEND ||
			open_mode == OPEN_MODE_OPEN_APPEND)
		{
			SetFilePointer(handle, NULL, NULL, FILE_END);
		}

		return core::unique_from<WinOSFile>(allocator, handle);
	}
}