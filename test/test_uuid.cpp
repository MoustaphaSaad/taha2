#include <doctest/doctest.h>

#include <core/UUID.h>
#include <core/Hash.h>
#include <core/String.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::UUID test")
{
	core::Mallocator allocator;
	core::Set<core::UUID> set{&allocator};

	for (size_t i = 0; i < 1000000; ++i)
	{
		auto uuid = core::UUID::generate();
		set.insert(uuid);
	}
	REQUIRE(set.count() == 1000000);
}

TEST_CASE("core::UUID parsing")
{
	core::Mallocator allocator;

	auto id = core::UUID::generate();
	auto str = core::strf(&allocator, "{}"_sv, id);
	auto id2 = core::UUID::parse(str, &allocator);
	REQUIRE(id2.is_error() == false);
	REQUIRE(id == id2.value());

	auto res = core::UUID::parse("this is not an UUID"_sv, &allocator);
	REQUIRE(res.is_error());

	res = core::UUID::parse("62013B88-FA54-4008-8D42-F9CA4889e0B5"_sv, &allocator);
	REQUIRE(res.is_error() == false);

	res = core::UUID::parse("62013BX88-FA54-4008-8D42-F9CA4889e0B5"_sv, &allocator);
	REQUIRE(res.is_error() == true);

	res = core::UUID::parse("62013B88-FA54-4008-8D42-F9CA4889e0B5"_sv, &allocator);
	REQUIRE(res.is_error() == false);

	res = core::UUID::parse("62013B88,FA54-4008-8D42-F9CA4889e0B5"_sv, &allocator);
	REQUIRE(res.is_error() == true);

	res = core::UUID::parse("62013B88-FA54-4008-8D42-F9CA4889e0B5AA"_sv, &allocator);
	REQUIRE(res.is_error() == true);

	auto nil = core::strf(&allocator, "{}"_sv, core::UUID{});
	REQUIRE(nil == "00000000-0000-0000-0000-000000000000"_sv);

	res = core::UUID::parse("00000000-0000-0000-0000-000000000000"_sv, &allocator);
	REQUIRE(res.is_error() == false);
	REQUIRE(res.value() == core::UUID{});
}

