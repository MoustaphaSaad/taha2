#include <doctest/doctest.h>

#include <core/Buffer.h>
#include <core/Mallocator.h>
#include <core/StringView.h>

TEST_CASE("core::Buffer basics")
{
	auto str = "Hello"_sv;

	core::Mallocator mallocator;
	core::Buffer buffer{&mallocator};
	buffer.push(str);

	REQUIRE(buffer.count() == 5);
	REQUIRE(buffer[0] == std::byte{'H'});
	REQUIRE(buffer[1] == std::byte{'e'});
	REQUIRE(buffer[2] == std::byte{'l'});
	REQUIRE(buffer[3] == std::byte{'l'});
	REQUIRE(buffer[4] == std::byte{'o'});
}
