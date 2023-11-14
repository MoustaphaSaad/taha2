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

	StringView::RabinKarpState StringView::hashRabinKarpReverse(StringView str)
	{
		assert(str.m_count > 0);

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

		auto self = slice(start, m_count);

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

	size_t StringView::find(Rune target, size_t start) const
	{
		assert(start < m_count);
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
}