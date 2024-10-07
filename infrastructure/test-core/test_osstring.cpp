#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/OSString.h>

TEST_CASE("core::OSString roundtrip")
{
	core::Mallocator allocator;

	auto str = "Hello يا عالم 🌎!"_sv;
	auto osStr = core::OSString(str, &allocator);
	auto utf8 = osStr.toUtf8(&allocator);
	REQUIRE(utf8 == str);
}
