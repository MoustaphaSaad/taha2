#include <doctest/doctest.h>

#include <core/StringView.h>

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