#pragma once

#include <core/String.h>
#include <core/Array.h>
#include <core/Unique.h>
#include <core/Result.h>

namespace sabre
{
	struct UnitPackage;
	struct Unit;

	struct UnitFile
	{
		// the package which this file belongs to
		UnitPackage* parent = nullptr;
		// the path of the file as supplied by the user
		core::String filepath;
		// content of the file
		core::String content;

		explicit UnitFile(core::Allocator* a)
			: filepath(a),
			  content(a)
		{}
	};

	struct UnitPackage
	{
		// parent compilation unit which this package belongs to
		Unit* parent = nullptr;
		// files of this package
		core::Array<core::Unique<UnitFile>> files;

		explicit UnitPackage(core::Allocator* a)
			: files(a)
		{}
	};

	struct Unit
	{
		// list of all the packages that's included in this compilation unit including the main package
		core::Array<core::Unique<UnitPackage>> packages;
		// root package is the first package added to the compilation unit, this is the main package provided by user
		UnitPackage* root_package = nullptr;
		// root file is the first file added to the compilation unit, this is the main file provided by the user
		UnitFile* root_file = nullptr;

		explicit Unit(core::Allocator* a)
			: packages(a)
		{}

		static core::Result<core::Unique<Unit>> create_from_file(core::Allocator* allocator, core::StringView filepath);
	};
}