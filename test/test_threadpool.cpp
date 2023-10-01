#include <doctest/doctest.h>

#include <core/ThreadPool.h>
#include <core/Mallocator.h>

TEST_CASE("core::ThreadPool basics")
{
	core::Mallocator allocator;

	std::atomic<int> count = 0;

	{
		core::ThreadPool pool{&allocator};

		for (size_t i = 0; i < 1000; ++i)
		{
			pool.run([&]() {
				count += 1;
			});
		}
	}

	REQUIRE(count == 1000);
}
