#include "core/Path.h"
#include "core/OSString.h"

#include <unistd.h>
#include <stdlib.h>

namespace core
{
	Result<String> Path::abs(StringView path, Allocator* allocator)
	{
		if (path.startsWith("/"_sv))
			return clean(path, allocator);

		auto workingDirResult = workingDir(allocator);
		if (workingDirResult.isError())
			return workingDirResult.releaseError();

		return join(allocator, workingDirResult.releaseValue(), path);
	}

	Result<String> Path::workingDir(Allocator* allocator)
	{
		String result{allocator};
		result.resize(PATH_MAX + 1);
		auto res = getcwd(result.data(), result.count());
		if (res == nullptr)
			return errf(allocator, "getcwd failed, ErrorCode({})"_sv, errno);
		result.resize(strlen(result.data()));
		return result;
	}

	Result<String> Path::tmpDir(Allocator* allocator)
	{
		if (auto p = secure_getenv("TMPDIR"))
			return String{StringView{p}, allocator};
		else if (auto p = secure_getenv("TMP"))
			return String{StringView{p}, allocator};
		else if (auto p = secure_getenv("TEMP"))
			return String{StringView{p}, allocator};
		else if (auto p = secure_getenv("TEMPDIR"))
			return String{StringView{p}, allocator};
		else
			return String{"/tmp"_sv, allocator};
	}
}