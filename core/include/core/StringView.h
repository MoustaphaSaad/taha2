#pragma once

#include "core/Exports.h"
#include "core/Rune.h"
#include "core/Array.h"

#include <fmt/core.h>
#include <fmt/format.h>

#include <cassert>
#include <cstring>
#include <cstdint>

namespace core
{
	class String;

	class RuneIterator
	{
		const char* m_ptr = nullptr;
		Rune m_rune;
	public:
		RuneIterator(const char* ptr, Rune rune)
			: m_ptr(ptr)
			, m_rune(rune)
		{}

		RuneIterator& operator++()
		{
			m_ptr = Rune::next(m_ptr);
			m_rune = Rune::decode(m_ptr);
			return *this;
		}

		RuneIterator operator++(int)
		{
			auto copy = *this;
			++(*this);
			return copy;
		}

		bool operator==(RuneIterator other) const { return m_ptr == other.m_ptr; }
		bool operator!=(RuneIterator other) const { return m_ptr != other.m_ptr; }
		const Rune& operator*() const { return m_rune; }
		const Rune* operator->() const { return &m_rune; }
	};

	class StringRunes
	{
		const char* m_begin = nullptr;
		const char* m_end = nullptr;
	public:
		StringRunes(const char* begin, const char* end)
			: m_begin(begin)
			, m_end(end)
		{}

		RuneIterator begin() const { return RuneIterator(m_begin, Rune::decode(m_begin)); }
		RuneIterator end() const { return RuneIterator(m_end, Rune{}); }
	};

	class StringView
	{
		friend class String;

		const char* m_begin = nullptr;
		size_t m_count = 0;

		struct RabinKarpState
		{
			uint32_t hash, pow;
		};

		constexpr static uint32_t PRIME_RABIN_KARP = 16777619;

		static RabinKarpState hashRabinKarp(StringView v);
		static RabinKarpState hashRabinKarpReverse(StringView v);
		CORE_EXPORT static int cmp(StringView a, StringView b);
	public:
		StringView() = default;

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

		const char* data() const { return m_begin; }
		const char* begin() const { return m_begin; }
		const char* end() const { return m_begin + m_count; }

		CORE_EXPORT size_t find(StringView target, size_t start = 0) const;
		CORE_EXPORT size_t find(Rune target, size_t start = 0) const;
		CORE_EXPORT size_t findLast(StringView target, size_t start) const;
		CORE_EXPORT size_t findLast(StringView target) const { return findLast(target, m_count); }
		CORE_EXPORT size_t findFirstByte(StringView str, size_t start = 0) const;

		bool operator==(StringView other) const
		{
			if (m_begin == other.m_begin && m_count == other.m_count)
				return true;

			return cmp(*this, other) == 0;
		}
		bool operator!=(StringView other) const { return !operator==(other); }
		bool operator<(StringView other) const { return cmp(*this, other) < 0; }
		bool operator<=(StringView other) const { return cmp(*this, other) <= 0; }
		bool operator>(StringView other) const { return cmp(*this, other) > 0; }
		bool operator>=(StringView other) const { return cmp(*this, other) >= 0; }

		StringRunes runes() const { return StringRunes{m_begin, m_begin + m_count}; }
		StringView slice(size_t start, size_t end) const
		{
			assert(start <= end && end - start <= m_count);
			return StringView{m_begin + start, end - start};
		}

		CORE_EXPORT Array<StringView> split(StringView delim, bool skipEmpty, Allocator* allocator) const;

		bool startsWith(StringView str) const
		{
			if (str.m_count > m_count)
				return false;

			return slice(0, str.m_count) == str;
		}

		bool endsWith(StringView str) const
		{
			if (str.m_count > m_count)
				return false;

			return slice(m_count - str.m_count, m_count) == str;
		}
	};
}

inline static core::StringView operator "" _sv(const char* ptr, size_t len)
{
	return core::StringView(ptr, len);
}

namespace fmt
{
	template<>
	struct formatter<core::StringView>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const core::StringView& str, FormatContext& ctx)
		{
			return format_to(ctx.out(), "{}", fmt::string_view{str.data(), str.count()});
		}
	};
}