#include <doctest/doctest.h>

#include <core/Hash.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::Hash test")
{
	core::Mallocator allocator;
	core::Map<int, int> numbers{&allocator};
	REQUIRE(numbers.count() == 0);
	REQUIRE(numbers.capacity() == 0);

	for (int i = 0; i < 10; ++i)
		numbers.insert(i, i + 1);
	REQUIRE(numbers.count() == 10);

	for (int i = 0; i < 10; ++i)
		REQUIRE(numbers.lookup(i)->value == i + 1);
	REQUIRE(numbers.capacity() >= numbers.count());

	numbers.clear();
	REQUIRE(numbers.count() == 0);

	numbers.reserve(100);
	REQUIRE(numbers.capacity() >= 100);
}
