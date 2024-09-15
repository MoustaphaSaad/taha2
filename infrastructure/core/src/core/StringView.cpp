#include "core/StringView.h"

namespace core
{
	StringView::RabinKarpState StringView::hashRabinKarp(StringView str)
	{
		RabinKarpState res{0, 1};

		for (size_t i = 0; i < str.count(); ++i)
			res.hash = res.hash * PRIME_RABIN_KARP + uint32_t(str.m_begin[i]);
		auto sq = PRIME_RABIN_KARP;
		for (size_t i = str.m_count; i > 0; i >>= 1)
		{
			if ((i & 1) != 0)
				res.pow *= sq;
			sq *= sq;
		}
		return res;
	}

	StringView::RabinKarpState StringView::hashRabinKarpIgnoreCase(StringView str)
	{
		RabinKarpState res{0, 1};

		for (size_t i = 0; i < str.count(); ++i)
			res.hash = res.hash * PRIME_RABIN_KARP + uint32_t(Rune{str.m_begin[i]}.lower());
		auto sq = PRIME_RABIN_KARP;
		for (size_t i = str.m_count; i > 0; i >>= 1)
		{
			if ((i & 1) != 0)
				res.pow *= sq;
			sq *= sq;
		}
		return res;
	}

	StringView::RabinKarpState StringView::hashRabinKarpReverse(StringView str)
	{
		validate(str.m_count > 0);

		RabinKarpState res{0, 1};

		for (size_t i = 0; i < str.m_count; ++i)
		{
			auto rev_i = str.m_count - i - 1;
			res.hash = res.hash * PRIME_RABIN_KARP + uint32_t(str.m_begin[rev_i]);
		}
		auto sq = PRIME_RABIN_KARP;
		for (size_t i = str.m_count; i > 0; i >>= 1)
		{
			if ((i & 1) != 0)
				res.pow *= sq;
			sq *= sq;
		}
		return res;
	}

	int StringView::cmp(StringView a, StringView b)
	{
		auto min_count = a.m_count < b.m_count ? a.m_count : b.m_count;
		auto res = ::memcmp(a.m_begin, b.m_begin, min_count);
		// NOTE: Possible that a[:min_count] == b[:min_count] but a>b, or vice versa, like "ABC" and "AB"
		if (res == 0)
		{
			if (a.m_count == b.m_count)
				return 0;
			else if (a.m_count > b.m_count)
				return 1;
			else
				return -1;
		}
		else
		{
			return res;
		}
	}

	size_t StringView::find(StringView target, size_t start) const
	{
		if (start >= m_count || m_count - start < target.m_count)
			return SIZE_MAX;

		auto self = sliceRight(start);

		if (target.m_count == 0)
		{
			return start;
		}
		else if (target.m_count == 1)
		{
			for (size_t i = 0; i < self.count(); ++i)
				if (self.m_begin[i] == target.m_begin[0])
					return start + i;
			return SIZE_MAX;
		}
		else if (target.m_count == m_count)
		{
			if (target == self)
				return start;
			return SIZE_MAX;
		}
		else if (target.m_count > m_count)
		{
			return SIZE_MAX;
		}

		auto [hash, pow] = hashRabinKarp(target);

		uint32_t h{};
		for (size_t i = 0; i < target.m_count; ++i)
		{
			h = h * PRIME_RABIN_KARP + uint32_t(self.m_begin[i]);
		}

		if (h == hash && ::memcmp(self.m_begin, target.m_begin, target.m_count) == 0)
			return start;

		for (size_t i = target.m_count; i < self.m_count;)
		{
			h *= PRIME_RABIN_KARP;
			h += uint32_t(self.m_begin[i]);
			h -= pow * uint32_t(self.m_begin[i - target.m_count]);
			i += 1;
			if (h == hash && ::memcmp(self.m_begin + i - target.m_count, target.m_begin, target.m_count) == 0)
			{
				return i - target.m_count + start;
			}
		}
		return SIZE_MAX;
	}

	size_t StringView::findIgnoreCase(StringView target, size_t start) const
	{
		if (start >= m_count || m_count - start < target.m_count)
			return SIZE_MAX;

		auto self = sliceRight(start);

		if (target.m_count == 0)
		{
			return start;
		}
		else if (target.m_count == 1)
		{
			for (size_t i = 0; i < self.count(); ++i)
				if (Rune{self.m_begin[i]}.lower() == Rune{target.m_begin[0]}.lower())
					return start + i;
			return SIZE_MAX;
		}
		else if (target.m_count == m_count)
		{
			if (target.endsWithIgnoreCase(self))
				return start;
			return SIZE_MAX;
		}
		else if (target.m_count > m_count)
		{
			return SIZE_MAX;
		}

		auto [hash, pow] = hashRabinKarpIgnoreCase(target);

		uint32_t h{};
		for (size_t i = 0; i < target.m_count; ++i)
		{
			h = h * PRIME_RABIN_KARP + uint32_t(Rune{self.m_begin[i]}.lower());
		}

		if (h == hash && self.equalsIgnoreCase(target))
			return start;

		for (size_t i = target.m_count; i < self.m_count;)
		{
			h *= PRIME_RABIN_KARP;
			h += uint32_t(Rune{self.m_begin[i]}.lower());
			h -= pow * uint32_t(Rune{self.m_begin[i - target.m_count]}.lower());
			i += 1;
			if (h == hash && self.sliceRight(i - target.count()).equalsIgnoreCase(target))
			{
				return i - target.m_count + start;
			}
		}
		return SIZE_MAX;
	}

	size_t StringView::find(Rune target, size_t start) const
	{
		validate(start < m_count);
		for (auto it = m_begin + start; it < m_begin + m_count; it = Rune::next(it))
		{
			auto c = Rune::decode(it);
			if (c == target)
				return it - m_begin;
		}
		return SIZE_MAX;
	}

