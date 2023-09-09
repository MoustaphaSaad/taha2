#pragma once

#include <core/String.h>
#include <core/Array.h>
#include <core/Unique.h>

namespace sabre
{
	struct UnitPackage;
	struct Unit;

	struct UnitFile
	{
		// the package which this file belongs to
		UnitPackage* parent = nullptr;
		// absolute path of the file on disk
		core::String absolute_path;
		// the path of the file as supplied by the user
		core::String path;
		// content of the file
		core::String content;
	};

	struct UnitPackage
	{
		// parent compilation unit which this package belongs to
		Unit* parent = nullptr;
		// absolute path of the package on disk
		core::String absolute_path;
		// files of this package
		core::Array<core::Unique<UnitFile>> files;
	};

	struct Unit
	{
		// list of all the packages that's included in this compilation unit including the main package
		core::Array<core::Unique<UnitPackage>> packages;
		// root package is the first package added to the compilation unit, this is the main package provided by user
		UnitPackage* root_package = nullptr;
		// root file is the first file added to the compilation unit, this is the main file provided by the user
		UnitFile* root_file = nullptr;
	};
}