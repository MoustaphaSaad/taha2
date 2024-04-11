#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/Rune.h"
#include "core/StringView.h"
#include "core/Assert.h"

#include <fmt/core.h>
#include <fmt/format.h>

#include <cstring>
#include <utility>

namespace core
{
	class OSString;
	class Buffer;

	class String
	{
		friend class OSString;
		friend class Buffer;

		Allocator* m_allocator = nullptr;
		char* m_ptr = nullptr;
		size_t m_capacity = 0;
		size_t m_count = 0;

		CORE_EXPORT void destroy();

		CORE_EXPORT void copyFrom(const String& other);

		CORE_EXPORT void moveFrom(String&& other);

		void grow(size_t new_capacity);

		CORE_EXPORT void ensureSpaceExists(size_t count);

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

		explicit String(Buffer&& buffer);

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
			coreAssert(i < m_count);
			return m_ptr[i];
		}

		const char& operator[](size_t i) const
		{
			coreAssert(i < m_count);
			return m_ptr[i];
		}

		operator StringView() const { return StringView{m_ptr, m_count}; }
		explicit operator Span<const std::byte>() const { return Span<const std::byte>{(const std::byte*)m_ptr, m_count}; }

		size_t count() const { return m_count; }
		size_t capacity() const { return m_capacity; }
		size_t runeCount() const { return Rune::count(m_ptr, m_ptr + m_count); }
		char* data() { return m_ptr; }
		const char* data() const { return m_ptr; }
		Allocator* allocator() const { return m_allocator; }

		CORE_EXPORT void resize(size_t new_count);
		void reserve(size_t extra_count) { ensureSpaceExists(extra_count); }

		CORE_EXPORT void push(StringView str);
		CORE_EXPORT void push(Rune r);
		CORE_EXPORT void pushByte(char v);

		CORE_EXPORT size_t find(StringView str, size_t start = 0) const;
		CORE_EXPORT size_t findIgnoreCase(StringView str, size_t start = 0) const;
		CORE_EXPORT size_t find(Rune target, size_t start = 0) const;
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

		CORE_EXPORT void replace(StringView search, StringView replace);
		StringRunes runes() const { return StringView{*this}.runes(); }
		size_t findFirstByte(StringView str, size_t start = 0) const { return StringView{*this}.findFirstByte(str, start); }

		const char* begin() const { return m_ptr; }
		char* begin() { return m_ptr; }
		const char* end() const { return m_ptr + m_count; }
		char* end() { return m_ptr + m_count; }

		Array<StringView> split(StringView delim, bool skipEmpty, Allocator* allocator) const { return StringView{*this}.split(delim, skipEmpty, allocator); }

		bool startsWith(StringView other) const { return StringView{*this}.startsWith(other); }
		bool endsWith(StringView other) const { return StringView{*this}.endsWith(other); }
	};

	class StringBackInserter
	{
		String* m_str = nullptr;
	public:
		using iterator_category = std::output_iterator_tag;

		StringBackInserter(String* str)
			: m_str(str)
		{}

		StringBackInserter& operator=(char v)
		{
			m_str->pushByte(v);
			return *this;
		}

		StringBackInserter& operator*() { return *this; }
		StringBackInserter& operator++() { return *this; }
		StringBackInserter& operator++(int) { return *this; }
	};

	template<typename ... Args>
	[[nodiscard]] inline String strf(Allocator* allocator, StringView format, Args&& ... args)
	{
		String out{allocator};
		StringBackInserter it{&out};
		fmt::format_to(it, fmt::runtime(std::string_view{format.begin(), format.count()}), std::forward<Args>(args)...);
		return out;
	}
}

namespace std
{
	template<>
	struct iterator_traits<core::StringBackInserter>
	{
		typedef ptrdiff_t difference_type;
		typedef char value_type;
		typedef char* pointer;
		typedef char& reference;
		typedef std::output_iterator_tag iterator_category;
	};
}

namespace fmt
{
	template<>
	struct formatter<core::String>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const core::String& str, FormatContext& ctx)
		{
			return format_to(ctx.out(), fmt::runtime("{}"), core::StringView{str});
		}
	};
}