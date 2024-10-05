#pragma once

#include "minijava/Token.h"

#include <core/Rune.h>

namespace minijava
{
	class Unit;

	class Scanner
	{
		core::Allocator* m_allocator = nullptr;
		Unit* m_unit = nullptr;
		size_t m_it = 0;
		core::Rune m_rune = core::Rune{0};
		Position m_position{1, 0};
		size_t m_lineBegin = 0;

		bool isSpace(core::Rune r) const;
		bool eof() const;
		bool eat();
		void skipWhitespace();
		core::StringView scanID();
		core::StringView scanNumber();
		core::StringView scanSingleLineComment();
		core::StringView scanMultiLineComment();
		Token::KIND inferKeywordType(core::StringView id);

	public:
		explicit Scanner(Unit* unit, core::Allocator* allocator);

		Token scan();
	};
}