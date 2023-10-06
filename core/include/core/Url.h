#pragma once

#include "core/Exports.h"
#include "core/String.h"
#include "core/Array.h"
#include "core/Hash.h"
#include "core/Result.h"

namespace core
{
	class UrlQuery
	{
		struct KeyValue
		{
			String key, value;
		};

		Array<KeyValue> m_keyValues;
		Map<StringView, size_t> m_keyToIndex;
	public:
		using KeyIterator = Map<StringView, size_t>::iterator;

		static Result<UrlQuery> parse(StringView query, Allocator* allocator);

		explicit UrlQuery(Allocator* allocator);

		void add(StringView key, StringView value);
		const KeyIterator find(StringView key) const;
		StringView get(const KeyIterator) const;
	};
}