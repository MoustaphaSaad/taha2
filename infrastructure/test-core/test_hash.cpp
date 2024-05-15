#include <doctest/doctest.h>

#include <core/Hash.h>
#include <core/Mallocator.h>
#include <core/Unique.h>
#include <core/Shared.h>

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

TEST_CASE("basic core::Set test with core::Unique")
{
	core::Mallocator allocator;
	core::Set<core::Unique<int>> numbers{&allocator};
	numbers.insert(core::unique_from<int>(&allocator, 1));
}

TEST_CASE("basic core::Map test with core::Unique")
{
	core::Mallocator allocator;
	core::Map<int, core::Unique<int>> numbers{&allocator};
	numbers.insert(1, core::unique_from<int>(&allocator, 1));
	numbers.remove(1);
}

TEST_CASE("basic core::Set test with core::Shared")
{
	core::Mallocator allocator;
	core::Set<core::Shared<int>> numbers{&allocator};
	auto val = core::shared_from<int>(&allocator, 1);
	numbers.insert(val);
	numbers.remove(val);
}

TEST_CASE("basic core::Map test with core::Shared")
{
	core::Mallocator allocator;
	core::Map<int, core::Shared<int>> numbers{&allocator};
	numbers.insert(1, core::shared_from<int>(&allocator, 1));
	numbers.remove(1);
}

TEST_CASE("basic core::Set test with core::Weak")
{
	core::Mallocator allocator;
	core::Set<core::Weak<int>> numbers{&allocator};
	auto val = core::shared_from<int>(&allocator, 1);
	numbers.insert(val);
	numbers.remove(val);
}
