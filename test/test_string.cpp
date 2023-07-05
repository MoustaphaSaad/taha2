#include <doctest/doctest.h>

#include <core/StringView.h>
#include <core/Mallocator.h>
#include <core/String.h>

TEST_CASE("core::StringView basics")
{
	auto str = "Hello"_sv;
	REQUIRE(str.count() == 5);
	REQUIRE(str.runeCount() == 5);
	REQUIRE(str[0] == 'H');
	REQUIRE(str[1] == 'e');
	REQUIRE(str[2] == 'l');
	REQUIRE(str[3] == 'l');
	REQUIRE(str[4] == 'o');

	str = "مصطفى"_sv;
	REQUIRE(str.count() == 10);
	REQUIRE(str.runeCount() == 5);
}

TEST_CASE("core::StringView find")
{
	REQUIRE("hello world"_sv.find("hello world"_sv) == 0);
	REQUIRE("hello world"_sv.find("hello"_sv) == 0);
	REQUIRE("hello world"_sv.find("hello"_sv, 1) == SIZE_MAX);
	REQUIRE("hello world"_sv.find("world"_sv) == 6);
	REQUIRE("hello world"_sv.find("ld"_sv) == 9);
	REQUIRE("hello world"_sv.find("hello"_sv, 8) == SIZE_MAX);
	REQUIRE("hello world"_sv.find("hello world hello"_sv) == SIZE_MAX);
	REQUIRE("hello world"_sv.find(""_sv) == 0);
	REQUIRE(""_sv.find("hello"_sv) == SIZE_MAX);
}

TEST_CASE("core::StringView findLast")
{
	REQUIRE("hello world"_sv.findLast("hello world"_sv) == 0);
	REQUIRE("hello world"_sv.findLast("hello world"_sv, 0) == SIZE_MAX);
	REQUIRE("hello world"_sv.findLast("world"_sv, 9) == SIZE_MAX);
	REQUIRE("hello world"_sv.findLast("world"_sv) == 6);
	REQUIRE("hello world"_sv.findLast("ld"_sv) == 9);
	REQUIRE("hello world"_sv.findLast("hello"_sv, 8) == 0);
	REQUIRE("hello world"_sv.findLast("world"_sv, 3) == SIZE_MAX);
	REQUIRE("hello world"_sv.findLast("hello world hello"_sv) == SIZE_MAX);
	REQUIRE("hello world"_sv.findLast(""_sv) == 11);
	REQUIRE(""_sv.findLast("hello"_sv) == SIZE_MAX);
}

TEST_CASE("core::StringView::find Rune")
{
	REQUIRE("hello world"_sv.find(core::Rune{'h'}) == 0);
	REQUIRE("hello world"_sv.find(core::Rune{' '}) == 5);
	REQUIRE("hello world"_sv.find(core::Rune{'o'}, 6) == 7);
}

TEST_CASE("core::String creation")
{
	core::Mallocator allocator;

	core::String str{"Hello"_sv, &allocator};
	REQUIRE(str.count() == 5);
	REQUIRE(str.runeCount() == 5);
	REQUIRE(str[0] == 'H');
	REQUIRE(str[1] == 'e');
	REQUIRE(str[2] == 'l');
	REQUIRE(str[3] == 'l');
	REQUIRE(str[4] == 'o');
	REQUIRE(str == "Hello"_sv);

	str = "مصطفى"_sv;
	REQUIRE(str.count() == 10);
	REQUIRE(str.runeCount() == 5);
	REQUIRE(str == "مصطفى"_sv);
}

TEST_CASE("core::String::replace")
{
	core::Mallocator allocator;

	core::String str{"hello world"_sv, &allocator};
	str.replace("world"_sv, "مصطفى"_sv);
	REQUIRE(str == "hello مصطفى"_sv);
}
