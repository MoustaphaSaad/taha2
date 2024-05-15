#include <doctest/doctest.h>

#include <core/Intrinsics.h>
#include <core/File.h>

TEST_CASE("report endianness")
{
	auto endianness = core::systemEndianness();
	switch (endianness)
	{
	case core::Endianness::Little:
		core::strf(core::File::STDOUT, "System endianness: Little\n"_sv);
		break;
	case core::Endianness::Big:
		core::strf(core::File::STDOUT, "System endianness: Big\n"_sv);
		break;
	default:
		core::strf(core::File::STDOUT, "System endianness: Unknown\n"_sv);
		break;
	}
}

TEST_CASE("basic byteswap")
{
	uint16_t v16 = 0x1234;
	uint32_t v32 = 0x12345678;
	uint64_t v64 = 0x123456789abcdef0;

	auto v16_swapped = core::byteswap_uint16(v16);
	auto v32_swapped = core::byteswap_uint32(v32);
	auto v64_swapped = core::byteswap_uint64(v64);

	REQUIRE(v16_swapped == 0x3412);
	REQUIRE(v32_swapped == 0x78563412);
	REQUIRE(v64_swapped == 0xf0debc9a78563412);
}

TEST_CASE("basic leading zeros")
{
	uint64_t v64 = 0x123456789abcdef0;

	auto v64_leading_zeros = core::leading_zeros(v64);

	REQUIRE(v64_leading_zeros == 3);
}
