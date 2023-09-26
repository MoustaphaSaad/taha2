#pragma once

#include "core/Exports.h"
#include "core/Result.h"

#include <fmt/core.h>

#include <cstdint>
#include <cstring>

namespace core
{
	class UUID
	{
		union
		{
			struct
			{
				uint32_t time_low;
				uint16_t time_mid;
				uint16_t time_hi_and_version;
				uint8_t clk_seq_hi_res;
				uint8_t clk_seq_low;
				uint8_t node[6];
			} parts;
			uint64_t rnd[2];
			uint8_t bytes[16];
		} data;

	public:
		// indicated by a bit pattern in octet 8, marked with N in xxxxxxxx-xxxx-xxxx-Nxxx-xxxxxxxxxxxx
		enum UUID_VARIANT
		{
			// NCS backward compatibility (with the obsolete Apollo Network Computing System 1.5 UUID format)
			// N bit pattern: 0xxx
			// > the first 6 octets of the UUID are a 48-bit timestamp (the number of 4 microsecond units of time since 1 Jan 1980 UTC);
			// > the next 2 octets are reserved;
			// > the next octet is the "address family";
			// > the final 7 octets are a 56-bit host ID in the form specified by the address family
			UUID_VARIANT_NCS,

			// RFC 4122/DCE 1.1
			// N bit pattern: 10xx
			// > big-endian byte order
			UUID_VARIANT_RFC,

			// Microsoft Corporation backward compatibility
			// N bit pattern: 110x
			// > little endian byte order
			// > formely used in the Component Object Model (COM) library
			UUID_VARIANT_MICROSOFT,

			// reserved for possible future definition
			// N bit pattern: 111x
			UUID_VARIANT_RESERVED,
		};

		enum UUID_VERSION
		{
			// only possible for nil or invalid uuids
			UUID_VERSION_NONE,
			// The time-based version specified in RFC 4122
			UUID_VERSION_TIME_BASED,
			// DCE Security version, with embedded POSIX UIDs.
			UUID_VERSION_DCE_SECURITY,
			// The name-based version specified in RFS 4122 with MD5 hashing
			UUID_VERSION_NAME_BASED_MD5,
			// The randomly or pseudo-randomly generated version specified in RFS 4122
			UUID_VERSION_RANDOM_NUMBER_BASED,
			// The name-based version specified in RFS 4122 with SHA1 hashing
			UUID_VERSION_NAME_BASED_SHA1,
		};

		CORE_EXPORT static UUID generate();
		CORE_EXPORT static Result<UUID> parse(StringView str, Allocator* allocator);

		UUID()
		{
			::memset(&data, 0, sizeof(data));
		}
		UUID(const UUID& other) = default;
		UUID(UUID&& other) = default;
		UUID& operator=(const UUID& other) = default;
		UUID& operator=(UUID&& other) = default;
		~UUID() = default;

		bool operator==(const UUID& other) const
		{
			return ::memcmp(&data, &other.data, sizeof(data)) == 0;
		}

		bool operator!=(const UUID& other) const
		{
			return !(*this == other);
		}

		bool operator<(const UUID& other) const
		{
			return ::memcmp(&data, &other.data, sizeof(data)) < 0;
		}

		bool operator>(const UUID& other) const
		{
			return ::memcmp(&data, &other.data, sizeof(data)) > 0;
		}

		bool operator<=(const UUID& other) const
		{
			return ::memcmp(&data, &other.data, sizeof(data)) <= 0;
		}

		bool operator>=(const UUID& other) const
		{
			return ::memcmp(&data, &other.data, sizeof(data)) >= 0;
		}

		uint8_t& operator[](size_t i)
		{
			return data.bytes[i];
		}

		const uint8_t& operator[](size_t i) const
		{
			return data.bytes[i];
		}

		bool is_null() const
		{
			for (size_t i = 0; i < 16; ++i)
			{
				if (data.bytes[i] != 0)
					return false;
			}
			return true;
		}

		UUID_VARIANT variant() const
		{
			if ((data.bytes[8] & 0x80) == 0)
				return UUID_VARIANT_NCS;
			else if ((data.bytes[8] & 0xc0) == 0x80)
				return UUID_VARIANT_RFC;
			else if ((data.bytes[8] & 0xe0) == 0xc0)
				return UUID_VARIANT_MICROSOFT;
			else
				return UUID_VARIANT_RESERVED;
		}

		UUID_VERSION version() const
		{
			if ((data.bytes[6] & 0xf0) == 0x10)
				return UUID_VERSION_TIME_BASED;
			else if ((data.bytes[6] & 0xf0) == 0x20)
				return UUID_VERSION_DCE_SECURITY;
			else if ((data.bytes[6] & 0xf0) == 0x30)
				return UUID_VERSION_NAME_BASED_MD5;
			else if ((data.bytes[6] & 0xf0) == 0x40)
				return UUID_VERSION_RANDOM_NUMBER_BASED;
			else if ((data.bytes[6] & 0xf0) == 0x50)
				return UUID_VERSION_NAME_BASED_SHA1;
			else
				return UUID_VERSION_NONE;
		}
	};
}

namespace fmt
{
	template<>
	struct formatter<core::UUID>
	{
		template<typename ParseContext>
		constexpr auto
		parse(ParseContext &ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto
		format(core::UUID value, FormatContext &ctx)
		{
			return format_to(
				ctx.out(),
				"{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
				value[0],
				value[1],
				value[2],
				value[3],
				value[4],
				value[5],
				value[6],
				value[7],
				value[8],
				value[9],
				value[10],
				value[11],
				value[12],
				value[13],
				value[14],
				value[15]
			);
		}
	};
}