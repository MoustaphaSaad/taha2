#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/Queue.h>

TEST_CASE("basic core::Queue test")
{
	core::Mallocator allocator;

	core::Queue<int> queue{ &allocator };
	REQUIRE(queue.count() == 0);

	queue.push_back(1);
	REQUIRE(queue.count() == 1);
	REQUIRE(queue.back() == 1);
	REQUIRE(queue.front() == 1);

	queue.pop_back();
	REQUIRE(queue.count() == 0);
}

TEST_CASE("core::Queue numbers")
{
	core::Mallocator allocator;

	core::Queue<int> queue{ &allocator };

	for (int i = 0; i < 100; ++i)
		queue.push_back(i);

	for (int i = 0; i < 100; ++i)
	{
		REQUIRE(queue.front() == i);
		queue.pop_front();
	}
}
