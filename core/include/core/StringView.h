#pragma once

#include "core/Rune.h"

#include <cassert>
#include <cstring>
#include <cstdint>

namespace core
{
	class StringView
	{
		const char* m_begin = nullptr;
		size_t m_count = 0;

		struct RabinKarpState
		{
			uint32_t hash, pow;
		};

		constexpr static uint32_t PRIME_RABIN_KARP = 16777619;

		static RabinKarpState hashRabinKarp(StringView v);
		static RabinKarpState hashRabinKarpReverse(StringView v);
		static int cmp(StringView a, StringView b);
	public:
		explicit StringView(const char* ptr)
		{
			m_begin = ptr;
			m_count = ::strlen(ptr);
		}

		StringView(const char* begin, size_t count)
			: m_begin(begin)
			, m_count(count)
		{}

		StringView(const char* begin, const char* end)
			: m_begin(begin)
			, m_count(end - begin)
		{
			assert(begin <= end);
		}

		const char& operator[](size_t i) const
		{
			assert(i < m_count);
			return m_begin[i];
		}

		size_t count() const { return m_count; }
		size_t runeCount() const { return Rune::count(m_begin, m_begin + m_count); }

		const char* begin() const { return m_begin; }
		const char* end() const { return m_begin + m_count; }

		size_t find(StringView target, size_t start = 0) const;
		size_t findLast(StringView target, size_t start) const;
		size_t findLast(StringView target) const { return findLast(target, m_count); }

		bool operator==(StringView other) const { return cmp(*this, other) == 0; }
		bool operator!=(StringView other) const { return cmp(*this, other) != 0; }
		bool operator<(StringView other) const { return cmp(*this, other) < 0; }
		bool operator<=(StringView other) const { return cmp(*this, other) <= 0; }
		bool operator>(StringView other) const { return cmp(*this, other) > 0; }
		bool operator>=(StringView other) const { return cmp(*this, other) >= 0; }
	};
}

inline static core::StringView operator "" _sv(const char* ptr, size_t len)
{
	return core::StringView(ptr, len);
}