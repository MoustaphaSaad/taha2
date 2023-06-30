#include <doctest/doctest.h>

#include <core/StringView.h>

TEST_CASE("basic core::StringView test")
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
