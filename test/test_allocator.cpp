#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/VirtualMem.h>
#include <core/FastLeak.h>

TEST_CASE("basic core::Mallocator test")
{
	core::Mallocator allocator;
	auto ptr = (int*)allocator.alloc(sizeof(int), alignof(int));
	REQUIRE(ptr != nullptr);
	allocator.commit(ptr, sizeof(*ptr));
	*ptr = 42;
	REQUIRE(*ptr == 42);
	allocator.release(ptr, sizeof(*ptr));
	allocator.free(ptr, sizeof(*ptr));
}

TEST_CASE("basic core::VirtualMem test")
{
	core::VirtualMem allocator;
	auto ptr = (int*)allocator.alloc(sizeof(int), alignof(int));
	REQUIRE(ptr != nullptr);
	allocator.commit(ptr, sizeof(*ptr));
	*ptr = 42;
	REQUIRE(*ptr == 42);
	allocator.release(ptr, sizeof(*ptr));
	allocator.free(ptr, sizeof(*ptr));
}

TEST_CASE("basic core::FastLeak test")
{
	core::FastLeak allocator;
	auto ptr = (int*)allocator.alloc(sizeof(int), alignof(int));
	REQUIRE(ptr != nullptr);
	allocator.commit(ptr, sizeof(*ptr));
	*ptr = 42;
	REQUIRE(*ptr == 42);
	allocator.release(ptr, sizeof(*ptr));
	allocator.free(ptr, sizeof(*ptr));
}
