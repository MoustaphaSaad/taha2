#include "minijava/Scanner.h"
#include "minijava/Unit.h"

namespace minijava
{
	bool Scanner::isSpace(core::Rune r) const
	{
		return (r == ' ' || r == '\n' || r == '\r' || r == '\v' || r == '\t');
	}

	bool Scanner::eof() const
	{
		return m_it >= m_unit->content().count();
	}

	bool Scanner::eat()
	{
		if (eof())
		{
			m_unit->pushLine(m_unit->content().sliceRight(m_lineBegin));
			return false;
		}

		auto prevRune = m_rune;
		auto prevIt = m_it;
		auto nextPtr = core::Rune::next(m_unit->content().sliceRight(m_it).data());
		m_it = nextPtr - m_unit->content().data();
		m_rune = core::Rune::decode(m_unit->content().sliceRight(m_it).data());

		++m_position.column;
		if (prevRune == '\n')
		{
			m_position.column = 0;
			++m_position.line;
			m_unit->pushLine(m_unit->content().slice(m_lineBegin, prevIt));
			m_lineBegin = m_it;
		}
		return true;
	}

	void Scanner::skipWhitespace()
	{
		while (isSpace(m_rune))
		{
			if (eat() == false)
			{
				break;
			}
		}
	}

	core::StringView Scanner::scanID()
	{
		auto beginIt = m_it;
		while (m_rune.isLetter() || m_rune.isNumber() || m_rune == '_')
		{
			if (eat() == false)
			{
				break;
			}
		}
		return m_unit->content().slice(beginIt, m_it);
	}

	core::StringView Scanner::scanNumber()
	{
		auto beginIt = m_it;
		while (m_rune.isNumber())
		{
			if (eat() == false)
			{
				break;
			}
		}
		return m_unit->content().slice(beginIt, m_it);
	}

	core::StringView Scanner::scanSingleLineComment()
	{
		auto beginIt = m_it;
		while (m_rune == '\n')
		{
			if (m_rune == '\r')
			{
				if (eat() == false || m_rune == '\n')
				{
					break;
				}
			}

			if (eat() == false)
			{
				break;
			}
		}
		return m_unit->content().slice(beginIt, m_it);
	}

	core::StringView Scanner::scanMultiLineComment()
	{
		auto beginIt = m_it;
		while (true)
		{
			if (m_rune == '*')
			{
				if (eat() == false)
				{
					break;
				}

				if (m_rune == '/')
				{
					eat();
					break;
				}
			}
			else if (eat() == false)
			{
				break;
			}
		}
		return m_unit->content().slice(beginIt, m_it);
	}

	Token::KIND Scanner::inferKeywordType(core::StringView id)
	{
		if (id == "boolean"_sv)
		{
			return Token::KIND_KEYWORD_BOOLEAN;
		}
		else if (id == "class"_sv)
		{
			return Token::KIND_KEYWORD_CLASS;
		}
		else if (id == "else"_sv)
		{
			return Token::KIND_KEYWORD_ELSE;
		}
		else if (id == "false"_sv)
		{
			return Token::KIND_KEYWORD_FALSE;
		}
		else if (id == "if"_sv)
		{
			return Token::KIND_KEYWORD_IF;
		}
		else if (id == "int"_sv)
		{
			return Token::KIND_KEYWORD_INT;
		}
		else if (id == "String"_sv)
		{
			return Token::KIND_KEYWORD_STRING;
		}
		else if (id == "length"_sv)
		{
			return Token::KIND_KEYWORD_LENGTH;
		}
		else if (id == "main"_sv)
		{
			return Token::KIND_KEYWORD_MAIN;
		}
		else if (id == "new"_sv)
		{
			return Token::KIND_KEYWORD_NEW;
		}
		else if (id == "public"_sv)
		{
			return Token::KIND_KEYWORD_PUBLIC;
		}
		else if (id == "return"_sv)
		{
			return Token::KIND_KEYWORD_RETURN;
		}
		else if (id == "static"_sv)
		{
			return Token::KIND_KEYWORD_STATIC;
		}
		else if (id == "this"_sv)
		{
			return Token::KIND_KEYWORD_THIS;
		}
		else if (id == "true"_sv)
		{
			return Token::KIND_KEYWORD_TRUE;
		}
		else if (id == "void"_sv)
		{
			return Token::KIND_KEYWORD_VOID;
		}
		else if (id == "while"_sv)
		{
			return Token::KIND_KEYWORD_WHILE;
		}
		else
		{
			return Token::KIND_NONE;
		}
	}

