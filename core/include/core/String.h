#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/Rune.h"
#include "core/StringView.h"

#include <cstring>
#include <cassert>
#include <utility>

namespace core
{
	class String
	{
		Allocator* m_allocator = nullptr;
		char* m_ptr = nullptr;
		size_t m_capacity = 0;
		size_t m_count = 0;

		void destroy();

		void copyFrom(const String& other);

		void moveFrom(String&& other);

		void grow(size_t new_capacity);

		void ensureSpaceExists(size_t count);

		void resize(size_t new_count);

	public:
		explicit String(Allocator* allocator)
			: m_allocator(allocator)
		{}

		CORE_EXPORT String(StringView str, Allocator* allocator);

		String(const String& other)
		{
			copyFrom(other);
		}

		String(String&& other)
		{
			moveFrom(std::move(other));
		}

		String& operator=(const String& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		String& operator=(String&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		String& operator=(StringView other)
		{
			*this = String(other, m_allocator);
			return *this;
		}

		~String()
		{
			destroy();
		}

		char& operator[](size_t i)
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		const char& operator[](size_t i) const
		{
			assert(i < m_count);
			return m_ptr[i];
		}

		operator StringView() const { return StringView(m_ptr, m_count); }

		size_t count() const { return m_count; }
		size_t capacity() const { return m_capacity; }
		size_t runeCount() const { return Rune::count(m_ptr, m_ptr + m_count); }

		CORE_EXPORT void push(StringView str);
		CORE_EXPORT void push(Rune r);

		CORE_EXPORT size_t find(StringView str, size_t start = 0) const;
		CORE_EXPORT size_t findLast(StringView str, size_t start) const;
		size_t findLast(StringView str) const { return findLast(str, m_count); }

		bool operator==(const String& other) const { return StringView::cmp(*this, other) == 0; }
		bool operator!=(const String& other) const { return StringView::cmp(*this, other) != 0; }
		bool operator<(const String& other) const { return StringView::cmp(*this, other) < 0; }
		bool operator<=(const String& other) const { return StringView::cmp(*this, other) <= 0; }
		bool operator>(const String& other) const { return StringView::cmp(*this, other) > 0; }
		bool operator>=(const String& other) const { return StringView::cmp(*this, other) >= 0; }
		bool operator==(StringView other) const { return StringView::cmp(*this, other) == 0; }
		bool operator!=(StringView other) const { return StringView::cmp(*this, other) != 0; }
		bool operator<(StringView other) const { return StringView::cmp(*this, other) < 0; }
		bool operator<=(StringView other) const { return StringView::cmp(*this, other) <= 0; }
		bool operator>(StringView other) const { return StringView::cmp(*this, other) > 0; }
		bool operator>=(StringView other) const { return StringView::cmp(*this, other) >= 0; }
	};
}