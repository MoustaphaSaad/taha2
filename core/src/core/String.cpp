#include "core/String.h"

#include <utf8proc.h>

namespace core
{
	void String::destroy()
	{
		if (m_allocator == nullptr || m_ptr == nullptr)
			return;

		m_allocator->release(m_ptr, m_capacity);
		m_allocator->free(m_ptr, m_capacity);
	}

	void String::copyFrom(const String& other)
	{
		m_allocator = other.m_allocator;
		m_count = other.m_count;
		m_capacity = m_count;

		m_ptr = (char*)m_allocator->alloc(m_capacity, alignof(char));
		m_allocator->commit(m_ptr, m_capacity);

		::memcpy(m_ptr, other.m_ptr, m_capacity);
	}

	void String::moveFrom(String&& other)
	{
		m_allocator = other.m_allocator;
		m_ptr = other.m_ptr;
		m_count = other.m_count;
		m_capacity = other.m_capacity;

		other.m_allocator = nullptr;
		other.m_ptr = nullptr;
		other.m_count = 0;
		other.m_capacity = 0;
	}

	void String::grow(size_t new_capacity)
	{
		auto new_ptr = (char*)m_allocator->alloc(new_capacity, alignof(char));
		m_allocator->commit(new_ptr, m_count);

		::memcpy(new_ptr, m_ptr, m_count);

		m_allocator->release(m_ptr, m_capacity);
		m_allocator->free(m_ptr, m_capacity);

		m_ptr = new_ptr;
		m_capacity = new_capacity;
	}

	void String::ensureSpaceExists(size_t count)
	{
		if (m_count + count > m_capacity)
		{
			auto new_capacity = m_capacity * 2;
			if (new_capacity == 0)
				new_capacity = 8;

			if (new_capacity < m_count + count)
				new_capacity = m_count + count;

			grow(new_capacity);
		}
	}

	void String::resize(size_t new_count)
	{
		// +1 for the null terminator
		if (new_count + 1 > m_capacity)
			grow(new_count + 1);

		m_count = new_count;
		m_ptr[m_count] = '\0';
	}

	String::String(StringView str, Allocator* allocator)
		: m_allocator(allocator)
	{
		if (str.count() != 0)
		{
			m_count = str.count();
			m_capacity = m_count + 1;

			m_ptr = (char*)m_allocator->alloc(m_capacity, alignof(char));
			m_allocator->commit(m_ptr, m_capacity);

			::memcpy(m_ptr, str.begin(), m_count);
			m_ptr[m_count] = '\0';
		}
	}

	void String::push(StringView str)
	{
		auto len = m_count;
		resize(m_count + (str.count() + 1));
		--m_count;
		::memcpy(m_ptr + len, str.begin(), str.count());
		m_ptr[m_count] = '\0';
	}

	void String::push(Rune r)
	{
		auto old_count = m_count;
		// +5 = 4 for the rune + 1 for the null termination
		ensureSpaceExists(m_count + 5);

		auto width = Rune::encode(r, m_ptr + m_count);
		assert(width > 0 && width <= 4);
		m_count += width;
		m_ptr[m_count] = '\0';
	}

	void String::pushByte(char v)
	{
		// +2 = 1 for the byte + 1 for the null termination
		ensureSpaceExists(m_count + 2);
		m_ptr[m_count] = v;
		++m_count;
		m_ptr[m_count] = '\0';
	}

	size_t String::find(StringView str, size_t start) const
	{
		StringView self = *this;
		return self.find(str, start);
	}

	size_t String::find(Rune target, size_t start) const
	{
		StringView self = *this;
		return self.find(target, start);
	}

	size_t String::findLast(StringView str, size_t start) const
	{
		StringView self = *this;
		return self.findLast(str, start);
	}

	void String::replace(StringView search, StringView replace)
	{
		String out{m_allocator};
		out.ensureSpaceExists(m_count);
		// find the first pattern or -1
		size_t search_it = find(search, 0);
		size_t it		 = 0;
		// while we didn't finish the string
		while (it < m_count)
		{
			// if search_str is not found then put search_it to the end of string
			if (search_it == SIZE_MAX)
			{
				// push the remaining content
				if (it < m_count)
					out.push(StringView{m_ptr + it, m_count - it});

				// exit we finished the string
				break;
			}

			// push the preceding content
			if (search_it > it)
				out.push(StringView{m_ptr + it, search_it - it});

			// push the replacement string
			out.push(replace);

			// advance content iterator by search string
			it = search_it + search.count();

			// find for next pattern
			search_it = find(search, it);
		}
		*this = std::move(out);
	}
}