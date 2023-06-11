#include <doctest/doctest.h>

#include <core/Unique.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::Unique test")
{
	core::Mallocator allocator;
	auto ptr = core::unique_from<int>(&allocator, 42);
	REQUIRE(*ptr == 42);
}
