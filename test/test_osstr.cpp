#include <doctest/doctest.h>

#include <core/OSStr.h>
#include <core/Mallocator.h>

TEST_CASE("core::OSStr roundtrip")
{
	core::Mallocator allocator;

	auto str = "Hello يا عالم 🌎!"_sv;
	auto osStr = core::OSStr(str, &allocator);
	auto utf8 = osStr.toUtf8(&allocator);
	REQUIRE(utf8 == str);
}
