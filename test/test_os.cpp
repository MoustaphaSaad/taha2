#include <doctest/doctest.h>

#include <core/OS.h>
#include <core/Mallocator.h>
#include <core/Log.h>

TEST_CASE("core::OS")
{
	core::Mallocator allocator;
	core::Log log{&allocator};

	log.info("page size = {}"_sv, core::OS::pageSize());
}
