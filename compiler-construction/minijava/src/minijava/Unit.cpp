#include "minijava/Unit.h"

#include <core/File.h>
#include <core/Path.h>

namespace minijava
{
	core::Result<core::Unique<Unit>> Unit::create(core::StringView path, core::Allocator* allocator)
	{
		auto absolutePathResult = core::Path::abs(path, allocator);
		if (absolutePathResult.isError())
			return absolutePathResult.releaseError();

		auto contentResult = core::File::content(allocator, path);
		if (contentResult.isError())
			return contentResult.releaseError();

		auto filePath = core::String{path, allocator};

		return core::unique_from<Unit>(allocator, std::move(filePath), absolutePathResult.releaseValue(), contentResult.releaseValue(), allocator);
	}
}