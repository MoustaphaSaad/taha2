#include "core/Url.h"
#include "core/MemoryStream.h"

namespace core
{
	static constexpr uint8_t ALLOWED_CHAR[256] = {
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	// NUL SOH STX ETX  EOT ENQ ACK BEL
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	// BS  HT  LF  VT   FF  CR  SO  SI
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	// DLE DC1 DC2 DC3  DC4 NAK SYN ETB
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	// CAN EM  SUB ESC  FS  GS  RS  US
		0x00, 0x01, 0x00, 0x00, 0x01, 0x20, 0x01, 0x01, // SP ! " #  $ % & '
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x08, //  ( ) * +  , - . /
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //  0 1 2 3  4 5 6 7
		0x01, 0x01, 0x04, 0x01, 0x00, 0x01, 0x00, 0x10, //  8 9 : ;  < = > ?
		0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //  @ A B C  D E F G
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //  H I J K  L M N O
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //  P Q R S  T U V W
		0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, //  X Y Z [  \ ] ^ _
		0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //  ` a b c  d e f g
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //  h i j k  l m n o
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //  p q r s  t u v w
		0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, //  x y z {  | } ~ DEL
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	constexpr static auto HEX_DIGITS = "0123456789ABCDEF";

	static bool isAllowedChar(char c, uint8_t mask)
	{
		return (ALLOWED_CHAR[uint8_t(c)] & mask) != 0;
	}

	static bool isAllowedStr(StringView str, uint8_t mask)
	{
		for (auto b: str)
		{
			if (isAllowedChar(b, mask) == false)
			{
				return false;
			}
		}
		return true;
	}

	static int hexDigit(char c)
	{
		if (c >= '0' && c <= '9')
		{
			return c - '0';
		}
		else if (c >= 'A' && c <= 'F')
		{
			return c - 'A' + 10;
		}
		else if (c >= 'a' && c <= 'f')
		{
			return c - 'a' + 10;
		}
		return -1;
	}

	static Result<String> decodePlusStr(StringView str, Allocator* allocator)
	{
		if (str.count() == 0)
		{
			return String{allocator};
		}

		String res{allocator};
		res.reserve(str.count());

		for (size_t i = 0; i < str.count(); ++i)
		{
			auto c = str[i];

			if (c == '+')
			{
				c = ' ';
			}
			else if (c == '%')
			{
				++i;
				if (i >= str.count())
				{
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);
				}
				c = str[i];

				auto a = hexDigit(c);
				if (a == -1)
				{
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);
				}

				++i;
				if (i >= str.count())
				{
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);
				}
				c = str[i];

				auto b = hexDigit(c);
				if (b == -1)
				{
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);
				}