	size_t StringView::findLast(StringView target, size_t start) const
	{
		auto self = *this;

		if (start < self.m_count)
			self.m_count = start + 1;

		if (target.m_count == 0)
		{
			return self.m_count;
		}
		else if (target.m_count == 1)
		{
			for (size_t i = 0; i < self.m_count; ++i)
			{
				auto rev_i = self.m_count - i - 1;
				if (self.m_begin[rev_i] == target.m_begin[0])
					return rev_i;
			}
			return SIZE_MAX;
		}
		else if (target.m_count == self.m_count)
		{
			if (::memcmp(self.m_begin, target.m_begin, target.m_count) == 0)
				return 0;
			return SIZE_MAX;
		}
		else if (target.m_count > self.m_count)
		{
			return SIZE_MAX;
		}

		auto [hash, pow] = hashRabinKarpReverse(target);
		auto last = self.m_count - target.m_count;

		uint32_t h{};
		for (size_t i = self.m_count - 1; i >= last; --i)
			h = h * PRIME_RABIN_KARP + uint32_t(self.m_begin[i]);
		if (h == hash && ::memcmp(self.m_begin + last, target.m_begin, target.m_count) == 0)
			return last;

		for (size_t i = 0; i < last; ++i)
		{
			auto rev_i = last - i - 1;
			h *= PRIME_RABIN_KARP;
			h += uint32_t(self.m_begin[rev_i]);
			h -= pow * uint32_t(self.m_begin[rev_i + target.m_count]);
			if (h == hash && ::memcmp(self.m_begin + rev_i, target.m_begin, target.m_count) == 0)
				return rev_i;
		}
		return SIZE_MAX;
	}

	size_t StringView::findFirstByte(StringView str, size_t start) const
	{
		for (size_t i = start; i < m_count; ++i)
		{
			for (auto r: str)
			{
				if (r == m_begin[i])
					return i;
			}
		}
		return SIZE_MAX;
	}

	Array<StringView> StringView::split(StringView delim, bool skipEmpty, Allocator* allocator) const
	{
		Array<StringView> res{allocator};

		size_t ix = 0;
		while (true)
		{
			if (ix + delim.count() > m_count)
				break;

			size_t delim_ix = find(delim, ix);
			if (delim_ix == SIZE_MAX)
				break;

			auto skip = skipEmpty && ix == delim_ix;
			if (!skip)
				res.push(slice(ix, delim_ix));

			ix = delim_ix + delim.count();
			if (ix == m_count)
				break;
		}

		if (ix != m_count)
			res.push(slice(ix, m_count));
		else if (!skipEmpty && ix == m_count)
			res.push(StringView{});

		return res;
	}

	bool StringView::equalsIgnoreCase(StringView other) const
	{
		if (m_count != other.m_count)
			return false;

		for (size_t i = 0; i < m_count; ++i)
		{
			auto a = m_begin + i;
			auto b = other.m_begin + i;
			if (Rune::decode(a).lower() != Rune::decode(b).lower())
				return false;
		}
		return true;
	}

	StringView StringView::trimLeft() const
	{
		return trimLeft(" \n\t\r\v"_sv);
	}

	StringView StringView::trimRight() const
	{
		return trimRight(" \n\t\r\v"_sv);
	}

	StringView StringView::trim() const
	{
		return trim(" \n\t\r\v"_sv);
	}

	bool StringView::isValidUtf8() const
	{
		struct AcceptRange
		{
			uint8_t lo, hi;
		};

		static constexpr AcceptRange acceptRanges[5] = {
			AcceptRange{0x80, 0xbf},
			AcceptRange{0xa0, 0xbf},
			AcceptRange{0x80, 0x9f},
			AcceptRange{0x90, 0xbf},
			AcceptRange{0x80, 0x8f},
		};

		static constexpr uint8_t xx = 0xf1;
		static constexpr uint8_t as = 0xf0;
		static constexpr uint8_t s1 = 0x02;
		static constexpr uint8_t s2 = 0x13;
		static constexpr uint8_t s3 = 0x03;
		static constexpr uint8_t s4 = 0x23;
		static constexpr uint8_t s5 = 0x34;
		static constexpr uint8_t s6 = 0x04;
		static constexpr uint8_t s7 = 0x44;

		static constexpr uint8_t acceptSizes[256] = {
			//   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x00-0x0F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x10-0x1F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x20-0x2F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x30-0x3F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x40-0x4F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x50-0x5F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x60-0x6F
			as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x70-0x7F
			//   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
			xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x80-0x8F
			xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x90-0x9F
			xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xA0-0xAF
			xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xB0-0xBF
			xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xC0-0xCF
			s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xD0-0xDF
			s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3, // 0xE0-0xEF
			s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xF0-0xFF
		};

		auto bytes = (uint8_t*)m_begin;

		for (size_t i = 0; i < m_count;)
		{
			auto byte = bytes[i];
			if (Rune{byte} < Rune::SELF) {
				++i;
				continue;
			}

			auto x = acceptSizes[byte];
			if (x == xx) return false;

			auto size = (int)(x & 7);
			if (i + size > m_count)
				return false;

			auto ar = acceptRanges[x >> 4];
			if (auto b = bytes[i + 1]; b < ar.lo || ar.hi < b)
			{
				return false;
			}
			else if (size == 2)
			{
				// okay
			}
			else if (auto c = bytes[i + 2]; c < 0x80 || 0xbf < c)
			{
				return false;
			}
			else if (size == 3)
			{
				// okay
			}
			else if (auto d = bytes[i + 3]; d < 0x80 || 0xbf < d)
			{
				return false;
			}
			i += size;
		}
		return true;
	}
}