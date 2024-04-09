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
