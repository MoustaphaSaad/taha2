#include "core/Intrinsics.h"

namespace core
{
	static union
	{
		char c[4];
		uint32_t dword;
	} ENDIAN_TEST = {{'l', '?', '?', 'b'}};

	// API
	Endianness systemEndianness()
	{
		static Endianness endian = char(ENDIAN_TEST.dword) == 'l' ? Endianness::Little : Endianness::Big;
		return endian;
	}

	uint16_t byteswap_uint16(uint16_t v)
	{
		return __builtin_bswap16(v);
	}

	uint32_t byteswap_uint32(uint32_t v)
	{
		return __builtin_bswap32(v);
	}

	uint64_t byteswap_uint64(uint64_t v)
	{
		return __builtin_bswap64(v);
	}

	int leading_zeros(uint64_t v)
	{
		return __builtin_clzll(v);
	}
}