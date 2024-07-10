#include <doctest/doctest.h>

#include <core/Path.h>
#include <core/Mallocator.h>
#include <core/Log.h>

TEST_CASE("core::Path::tmpDir")
{
	core::Mallocator allocator;
	core::Log log{&allocator};

	auto tmpResult = core::Path::tmpDir(&allocator);
	REQUIRE(!tmpResult.isError());
	auto tmp = tmpResult.releaseValue();

	log.info("temp path: {}"_sv, tmp);
}

TEST_CASE("core::Path::join")
{
	core::Mallocator allocator;

	SUBCASE("no separator at end of dir join")
	{
		auto res = core::Path::join(&allocator, "C:/ABC"_sv, "def.txt"_sv);
		REQUIRE(res == "C:/ABC/def.txt"_sv);
	}

	SUBCASE("linux absolute path")
	{
		auto res = core::Path::join(&allocator, "/ABC"_sv, "def.txt"_sv);
		REQUIRE(res == "/ABC/def.txt"_sv);
	}
}