				c = (a << 4) | b;
			}
			res.pushByte(c);
		}

		return res;
	}

	static String encodeString(StringView str, uint8_t mask, Allocator* allocator)
	{
		String res{allocator};
		res.reserve(str.count());

		for (auto c: str)
		{
			if (isAllowedChar(c, mask))
			{
				res.pushByte(c);
			}
			else
			{
				res.pushByte('%');
				res.pushByte(HEX_DIGITS[c >> 4]);
				res.pushByte(HEX_DIGITS[c & 0x0F]);
			}
		}

		return res;
	}

	static bool is_allowed_char(char c, uint8_t mask)
	{
		return (ALLOWED_CHAR[uint8_t(c)] & mask) != 0;
	}

	static bool is_allowed_str(const char* begin, const char* end, uint8_t mask)
	{
		for (auto it = begin; it != end; ++it)
		{
			if (is_allowed_char(*it, mask) == false)
			{
				return false;
			}
		}
		return true;
	}

	static const char* find_first_of(const char* begin, const char* end, const char* search)
	{
		for (auto it = begin; it != end; ++it)
		{
			for (auto search_it = search; *search_it; ++search_it)
			{
				if (*it == *search_it)
				{
					return it;
				}
			}
		}
		return end;
	}

	static bool is_alpha(char c)
	{
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	}

	static bool is_num(char c)
	{
		return c >= '0' && c <= '9';
	}

	static bool is_alnum(char c)
	{
		return is_alpha(c) || is_num(c);
	}

	static bool is_scheme(const char* begin, const char* end)
	{
		if (begin == nullptr || end == nullptr || begin == end || is_alpha(*begin) == false)
		{
			return false;
		}

		for (auto it = begin; it != end; ++it)
		{
			char c = *it;
			if (is_alnum(c) == false && c != '+' && c != '-' && c != '.')
			{
				return false;
			}
		}
		return true;
	}

	static const char* find_char(const char* begin, const char* end, char c)
	{
		for (auto it = begin; it != end; ++it)
		{
			if (*it == c)
			{
				return it;
			}
		}
		return end;
	}

	static bool is_hexdigit(char c)
	{
		return (is_num(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
	}

	static int get_hex_digit(char c)
	{
		if (c >= '0' && c <= '9')
		{
			return c - '0';
		}
		else if (c >= 'A' && c <= 'F')
		{
			return c - 'A' + 10;
		}
		else if (c >= 'a' && c <= 'f')
		{
			return c - 'a' + 10;
		}
		return -1;
	}

	static bool is_ipv6(const char* begin, const char* end)
	{
		size_t len = end - begin;
		if (len < 2 || len > 254)
		{
			return false;
		}
		for (auto it = begin; it != end; ++it)
		{
			char c = *it;
			if (c != ':' && c != '.' && is_hexdigit(c) == false)
			{
				return false;
			}
		}
		return true;
	}

	static bool is_ipv4(const char* begin, const char* end)
	{
		size_t len = end - begin;
		if (len < 7 || len > 254)
		{
			return false;
		}
		for (auto it = begin; it != end; ++it)
		{
			char c = *it;
			if (c != '.' && is_num(c) == false)
			{
				return false;
			}
		}
		return true;
	}

	static bool is_reg_domain_name(const char* begin, const char* end)
	{
		return is_allowed_str(begin, end, 0x01);
	}

	static bool is_port(const char* begin, const char* end)
	{
		if (begin == end || is_num(*begin) == false)
		{
			return false;
		}

		auto it = begin;
		uint32_t val = 0;
		for (; it != end; ++it)
		{
			char c = *it;
			val = val * 10 + (c - '0');
		}

		if (val > 0xFFFF)
		{
			return false;
		}
		return it == end;
	}

	static void normalize_scheme(String& scheme)
	{
		for (auto& c: scheme)
		{
			c = ::tolower(c);
		}
	}

	static bool decode_str(String& str)
	{
		if (str.count() == 0)
		{
			return true;
		}

		String res{str.allocator()};
		res.reserve(str.count());

		for (auto it = str.begin(); it != str.end(); ++it)
		{
			char c = *it;
			char a = 0, b = 0;
			if (c == '%')
			{
				++it;
				if (it == str.end())
				{
					return false;
				}
				c = *it;

				a = get_hex_digit(c);
				if (a == -1)
				{
					return false;
				}

				++it;
				if (it == str.end())
				{
					return false;
				}
				c = *it;

				b = get_hex_digit(c);
				if (b == -1)
				{
					return false;
				}

				c = (a << 4) | b;
			}
			res.pushByte(c);
		}

		str = std::move(res);
		return true;
	}

	static void normalize_reg_domain_name(String& scheme)
	{
		for (auto& c: scheme)
		{
			c = ::tolower(c);
		}
	}

	String normalize_ipv6(const char* begin, const char* end, Allocator* allocator)
	{
		if ((end - begin) == 2 && begin[0] == ':' && begin[1] == ':')
		{
			return String{StringView{begin, end}, allocator};
		}

		// Split IPv6 at colons
		const char* it = begin;
		const char* tokens[10];

		if (*it == ':')
		{
			++it;
		}

		if (end[-1] == ':')
		{
			--end;
		}

		const char* b = it;
		size_t i = 0;
		while (it != end)
		{
			if (*it++ == ':')
			{
				tokens[i++] = b;
				b = it;
			}
		}

		if (i < 8)
		{
			tokens[i++] = b;
		}

		tokens[i] = it;
		size_t ntokens = i;

		// Get IPv4 address which is normalized by default
		const char *ipv4_b = nullptr, *ipv4_e = nullptr;
		if ((tokens[ntokens] - tokens[ntokens - 1]) > 5)
		{
			ipv4_b = tokens[ntokens - 1];
			ipv4_e = tokens[ntokens];
			--ntokens;
		}

		// Decode the fields
		uint16_t fields[8];
		size_t null_pos = 8, null_len = 0, nfields = 0;
		for (size_t i = 0; i < ntokens; ++i)
		{
			const char* p = tokens[i];
			if (p == tokens[i + 1] || *p == ':')
			{
				null_pos = i;
			}
			else
			{
				uint16_t field = get_hex_digit(*p++);
				while (p != tokens[i + 1] && *p != ':')
				{
					field = (field << 4) | get_hex_digit(*p++);
				}
				fields[nfields++] = field;
			}
		}

		i = nfields;
		nfields = ipv4_b ? 6 : 8;
		if (i < nfields)
		{
			size_t last = nfields;
			if (i != null_pos)
			{
				do
				{
					fields[--last] = fields[--i];
				}
				while (i != null_pos);
			}
			do
			{
				fields[--last] = 0;
			}
			while (last != null_pos);
		}

		// locate first longer sequence of zero
		i = null_len = 0;
		null_pos = nfields;
		size_t first = 0;
		while (true)
		{
			while (i < nfields && fields[i] != 0)
			{
				++i;
			}

			if (i == nfields)
			{
				break;
			}

			first = i;
			while (i < nfields && fields[i] == 0)
			{
				++i;
			}

			if ((i - first) > null_len)
			{
				null_pos = first;
				null_len = i - first;
			}

			if (i == nfields)
			{
				break;
			}
		}

		if (null_len == 1)
		{
			null_pos = nfields;
			null_len = 1;
		}

		// Encode normalized IPv6
		MemoryStream stream{allocator};

		if (null_pos == 0)
		{
			strf(&stream, ":"_sv);
			i = null_len;
		}
		else
		{
			strf(&stream, "{:x}"_sv, fields[0]);
			for (i = 1; i < null_pos; ++i)
			{
				strf(&stream, ":{:x}"_sv, fields[i]);
			}

			if (i < nfields)
			{
				strf(&stream, ":"_sv);
			}

			i += null_len;
			if (i == 8 && null_len != 0)
			{
				strf(&stream, ":"_sv);
			}
		}

		for (; i < nfields; ++i)
		{
			strf(&stream, ":{:x}"_sv, fields[i]);
		}

		if (ipv4_b)
		{
			auto ipv4_str = StringView{ipv4_b, ipv4_e};
			strf(&stream, ":{}"_sv, ipv4_str);
		}

		return stream.releaseString();
	}

	static void normalize_path(String& path, Allocator* allocator)
	{
		if (path.count() == 0)
		{
			return;
		}

		auto parts = path.split("/"_sv, true, allocator);

		// filter parts without . and ..
		Array<StringView> parts_filtered{allocator};
		for (const auto& part: parts)
		{
			if (part == ""_sv || part == "."_sv)
			{
				continue;
			}
			else if (part == ".."_sv && parts_filtered.count() > 0)
			{
				parts_filtered.pop();
			}
			parts_filtered.push(part);
		}

		MemoryStream stream{allocator};

		if (path[0] == '/')
		{
			strf(&stream, "/"_sv);
		}

		for (size_t i = 0; i < parts_filtered.count(); ++i)
		{
			if (i != 0)
			{
				strf(&stream, "/"_sv);
			}
			strf(&stream, "{}"_sv, parts_filtered[i]);
		}

		path = stream.releaseString();
	}

	static bool decode_plus_str(String& str)
	{
		if (str.count() == 0)
		{
			return true;
		}

		String res{str.allocator()};
		res.reserve(str.count());

		for (auto it = str.begin(); it != str.end(); ++it)
		{
			char c = *it;
			char a = 0, b = 0;
			if (c == '+')
			{
				c = ' ';
			}
			else if (c == '%')
			{
				++it;
				if (it == str.end())
				{
					return false;
				}
				c = *it;

				a = get_hex_digit(c);
				if (a == -1)
				{
					return false;
				}

				++it;
				if (it == str.end())
				{
					return false;
				}
				c = *it;

				b = get_hex_digit(c);
				if (b == -1)
				{
					return false;
				}

				c = (a << 4) | b;
			}
			res.pushByte(c);
		}

		str = res;
		return true;
	}

	Result<UrlQuery> UrlQuery::parse(core::StringView query, core::Allocator* allocator)
	{
		UrlQuery res{allocator};

		auto begin = query.begin();
		auto end = query.end();
		auto it = begin;
		while (begin < end)
		{
			it = find_first_of(begin, end, "=;&");
			if (is_allowed_str(begin, it, 0x3F) == false)
			{
				return errf(allocator, "Invalid query string '{}'"_sv, StringView{begin, it});
			}

			String value{allocator};
			String key{StringView{begin, it}, allocator};
			decode_plus_str(key);

			if (it == end)
			{
				res.add(key, ""_sv);
				break;
			}

			if (*it == '=')
			{
				begin = it + 1;
				it = find_first_of(begin, end, ";&");
				if (is_allowed_str(begin, it, 0x3F) == false)
				{
					return errf(allocator, "invalid query string '{}'"_sv, StringView{begin, it});
				}
				value = String{StringView{begin, it}, allocator};
				decode_plus_str(value);
			}

			res.add(key, value);
			begin = it + 1;
		}

		return res;
	}

	UrlQuery::UrlQuery(core::Allocator* allocator)
		: m_allocator(allocator),
		  m_keyValues(allocator),
		  m_keyToIndex(allocator)
	{}

	void UrlQuery::add(StringView key, StringView value)
	{
		size_t index = m_keyValues.count();
		m_keyValues.emplace(String{key, m_allocator}, String{value, m_allocator});
		m_keyToIndex.insert(m_keyValues[m_keyValues.count() - 1].key, index);
	}

	UrlQuery::KeyConstIterator UrlQuery::find(StringView key) const
	{
		return m_keyToIndex.lookup(key);
	}

	StringView UrlQuery::get(KeyConstIterator it) const
	{
		assertTrue(it != m_keyToIndex.end());
		return m_keyValues[it->value].value;
	}

	String Url::encodeQueryElement(StringView str, Allocator* allocator)
	{
		String res{allocator};
		res.reserve(str.count());

		for (auto c: str)
		{
			if (c == ' ')
			{
				res.pushByte('+');
			}
			else if (c >= '!' && c <= ',')
			{
				res.pushByte('%');
				res.pushByte(HEX_DIGITS[c >> 4]);
				res.pushByte(HEX_DIGITS[c & 0xF]);
			}
			else if (c == '=')
			{
				res.push("%3D"_sv);
			}
			else if (c == ';')
			{
				res.push("%3B"_sv);
			}
			else if (isAllowedChar(c, 0x01))
			{
				res.pushByte(c);
			}
			else
			{
				res.pushByte('%');
				res.pushByte("0123456789ABCDEF"[uint8_t(c) >> 4]);
				res.pushByte("0123456789ABCDEF"[uint8_t(c) & 0xF]);
			}
		}

		return res;
	}

	Result<Url> Url::parse(StringView str, Allocator* allocator)
	{
		Url res{allocator};

		if (str.count() == 0)
		{
			return res;
		}

		if (str.count() > 8000)
		{
			return errf(allocator, "URL is longer than 8000 characters"_sv);
		}

		//          userinfo       host      port
		//          ┌──┴───┐ ┌──────┴──────┐ ┌┴┐
		//  https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top
		//  └─┬─┘   └───────────┬──────────────┘└───────┬───────┘ └───────────┬─────────────┘ └┬┘
		//  scheme          authority                  path                 query           fragment
		//  Note that authority can be IP literal
		//  ldap://[2001:db8::7]/c=GB?objectClass?one
		//  └┬─┘   └─────┬─────┘└─┬─┘ └──────┬──────┘
		//  scheme   authority   path      query

		// this might be string begin and end
		const char* str_end = str.end();

		// let's start by finding [scheme ':', path '/', query '?', fragment '#']
		const char *str_begin = str.begin(), *str_it = find_first_of(str_begin, str_end, ":/?#");

		const char* query_begin = nullptr;
		const char* query_end = nullptr;

		// if we didn't find any of the above things then this is a path
		// /forum/questions/
		if (str_it == str_end)
		{
			// check the characters in the path part if it has any unallowed character then there's an error
			if (!is_allowed_str(str_begin, str_it, 0x2F))
			{
				return errf(allocator, "Path is invalid"_sv);
			}

			// yay: we have path
			res.m_path = String{StringView{str_begin, str_end}, allocator};
		}
		else
		{
			// get schema if any
			// https
			if (*str_it == ':')
			{
				if (is_scheme(str_begin, str_it) == false)
				{
					return errf(allocator, "Scheme is invalid"_sv);
				}

				// yay: we have scheme
				res.m_scheme = String{StringView{str_begin, str_it}, allocator};

				// update the p pointer to the search for [path '/', query '?', fragment '#']
				str_begin = str_it + 1;
				str_it = find_first_of(str_begin, str_end, "/?#");
			}

			// get authority if any
			// john.doe@www.example.com:123
			if (str_it != str_end &&					   // authority shouldn't be empty
				*str_it == '/' &&						   // authority should end with /
				(str_end - str_begin) > 1 &&			   // authority shouldn't be empty
				str_begin[0] == '/' && str_begin[1] == '/' // authority should start with "//"
			)
			{
				str_begin += 2; // eat "//" at the start of authority

				// locate end of authority [path '/', query '?', fragment '#']
				const char* end_authority = find_first_of(str_begin, str_end, "/?#");

				// authority consists of [userinfo, host, port]

				// try to find user info
				str_it = find_char(str_begin, end_authority, '@');
				// get user info if any
				// john.doe
				if (str_it != end_authority)
				{
					if (is_allowed_str(str_begin, str_it, 0x2F) == false)
					{
						return errf(allocator, "User info is invalid"_sv);
					}

					// yay: we have user info
					res.m_user = String{StringView{str_begin, str_it}, allocator};

					// skip to after user info
					str_begin = str_it + 1;
				}

				// Get IP literal if any
				// [2001:db8::7]
				if (*str_begin == '[')
				{
					str_begin += 1; // eat "[" at the start of ip literal

					// locate end of IP literal
					str_it = find_char(str_begin, end_authority, ']');

					// we didn't find the end of ip literal
					if (*str_it != ']')
					{
						return errf(allocator, "missing ] in ipv6 literal"_sv);
					}

					// decode IPvFuture protocol version
					if (*str_begin == 'v')
					{
						// ip version size is 1 byte so we read two hex digits and combine them together
						if (is_hexdigit(*++str_begin))
						{
							res.m_ipVersion = get_hex_digit(*str_begin);
							str_begin += 1; // eat the first hex digit
							if (is_hexdigit(*str_begin))
							{
								res.m_ipVersion = (res.m_ipVersion << 8) | get_hex_digit(*str_begin);
							}
						}

						str_begin += 1; // eat second hex digit
						if (res.m_ipVersion == -1 || *str_begin != '.' ||
							is_allowed_str(str_begin, str_it, 0x05) == false)
						{
							return errf(allocator, "Host address is invalid"_sv);
						}
					}
					else if (is_ipv6(str_begin, str_it))
					{
						res.m_ipVersion = 6;
					}
					else
					{
						return errf(allocator, "Host address is invalid"_sv);
					}

					// if we get the version then all we have left is the host
					res.m_host = String{StringView{str_begin, str_it}, allocator};
					str_begin = str_it + 1; // skip to after the ip literal
				}
				// no ip literal found then this must be normal name
				// www.example.com:123
				else
				{
					// find the port ':' if it exists
					str_it = find_char(str_begin, end_authority, ':');

					// check if it ip version 4
					// 192.168.1.1
					if (is_ipv4(str_begin, str_it))
					{
						res.m_ipVersion = 4;
					}
					// domain name
					// www.website.com
					else if (is_reg_domain_name(str_begin, str_it))
					{
						res.m_ipVersion = 0;
					}
					// wrong url!!
					else
					{
						return errf(allocator, "Host address is invalid"_sv);
					}

					// yay: we have host
					res.m_host = String{StringView{str_begin, str_it}, allocator};
					str_begin = str_it; // skip to the end of the host
				}

				// get port if any
				// :8080
				if (str_begin != end_authority && *str_begin == ':')
				{
					str_begin += 1; // eat the ':'
					if (is_port(str_begin, end_authority) == false)
					{
						return errf(allocator, "Port is invalid"_sv);
					}
					// yay: we have a port
					res.m_port = String{StringView{str_begin, end_authority}, allocator};
				}

				str_begin = end_authority; // skip to the end of authority
			}

			// let's find any of the following [query '?', fragment '#']
			str_it = find_first_of(str_begin, str_end, "?#");

			// let's try finding the path
			if (is_allowed_str(str_begin, str_it, 0x2F) == false)
			{
				return errf(allocator, "Path is invalid"_sv);
			}

			// yay: we have path
			res.m_path = String{StringView{str_begin, str_it}, allocator};

			// try to find a query if it exists
			if (str_it != str_end && *str_it == '?')
			{
				str_begin = str_it + 1; // skip to start of query
				// find end of query [fragment '#'] or end of string
				str_it = find_char(str_begin, str_end, '#');

				// yay: we have query
				query_begin = str_begin;
				query_end = str_it;
			}

			// try to find a fragment if it exists
			if (str_it != str_end && *str_it == '#')
			{
				if (is_allowed_str(str_it + 1, str_end, 0x3F) == false)
				{
					return errf(allocator, "Fragment is invalid"_sv);
				}

				// yay: we have a fragment
				res.m_fragment = String{StringView{str_it + 1, str_end}, allocator};
			}
		}

		normalize_scheme(res.m_scheme);

		decode_str(res.m_user);

		// decode and normalize host
		decode_str(res.m_host);
		if (res.m_ipVersion == 0)
		{
			normalize_reg_domain_name(res.m_host);
		}
		else if (res.m_ipVersion == 6)
		{
			res.m_host = normalize_ipv6(res.m_host.begin(), res.m_host.end(), allocator);
		}

		// decode and normalize path
		decode_str(res.m_path);
		normalize_path(res.m_path, allocator);

		// parse query
		if (query_begin)
		{
			auto parsedQuery = UrlQuery::parse(StringView{query_begin, query_end}, allocator);
			if (parsedQuery.isError())
			{
				return parsedQuery.releaseError();
			}
			res.m_query = parsedQuery.releaseValue();
		}

		decode_str(res.m_fragment);
		return res;
	}

	Result<String> Url::toString(core::Allocator* allocator) const
	{
		MemoryStream stream{allocator};

		if (m_scheme.count() > 0)
		{
			strf(&stream, "{}:"_sv, m_scheme);
		}

		if (m_host.count() > 0)
		{
			strf(&stream, "//"_sv);
			if (m_user.count() > 0)
			{
				strf(&stream, "{}@"_sv, encodeString(m_user, 0x05, allocator));
			}

			if (m_ipVersion == 0 || m_ipVersion == 4)
			{
				strf(&stream, "{}"_sv, m_host);
			}
			else if (m_ipVersion == 6)
			{
				strf(&stream, "[{}]"_sv, m_host);
			}
			else
			{
				strf(&stream, "[v{:x}.{}]"_sv, m_ipVersion, m_host);
			}

			if (m_port.count() > 0)
			{
				if (isDefaultPort() == false)
				{
					strf(&stream, ":{}"_sv, m_port);
				}
			}
		}
		else
		{
			if (m_user.count() > 0)
			{
				return errf(allocator, "User is only allowed if host is present"_sv);
			}

			if (m_port.count() > 0)
			{
				return errf(allocator, "Port is only allowed if host is present"_sv);
			}

			if (m_path.count() > 0)
			{
				auto p = m_path.findFirstByte(":/"_sv);
				if (p != SIZE_MAX && m_path[p] == ':')
				{
					return errf(allocator, "Port is only allowed if host is present"_sv);
				}
			}
		}

		if (m_path.count() > 0)
		{
			if (m_path[0] != '/' && m_host.count() > 0)
			{
				return errf(allocator, "Path must start with '/' if host is empty"_sv);
			}
			strf(&stream, "{}"_sv, encodeString(m_path, 0x0F, allocator));
		}

		if (m_query.count() > 0)
		{
			strf(&stream, "?"_sv);
			for (size_t i = 0; i < m_query.count(); ++i)
			{
				if (m_query[i].key.count() == 0)
				{
					return errf(allocator, "Empty key in query"_sv);
				}

				if (i != 0)
				{
					strf(&stream, "&"_sv);
				}

				strf(&stream, "{}"_sv, encodeQueryElement(m_query[i].key, allocator));
				if (m_query[i].value.count() > 0)
				{
					strf(&stream, "={}"_sv, encodeQueryElement(m_query[i].value, allocator));
				}
			}
		}

		if (m_fragment.count() > 0)
		{
			strf(&stream, "#{}"_sv, encodeString(m_fragment, 0x1F, allocator));
		}

		return stream.releaseString();
	}

	Result<String> Url::pathWithQueryAndFragment(Allocator* allocator) const
	{
		MemoryStream stream{allocator};

		if (m_path.count() > 0)
		{
			if (m_path[0] != '/' && m_host.count() > 0)
			{
				return errf(allocator, "Path must start with '/' if host is empty"_sv);
			}
			strf(&stream, "{}"_sv, encodeString(m_path, 0x0F, allocator));
		}

		if (m_query.count() > 0)
		{
			strf(&stream, "?"_sv);
			for (size_t i = 0; i < m_query.count(); ++i)
			{
				if (m_query[i].key.count() == 0)
				{
					return errf(allocator, "Empty key in query"_sv);
				}

				if (i != 0)
				{
					strf(&stream, "&"_sv);
				}

				strf(&stream, "{}"_sv, encodeQueryElement(m_query[i].key, allocator));
				if (m_query[i].value.count() > 0)
				{
					strf(&stream, "={}"_sv, encodeQueryElement(m_query[i].value, allocator));
				}
			}
		}

		if (m_fragment.count() > 0)
		{
			strf(&stream, "#{}"_sv, encodeString(m_fragment, 0x1F, allocator));
		}

		return stream.releaseString();
	}
}