#include <doctest/doctest.h>

#include <core/FastLeak.h>
#include <core/Mallocator.h>
#include <core/VirtualMem.h>

TEST_CASE("basic core::Mallocator test")
{
	core::Mallocator allocator;
	auto ptr = (int*)allocator.alloc(sizeof(int), alignof(int)).data();
	REQUIRE(ptr != nullptr);
	allocator.commit(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
	*ptr = 42;
	REQUIRE(*ptr == 42);
	allocator.release(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
	allocator.free(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
}

TEST_CASE("basic core::VirtualMem test")
{
	core::VirtualMem allocator;
	auto ptr = (int*)allocator.alloc(sizeof(int), alignof(int)).data();
	REQUIRE(ptr != nullptr);
	allocator.commit(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
	*ptr = 42;
	REQUIRE(*ptr == 42);
	allocator.release(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
	allocator.free(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
}

TEST_CASE("basic core::FastLeak test")
{
	core::FastLeak allocator;
	auto ptr = (int*)allocator.alloc(sizeof(int), alignof(int)).data();
	REQUIRE(ptr != nullptr);
	allocator.commit(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
	*ptr = 42;
	REQUIRE(*ptr == 42);
	allocator.release(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
	allocator.free(core::Span<std::byte>{(std::byte*)ptr, sizeof(*ptr)});
}
