#pragma once

#include "core/Rune.h"

#include <assert.h>
#include <string.h>

namespace core
{
	class StringView
	{
		const char* m_begin = nullptr;
		const char* m_end = nullptr;
	public:
		explicit StringView(const char* ptr)
		{
			m_begin = ptr;
			m_end = ptr + ::strlen(ptr);
		}

		StringView(const char* begin, const char* end)
			: m_begin(begin)
			, m_end(end)
		{
			assert(begin <= end);
		}

		const char& operator[](size_t i) const
		{
			assert(i < count());
			return m_begin[i];
		}

		size_t count() const { return m_end - m_begin; }
		size_t runeCount() const { return Rune::count(m_begin, m_end); }

		const char* begin() const { return m_begin; }
		const char* end() const { return m_end; }
	};
}

inline static core::StringView operator "" _sv(const char* ptr, size_t len)
{
	return core::StringView(ptr, ptr + len);
}