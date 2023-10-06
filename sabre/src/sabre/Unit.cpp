#include "sabre/Unit.h"

#include <core/File.h>

namespace sabre
{
	core::Result<core::Unique<Unit>> Unit::create_from_file(core::Allocator* allocator, core::StringView filepath)
	{
		auto content = core::File::content(allocator, filepath);
		if (content.isError())
			return content.releaseError();

		auto unit = core::unique_from<Unit>(allocator, allocator);
		auto package = core::unique_from<UnitPackage>(allocator, allocator);
		auto file = core::unique_from<UnitFile>(allocator, allocator);

		file->filepath = filepath;
		file->content = content.releaseValue();

		unit->root_file = file.get();
		unit->root_package = package.get();

		package->files.push(std::move(file));
		unit->packages.push(std::move(package));

		return unit;
	}
}