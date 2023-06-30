#pragma once

#include "core/Rune.h"

#include <assert.h>

namespace core
{
	class StringView
	{
		char* m_begin = nullptr;
		char* m_end = nullptr;
	public:
		explicit StringView(const char* ptr)
			: m_ptr(ptr)
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

		char& operator[](size_t i)
		{
			assert(i < count());
			return m_begin[i];
		}

		const char& operator[](size_t i) const
		{
			assert(i < count());
			return m_begin[i];
		}

		size_t count() const { return m_end - m_begin; }
		size_t runeCount() const { return Rune::count(m_begin, m_end); }
	}
}