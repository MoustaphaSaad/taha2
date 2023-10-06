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
		0,	  0,	0,	  0,	0,	  0,	0,	  0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr static auto HEX_DIGITS = "0123456789ABCDEF";

	static bool isAllowedChar(char c, uint8_t mask)
	{
		return (ALLOWED_CHAR[uint8_t(c)] & mask) != 0;
	}

	static bool isAllowedStr(StringView str, uint8_t mask)
	{
		for (auto b: str)
			if (isAllowedChar(b, mask) == false)
				return false;
		return true;
	}

	static int hexDigit(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		else if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		return -1;
	}

	static Result<String> decodePlusStr(StringView str, Allocator* allocator)
	{
		if (str.count() == 0)
			return String{allocator};

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
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);
				c = str[i];

				auto a = hexDigit(c);
				if (a == -1)
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);

				++i;
				if (i >= str.count())
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);
				c = str[i];

				auto b = hexDigit(c);
				if (b == -1)
					return errf(allocator, "Invalid percent encoding in '{}'"_sv, str);

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

	Result<UrlQuery> UrlQuery::parse(core::StringView query, core::Allocator *allocator)
	{
		UrlQuery res{allocator};

		auto q = query;
		size_t ix = 0;
		while (q.count() > 0)
		{
			ix = q.findFirstByte("=;&"_sv);
			auto keySlice = q.slice(0, ix);
			if (isAllowedStr(keySlice, 0x3F) == false)
				return errf(allocator, "Invalid character in query key '{}'"_sv, keySlice);

			auto decodedKey = decodePlusStr(keySlice, allocator);
			if (decodedKey.isError())
				return decodedKey.releaseError();

			if (ix == q.count())
			{
				auto key = decodedKey.releaseValue();
				res.m_keyToIndex.insert(StringView{key}, res.m_keyValues.count());
				res.m_keyValues.emplace(KeyValue{std::move(key), String{allocator}});
				break;
			}

			if (q[ix] == '=')
			{
				q = q.slice(ix + 1, q.count());
				ix = q.findFirstByte(";&"_sv);
				auto valueSlice = q.slice(0, ix);
				if (isAllowedStr(valueSlice, 0x3F) == false)
					return errf(allocator, "Invalid character in query value '{}'"_sv, valueSlice);

				auto decodedValue = decodePlusStr(valueSlice, allocator);
				if (decodedValue.isError())
					return decodedValue.releaseError();

				auto key = decodedKey.releaseValue();
				auto value = decodedValue.releaseValue();
				res.m_keyToIndex.insert(StringView{key}, res.m_keyValues.count());
				res.m_keyValues.emplace(KeyValue{std::move(key), std::move(value)});
			}
			else
			{
				auto key = decodedKey.releaseValue();
				res.m_keyToIndex.insert(StringView{key}, res.m_keyValues.count());
				res.m_keyValues.emplace(KeyValue{std::move(key), String{allocator}});
			}
		}

		return res;
	}

	UrlQuery::UrlQuery(core::Allocator *allocator)
		: m_keyValues(allocator), m_keyToIndex(allocator)
	{
	}

	void UrlQuery::add(StringView key, StringView value)
	{
		size_t index = m_keyValues.count();
		m_keyValues.push(KeyValue{String{key, m_keyValues.allocator()}, String{value, m_keyValues.allocator()}});
		m_keyToIndex.insert(key, index);
	}

	const UrlQuery::KeyIterator UrlQuery::find(StringView key) const
	{
		return m_keyToIndex.lookup(key);
	}

	StringView UrlQuery::get(const KeyIterator it) const
	{
		assert(it != m_keyToIndex.end());
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

	Result<String> Url::toString(core::Allocator *allocator) const
	{
		MemoryStream stream{allocator};

		if (m_scheme.count() > 0)
			strf(&stream, "{}:"_sv, m_scheme);

		if (m_host.count() > 0)
		{
			strf(&stream, "//"_sv);
			if (m_user.count() > 0)
				strf(&stream, "{}@"_sv, encodeString(m_user, 0x05, allocator));

			if (m_ipVersion == 0 || m_ipVersion == 4)
				strf(&stream, "{}"_sv, m_host);
			else if (m_ipVersion == 6)
				strf(&stream, "[{}]"_sv, m_host);
			else
				strf(&stream, "[v{:x}.{}]"_sv, m_ipVersion, m_host);

			if (m_port.count() > 0)
				if (isDefaultPort() == false)
					strf(&stream, ":{}"_sv, m_port);
		}
		else
		{
			if (m_user.count() > 0)
				return errf(allocator, "User is only allowed if host is present"_sv);

			if (m_port.count() > 0)
				return errf(allocator, "Port is only allowed if host is present"_sv);

			if (m_path.count() > 0)
			{
				auto p = m_path.findFirstByte(":/"_sv);
				if (p != SIZE_MAX && m_path[p] == ':')
					return errf(allocator, "Port is only allowed if host is present"_sv);
			}
		}

		if (m_path.count() > 0)
		{
			if (m_path[0] != '/' && m_host.count() > 0)
				return errf(allocator, "Path must start with '/' if host is empty"_sv);
			strf(&stream, "{}"_sv, encodeString(m_path, 0x0F, allocator));
		}

		if (m_query.count() > 0)
		{
			strf(&stream, "?"_sv);
			for (size_t i = 0; i < m_query.count(); ++i)
			{
				if (m_query[i].key.count() == 0)
					return errf(allocator, "Empty key in query"_sv);

				if (i != 0)
					strf(&stream, "&"_sv);

				strf(&stream, "{}"_sv, encodeQueryElement(m_query[i].key, allocator));
				if (m_query[i].value.count() > 0)
					strf(&stream, "={}"_sv, encodeQueryElement(m_query[i].value, allocator));
			}
		}

		if (m_fragment.count() > 0)
			strf(&stream, "#{}"_sv, encodeString(m_fragment, 0x1F, allocator));

		return stream.releaseString();
	}
}