#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/Func.h>

TEST_CASE("core::Func basics")
{
	core::Mallocator allocator;
	core::Func<int(int, int)> func;
	REQUIRE(!func);

	func = [](int a, int b) { return a + b; };
	REQUIRE(func);

	REQUIRE(func(1, 23) == 24);

	auto func2 = std::move(func);
	REQUIRE(!func);
	REQUIRE(func2);
	REQUIRE(func2(1, 24) == 25);
}

TEST_CASE("core::Func heap")
{
	core::Mallocator allocator;
	core::Func<int64_t()> func;
	REQUIRE(!func);

	struct Big
	{
		int64_t a, b, c, d, e;
	};

	Big big{1, 2, 3, 4, 5};
	func = core::Func<int64_t()>{&allocator, [big]() { return big.a + big.b + big.c + big.d + big.e; }};
	REQUIRE(func);

	REQUIRE(func() == 15);

	auto func2 = std::move(func);
	REQUIRE(func2);
	REQUIRE(func2() == 15);
}