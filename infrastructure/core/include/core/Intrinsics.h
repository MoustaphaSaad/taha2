#pragma once

#include "core/Exports.h"
#include <cstdint>

namespace core
{
	enum class Endianness
	{
		Little,
		Big,
	};

	CORE_EXPORT Endianness systemEndianness();
	CORE_EXPORT uint16_t byteswap_uint16(uint16_t value);
	CORE_EXPORT uint32_t byteswap_uint32(uint32_t value);
	CORE_EXPORT uint64_t byteswap_uint64(uint64_t value);
	CORE_EXPORT int leading_zeros(uint64_t value);
}