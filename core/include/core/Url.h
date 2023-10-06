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
	public:
		struct KeyValue
		{
			String key, value;
		};

	private:
		Array<KeyValue> m_keyValues;
		Map<StringView, size_t> m_keyToIndex;

	public:
		using KeyIterator = Map<StringView, size_t>::iterator;

		static Result<UrlQuery> parse(StringView query, Allocator* allocator);

		explicit UrlQuery(Allocator* allocator);

		void add(StringView key, StringView value);
		const KeyIterator find(StringView key) const;
		StringView get(const KeyIterator) const;

		size_t count() const { return m_keyValues.count(); }
		const KeyValue& operator[](size_t index) const { return m_keyValues[index]; }
	};

	class Url
	{
		String m_scheme;
		String m_user;
		String m_host;
		String m_port;
		String m_path;
		UrlQuery m_query;
		String m_fragment;
		int m_ipVersion = 0;
	public:
		static String encodeQueryElement(StringView str, Allocator* allocator);

		explicit Url(Allocator* allocator)
			: m_scheme(allocator),
			  m_user(allocator),
			  m_host(allocator),
			  m_port(allocator),
			  m_path(allocator),
			  m_query(allocator),
			  m_fragment(allocator)
		{}

		StringView scheme() const { return m_scheme; }
		StringView user() const { return m_user; }
		StringView host() const { return m_host; }
		StringView port() const { return m_port; }
		StringView path() const { return m_path; }
		const UrlQuery& query() const { return m_query; }
		UrlQuery& query() { return m_query; }
		StringView fragment() const { return m_fragment; }
		int ipVersion() const { return m_ipVersion; }

		Result<String> toString(Allocator* allocator) const;

		bool isDefaultPort() const
		{
			return ((m_scheme == "http"_sv && m_port == "80"_sv) || (m_scheme == "https"_sv && m_port == "443"_sv));
		}
	};
}