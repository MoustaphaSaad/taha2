#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/SHA1.h>

TEST_CASE("basic core::SHA1")
{
	auto sha1 = core::SHA1::hash("Mostafa Saad"_sv);

	core::Mallocator allocator;
	auto str = core::strf(&allocator, "{}"_sv, sha1);
	REQUIRE(str == "4f1f78e998e26bed5854ec0b55140691b039afbf"_sv);
}
