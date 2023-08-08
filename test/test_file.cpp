#include <doctest/doctest.h>

#include <core/File.h>
#include <core/Mallocator.h>

TEST_CASE("core::File roundtrip")
{
	core::Mallocator allocator;

	auto file = core::File::open(&allocator, "test.txt"_sv, core::File::IO_MODE_READ_WRITE, core::File::OPEN_MODE_CREATE_OVERWRITE);
	REQUIRE(file != nullptr);
}
