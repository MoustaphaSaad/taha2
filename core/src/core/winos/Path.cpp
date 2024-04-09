#include "core/Path.h"
#include "core/OSString.h"

#include <Windows.h>

namespace core
{
	Result<String> Path::abs(StringView path, Allocator* allocator)
	{
		if (path == ""_sv)
			path = "."_sv;

		auto osPath = OSString{path, allocator};
		auto requiredSize = GetFullPathName((LPCTSTR)osPath.data(), 0, nullptr, nullptr);
		if (requiredSize == 0)
			return errf(allocator, "GetFullPathName failed, ErrorCode({})"_sv, GetLastError());

		Buffer buffer{allocator};
		buffer.resize(requiredSize * sizeof(TCHAR));
		auto writtenSize = GetFullPathName((LPCTSTR)osPath.data(), requiredSize, (LPTSTR)buffer.data(), nullptr);
		if (writtenSize == 0)
			return errf(allocator, "GetFullPathName failed, ErrorCode({})"_sv, GetLastError());

		auto result = OSString::fromOSEncodedBuffer(std::move(buffer)).toUtf8(allocator);
		return clean(result, allocator);
	}

	Result<String> Path::workingDir(Allocator* allocator)
	{
		auto requiredSize = GetCurrentDirectory(0, nullptr);
		if (requiredSize == 0)
			return errf(allocator, "GetCurrentDirectory failed, ErrorCode({})"_sv, GetLastError());

		Buffer buffer{allocator};
		buffer.resize(requiredSize * sizeof(TCHAR));
		auto writtenSize = GetCurrentDirectory(requiredSize, (LPTSTR)buffer.data());
		if (writtenSize == 0)
			return errf(allocator, "GetCurrentDirectory failed, ErrorCode({})"_sv, GetLastError());

		auto result = OSString::fromOSEncodedBuffer(std::move(buffer)).toUtf8(allocator);
		return clean(result, allocator);
	}
}