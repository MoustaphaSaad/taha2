#pragma once

#include <core/StringView.h>

#include <cstdint>

namespace sabre
{
	struct Position
	{
		uint32_t line = 0, column = 0;
	};

	struct UnitFile;

	struct Location
	{
		Position position;
		core::StringView sub_str;
		UnitFile* file = nullptr;
	};

	struct Token
	{
		enum KIND
		{
			KIND_NULL,
			KIND_EOF,
			KIND_ID,
			KIND_COMMENT,
			KIND_LITERAL_INTEGER,
			KIND_LITERAL_FLOAT,
			KIND_LITERAL_STRING,
			KIND_OPEN_PAREN,
			KIND_CLOSE_PAREN,
			KIND_OPEN_CURLY,
			KIND_CLOSE_CURLY,
			KIND_OPEN_BRACKET,
			KIND_CLOSE_BRACKET,
			KIND_COLON,
			KIND_SEMICOLON,
			KIND_DOT,
			KIND_LESS,
			KIND_GREATER,
			KIND_LESS_EQUAL,
			KIND_GREATER_EQUAL,
			KIND_EQUAL,
			KIND_EQUAL_EQUAL,
			KIND_NOT_EQUAL,
			KIND_PLUS,
			KIND_MINUS,
			KIND_STAR,
			KIND_DIVIDE,
			KIND_MODULUS,
			KIND_PLUS_EQUAL,
			KIND_MINUS_EQUAL,
			KIND_STAR_EQUAL,
			KIND_DIVIDE_EQUAL,
			KIND_MODULUS_EQUAL,
			KIND_LOGICAL_OR,
			KIND_LOGICAL_AND,
			KIND_LOGICAL_NOT,
			KIND_BIT_XOR,
			KIND_BIT_OR,
			KIND_BIT_AND,
			KIND_BIT_NOT,
			KIND_BIT_SHIFT_LEFT,
			KIND_BIT_SHIFT_RIGHT,
			KIND_BIT_XOR_EQUAL,
			KIND_BIT_OR_EQUAL,
			KIND_BIT_AND_EQUAL,
			KIND_BIT_NOT_EQUAL,
			KIND_BIT_SHIFT_LEFT_EQUAL,
			KIND_BIT_SHIFT_RIGHT_EQUAL,
			KIND_INC,
			KIND_DEC,
			KIND_COMMA,
			KIND_AT,
			KIND_KEYWORDS__BEGIN,
			KIND_KEYWORD_CONST = KIND_KEYWORDS__BEGIN,
			KIND_KEYWORD_VAR,
			KIND_KEYWORD_TYPE,
			KIND_KEYWORD_STRUCT,
			KIND_KEYWORD_FUNC,
			KIND_KEYWORD_RETURN,
			KIND_KEYWORD_DISCARD,
			KIND_KEYWORD_CONTINUE,
			KIND_KEYWORD_BREAK,
			KIND_KEYWORD_IMPORT,
			KIND_KEYWORD_IF,
			KIND_KEYWORD_ELSE,
			KIND_KEYWORD_FOR,
			KIND_KEYWORD_FALSE,
			KIND_KEYWORD_TRUE,
			KIND_KEYWORD_ENUM,
			KIND_KEYWORD_PACKAGE,
			KIND_KEYWORDS__END,
		};

		KIND kind = KIND_NULL;
		Location location;

		bool is_null() const
		{
			return kind == KIND_NULL;
		}

		bool can_ignore() const
		{
			return kind == KIND_COMMENT;
		}

		bool is_cmp() const
		{
			return (
				kind == KIND_LESS || kind == KIND_GREATER || kind == KIND_LESS_EQUAL || kind == KIND_GREATER_EQUAL ||
				kind == KIND_EQUAL_EQUAL || kind == KIND_NOT_EQUAL);
		}

		bool is_add() const
		{
			return (kind == KIND_PLUS || kind == KIND_MINUS || kind == KIND_BIT_XOR || kind == KIND_BIT_OR);
		}

		bool is_unary() const
		{
			return (
				kind == KIND_INC || kind == KIND_DEC || kind == KIND_PLUS || kind == KIND_MINUS ||
				kind == KIND_LOGICAL_NOT || kind == KIND_BIT_NOT);
		}

		bool is_mul() const
		{
			return (
				kind == KIND_STAR || kind == KIND_DIVIDE || kind == KIND_MODULUS || kind == KIND_BIT_AND ||
				kind == KIND_BIT_SHIFT_LEFT || kind == KIND_BIT_SHIFT_RIGHT);
		}

		bool is_assign() const
		{
			return (
				kind == KIND_EQUAL || kind == KIND_PLUS_EQUAL || kind == KIND_MINUS_EQUAL || kind == KIND_STAR_EQUAL ||
				kind == KIND_DIVIDE_EQUAL || kind == KIND_MODULUS_EQUAL || kind == KIND_BIT_OR_EQUAL ||
				kind == KIND_BIT_AND_EQUAL || kind == KIND_BIT_XOR_EQUAL || kind == KIND_BIT_SHIFT_LEFT_EQUAL ||
				kind == KIND_BIT_SHIFT_RIGHT_EQUAL || kind == KIND_BIT_SHIFT_NOT_EQUAL ||);
		}

		bool is_keyword() const
		{
			return (kind >= KIND_KEYWORDS__BEGIN && kind < KIND_KEYWORDS__END);
		}
	};
}