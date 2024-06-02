#pragma once

#include <core/StringView.h>

#include <fmt/format.h>

namespace minijava
{
	struct Position
	{
		int line = 0;
		int column = 0;
	};

	class Unit;

	struct Location
	{
		Position position;
		core::StringView range;
		Unit* unit = nullptr;
	};

	class Token
	{
	public:
		enum KIND
		{
			KIND_NONE,
			KIND_EOF,

			// keywords
			KIND_KEYWORD_BOOLEAN,
			KIND_KEYWORD_CLASS,
			KIND_KEYWORD_ELSE,
			KIND_KEYWORD_FALSE,
			KIND_KEYWORD_IF,
			KIND_KEYWORD_INT,
			KIND_KEYWORD_LENGTH,
			KIND_KEYWORD_MAIN,
			KIND_KEYWORD_NEW,
			KIND_KEYWORD_PUBLIC,
			KIND_KEYWORD_RETURN,
			KIND_KEYWORD_STATIC,
			KIND_KEYWORD_THIS,
			KIND_KEYWORD_TRUE,
			KIND_KEYWORD_VOID,
			KIND_KEYWORD_WHILE,

			// operators
			KIND_OPERATOR_PLUS,
			KIND_OPERATOR_MINUS,
			KIND_OPERATOR_MULTIPLY,
			KIND_OPERATOR_LESS_THAN,
			KIND_OPERATOR_ASSIGN,
			KIND_OPERATOR_LOGIC_AND,
			KIND_OPERATOR_LOGIC_NOT,

			// separators
			KIND_SEMICOLON,
			KIND_DOT,
			KIND_COMMA,
			KIND_OPEN_BRACE,
			KIND_CLOSE_BRACE,
			KIND_OPEN_BRACKET,
			KIND_CLOSE_BRACKET,
			KIND_OPEN_PAREN,
			KIND_CLOSE_PAREN,

			// literals
			KIND_LITERAL_INT,

			// ID
			KIND_ID,

			// comments
			KIND_COMMENT,
		};

		Token(KIND kind, core::StringView text, Location location)
			: m_kind(kind),
			  m_text(text),
			  m_location(location)
		{}

		KIND kind() const { return m_kind; }
		core::StringView text() const { return m_text; }
		Location location() const { return m_location; }

	private:
		KIND m_kind = KIND_NONE;
		core::StringView m_text;
		Location m_location;
	};
}

namespace fmt
{
	template<>
	struct formatter<minijava::Token::KIND>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(minijava::Token::KIND kind, FormatContext& ctx)
		{
			const char* name = "";
			switch (kind)
			{
			case minijava::Token::KIND_NONE:
				name = "<NONE>";
				break;
			case minijava::Token::KIND_EOF:
				name = "<EOF>";
				break;

			case minijava::Token::KIND_KEYWORD_BOOLEAN:
				name = "boolean";
				break;
			case minijava::Token::KIND_KEYWORD_CLASS:
				name = "class";
				break;
			case minijava::Token::KIND_KEYWORD_ELSE:
				name = "else";
				break;
			case minijava::Token::KIND_KEYWORD_FALSE:
				name = "false";
				break;
			case minijava::Token::KIND_KEYWORD_IF:
				name = "if";
				break;
			case minijava::Token::KIND_KEYWORD_INT:
				name = "int";
				break;
			case minijava::Token::KIND_KEYWORD_LENGTH:
				name = "length";
				break;
			case minijava::Token::KIND_KEYWORD_MAIN:
				name = "main";
				break;
			case minijava::Token::KIND_KEYWORD_NEW:
				name = "new";
				break;
			case minijava::Token::KIND_KEYWORD_PUBLIC:
				name = "public";
				break;
			case minijava::Token::KIND_KEYWORD_RETURN:
				name = "return";
				break;
			case minijava::Token::KIND_KEYWORD_STATIC:
				name = "static";
				break;
			case minijava::Token::KIND_KEYWORD_THIS:
				name = "this";
				break;
			case minijava::Token::KIND_KEYWORD_TRUE:
				name = "true";
				break;
			case minijava::Token::KIND_KEYWORD_VOID:
				name = "void";
				break;
			case minijava::Token::KIND_KEYWORD_WHILE:
				name = "while";
				break;

			case minijava::Token::KIND_OPERATOR_PLUS:
				name = "+";
				break;
			case minijava::Token::KIND_OPERATOR_MINUS:
				name = "-";
				break;
			case minijava::Token::KIND_OPERATOR_MULTIPLY:
				name = "*";
				break;
			case minijava::Token::KIND_OPERATOR_LESS_THAN:
				name = "<";
				break;
			case minijava::Token::KIND_OPERATOR_ASSIGN:
				name = "=";
				break;
			case minijava::Token::KIND_OPERATOR_LOGIC_AND:
				name = "&&";
				break;
			case minijava::Token::KIND_OPERATOR_LOGIC_NOT:
				name = "!";
				break;

			case minijava::Token::KIND_SEMICOLON:
				name = ";";
				break;
			case minijava::Token::KIND_DOT:
				name = ".";
				break;
			case minijava::Token::KIND_COMMA:
				name = ",";
				break;
			case minijava::Token::KIND_OPEN_BRACE:
				name = "{";
				break;
			case minijava::Token::KIND_CLOSE_BRACE:
				name = "}";
				break;
			case minijava::Token::KIND_OPEN_BRACKET:
				name = "[";
				break;
			case minijava::Token::KIND_CLOSE_BRACKET:
				name = "]";
				break;
			case minijava::Token::KIND_OPEN_PAREN:
				name = "(";
				break;
			case minijava::Token::KIND_CLOSE_PAREN:
				name = ")";
				break;

			case minijava::Token::KIND_LITERAL_INT:
				name = "<LITERAL_INT>";
				break;

			case minijava::Token::KIND_ID:
				name = "id";
				break;

			case minijava::Token::KIND_COMMENT:
				name = "comment";
				break;

			default:
				name = "<UNKNOWN>";
				break;
			}
			return format_to(ctx.out(), fmt::runtime("{}"), name);
		}
	};
}