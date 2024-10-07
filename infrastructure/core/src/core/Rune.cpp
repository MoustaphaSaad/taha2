#include "core/Rune.h"

#include <utf8proc.h>

namespace core
{
	const Rune Rune::SELF{0x80};

	Rune Rune::lower() const
	{
		return Rune{utf8proc_tolower(m_value)};
	}

	Rune Rune::upper() const
	{
		return Rune{utf8proc_toupper(m_value)};
	}

	size_t Rune::size() const
	{
		char bytes[4] = {};
		return encode(*this, bytes);
	}

	bool Rune::isLetter() const
	{
		auto cat = utf8proc_category(m_value);
		return (
			cat == UTF8PROC_CATEGORY_LU || cat == UTF8PROC_CATEGORY_LL || cat == UTF8PROC_CATEGORY_LT ||
			cat == UTF8PROC_CATEGORY_LM || cat == UTF8PROC_CATEGORY_LO);
	}

	bool Rune::isNumber() const
	{
		auto cat = utf8proc_category(m_value);
		return (cat == UTF8PROC_CATEGORY_ND || cat == UTF8PROC_CATEGORY_NL || cat == UTF8PROC_CATEGORY_NO);
	}

	bool Rune::isValid() const
	{
		return utf8proc_codepoint_valid(m_value);
	}

	size_t Rune::count(const char* ptr)
	{
		size_t result = 0;
		while (ptr != nullptr && *ptr != '\0')
		{
			result += ((*ptr & 0xC0) != 0x80);
			++ptr;
		}
		return result;
	}

	size_t Rune::count(const char* begin, const char* end)
	{
		size_t result = 0;
		while (begin != nullptr && *begin != '\0' && begin < end)
		{
			result += ((*begin & 0xC0) != 0x80);
			++begin;
		}
		return result;
	}

	const char* Rune::next(const char* ptr)
	{
		++ptr;
		while (*ptr && ((*ptr & 0xC0) == 0x80))
		{
			++ptr;
		}
		return ptr;
	}

	const char* Rune::prev(const char* ptr)
	{
		--ptr;
		while (*ptr && ((*ptr & 0xC0) == 0x80))
		{
			--ptr;
		}
		return ptr;
	}

	Rune Rune::decode(const char* ptr)
	{
		if (ptr == nullptr)
		{
			return Rune{};
		}

		if (*ptr == 0)
		{
			return Rune{};
		}

		size_t str_count = 1;
		for (size_t i = 1; i < 4; ++i)
		{
			if (ptr[i] == 0)
			{
				break;
			}
			else
			{
				++str_count;
			}
		}

		Rune r{};
		utf8proc_iterate((utf8proc_uint8_t*)ptr, str_count, (utf8proc_int32_t*)&r.m_value);
		return r;
	}

	size_t Rune::encode(Rune c, char* ptr)
	{
		return utf8proc_encode_char(c, (utf8proc_uint8_t*)ptr);
	}
}
