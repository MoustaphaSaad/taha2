#include <doctest/doctest.h>

#include <core/Array.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::Array test")
{
	core::Mallocator allocator;
	core::Array<int> numbers{&allocator};
	REQUIRE(numbers.count() == 0);
	REQUIRE(numbers.capacity() == 0);

	for (int i = 0; i < 10; ++i)
		numbers.push(i);
	REQUIRE(numbers.count() == 10);

	for (int i = 0; i < 10; ++i)
		REQUIRE(numbers[i] == i);
	REQUIRE(numbers.capacity() >= numbers.count());

	numbers.shrink_to_fit();
	REQUIRE(numbers.capacity() == numbers.count());

	numbers.clear();
	REQUIRE(numbers.count() == 0);

	numbers.reserve(100);
	REQUIRE(numbers.capacity() >= 100);
}

TEST_CASE("test core::Array::removeIf")
{
	core::Mallocator allocator;
	core::Array<int> numbers{&allocator};

	for (int i = 0; i < 10; ++i)
		numbers.push(i);
	REQUIRE(numbers.count() == 10);

	numbers.removeIf([](auto n) { return n % 2 == 0; });
	REQUIRE(numbers.count() == 5);
	REQUIRE(numbers[0] == 1);
	REQUIRE(numbers[1] == 3);
	REQUIRE(numbers[2] == 5);
	REQUIRE(numbers[3] == 7);
	REQUIRE(numbers[4] == 9);
}
