#include <doctest/doctest.h>

#include <core/Thread.h>
#include <core/Mallocator.h>

TEST_CASE("core::Thread basics")
{
	core::Mallocator allocator;

	int count = 0;
	core::Thread thread{&allocator, [&]() {
		count = 1;
	}};
	thread.join();
	REQUIRE(count == 1);
}
