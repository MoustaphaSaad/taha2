#include <doctest/doctest.h>

#include <core/Shared.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::Unique test")
{
	core::Mallocator allocator;
	core::Shared<int> ptr;
	REQUIRE(ptr.ref_count() == 0);

	ptr = core::shared_from<int>(&allocator, 42);
	REQUIRE(*ptr == 42);
	REQUIRE(ptr.ref_count() == 1);

	core::Weak<int> weak_ptr = ptr;
	REQUIRE(weak_ptr.ref_count() == 1);

	auto ptr2 = weak_ptr.lock();
	REQUIRE(*ptr2 == 42);
	REQUIRE(ptr2.ref_count() == 2);

	ptr = nullptr;
	REQUIRE(ptr2.ref_count() == 1);
	ptr2 = nullptr;
	REQUIRE(weak_ptr.ref_count() == 0);
	REQUIRE(weak_ptr.expired() == true);
}
