#include <doctest/doctest.h>

#include <core/File.h>
#include <core/Mallocator.h>
#include <core/MemoryStream.h>

TEST_CASE("core::MemoryStream strf")
{
	core::Mallocator mallocator;
	core::MemoryStream stream{&mallocator};
	core::strf(&stream, "Hello {}!\n"_sv, "يا عالم 🌎");
	REQUIRE(stream.releaseString() == "Hello يا عالم 🌎!\n"_sv);
}
