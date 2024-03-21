#include "core/UUID.h"
#include "core/Rand.h"

namespace core
{
	inline uint8_t hex_to_uint8(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return 10 + c - 'a';
		if (c >= 'A' && c <= 'F')
			return 10 + c - 'A';
		return 0;
	}

	inline char char_is_hex(char c)
	{
		return (c >= '0' && c <= '9') ||
			   (c >= 'a' && c <= 'f') ||
			   (c >= 'A' && c <= 'F');
	}

	inline uint8_t hex_to_uint8(char c1, char c2)
	{
		return (hex_to_uint8(c1) << 4) | hex_to_uint8(c2);
	}

	UUID UUID::generate()
	{
		UUID uuid;
		auto ok = Rand::cryptoRand(Span<std::byte>{(std::byte*)&uuid.data, sizeof(uuid.data)});
		coreAssert(ok);
		// version 4
		uuid.data.bytes[6] = (uuid.data.bytes[6] & 0x0f) | 0x40;
		// variant is 10
		uuid.data.bytes[8] = (uuid.data.bytes[8] & 0x3f) | 0x80;
		return uuid;
	}

	Result<UUID> UUID::parse(StringView str, Allocator* allocator)
	{
		UUID res{};

		if (str.count() == 0)
			return errf(allocator, "UUID string is empty"_sv);

		size_t has_braces = 0;
		if (str[0] == '{')
			has_braces = 1;

		if (has_braces > 0 && str[str.count() - 1] != '}')
			return errf(allocator, "mismatched opening curly brace"_sv);

		size_t index = 0;
		bool first_digit = true;
		char digit = 0;
		for (size_t i = has_braces; i < str.count() - has_braces; ++i)
		{
			if (str[i] == '-')
				continue;

			if (index >= 16 || char_is_hex(str[i]) == false)
				return errf(allocator, "invalid uuid"_sv);

			if (first_digit)
			{
				digit = str[i];
				first_digit = false;
			}
			else
			{
				res.data.bytes[index++] = hex_to_uint8(digit, str[i]);
				first_digit = true;
			}
		}

		if (index < 16)
			return errf(allocator, "invalid uuid"_sv);

		return res;
	}
}