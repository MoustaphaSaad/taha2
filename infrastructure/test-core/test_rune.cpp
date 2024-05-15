#include <doctest/doctest.h>

#include <core/Rune.h>

TEST_CASE("basic core::Rune test")
{
	core::Rune a{'a'};
	REQUIRE(a.isLetter());
	auto A = a.upper();
	REQUIRE(A == 'A');
	REQUIRE(A != a);

	core::Rune meem{U'\u0645'};
	REQUIRE(meem.isLetter());
	REQUIRE(meem.lower() == meem);
}