	Scanner::Scanner(Unit* unit, core::Allocator* allocator)
		: m_unit(unit),
		  m_allocator(allocator)
	{
		if (m_unit->content().count() > 0)
		{
			m_rune = core::Rune::decode(m_unit->content().data() + m_it);
		}
	}

	Token Scanner::scan()
	{
		skipWhitespace();

		Location location{.position = m_position, .range = m_unit->content().slice(m_it, m_it), .unit = m_unit};

		if (eof())
		{
			m_unit->pushLine(m_unit->content().sliceRight(m_lineBegin));
			return Token{Token::KIND_EOF, ""_sv, location};
		}

		if (m_rune.isLetter() || m_rune == '_')
		{
			auto id = scanID();
			auto kind = inferKeywordType(id);
			if (kind == Token::KIND_NONE)
			{
				kind = Token::KIND_ID;
			}
			location.range = id;
			return Token{kind, id, location};
		}
		else if (m_rune.isNumber())
		{
			auto number = scanNumber();
			location.range = number;
			return Token{Token::KIND_LITERAL_INT, number, location};
		}
		else
		{
			auto beginIt = m_it;
			auto rune = m_rune;
			eat();

			switch (rune)
			{
			case '+':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPERATOR_PLUS, text, location};
			}
			case '-':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPERATOR_MINUS, text, location};
			}
			case '*':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPERATOR_MULTIPLY, text, location};
			}
			case '<':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPERATOR_LESS_THAN, text, location};
			}
			case '=':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPERATOR_ASSIGN, text, location};
			}
			case '&':
			{
				if (m_rune == '&')
				{
					eat();
					auto text = m_unit->content().slice(beginIt, m_it);
					location.range = text;
					return Token{Token::KIND_OPERATOR_LOGIC_AND, text, location};
				}
				break;
			}
			case '!':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPERATOR_LOGIC_NOT, text, location};
			}
			case ';':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_SEMICOLON, text, location};
			}
			case '.':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_DOT, text, location};
			}
			case ',':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_COMMA, text, location};
			}
			case '{':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPEN_BRACE, text, location};
			}
			case '}':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_CLOSE_BRACE, text, location};
			}
			case '[':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPEN_BRACKET, text, location};
			}
			case ']':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_CLOSE_BRACKET, text, location};
			}
			case '(':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_OPEN_PAREN, text, location};
			}
			case ')':
			{
				auto text = m_unit->content().slice(beginIt, m_it);
				location.range = text;
				return Token{Token::KIND_CLOSE_PAREN, text, location};
			}
			case '/':
			{
				if (m_rune == '/')
				{
					eat();
					auto text = scanSingleLineComment();
					location.range = m_unit->content().slice(beginIt, m_it);
					return Token{Token::KIND_COMMENT, text, location};
				}
				else if (m_rune == '*')
				{
					eat();
					auto text = scanMultiLineComment();
					location.range = m_unit->content().slice(beginIt, m_it);
					return Token{Token::KIND_COMMENT, text, location};
				}
				else
				{
					break;
				}
			}
			}

			location.range = m_unit->content().slice(beginIt, m_it);
			m_unit->pushError(errf(m_allocator, location, "illegal token '{}'"_sv, location.range));
			return Token{Token::KIND_NONE, ""_sv, location};
		}
	}
}