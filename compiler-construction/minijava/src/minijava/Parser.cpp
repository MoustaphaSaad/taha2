#include "minijava/Parser.h"
#include "minijava/Unit.h"

namespace minijava
{
	Token Parser::lookahead(size_t k)
	{
		if (m_it + k < m_unit->tokens().count())
			return m_unit->tokens()[m_it + k];
		return Token{Token::KIND_NONE, ""_sv, Location{}};
	}

	Token Parser::look()
	{
		return lookahead(0);
	}

	Token Parser::eat()
	{
		if (m_it >= m_unit->tokens().count())
			return Token{Token::KIND_NONE, ""_sv, Location{}};
		auto token = m_unit->tokens()[m_it];
		++m_it;
		return token;
	}

	Token Parser::eatKind(Token::KIND kind)
	{
		if (auto token = look(); token.kind() == kind)
			return eat();
		return Token{Token::KIND_NONE, ""_sv, Location{}};
	}

	Token Parser::eatMust(Token::KIND kind)
	{
		if (m_it >= m_unit->tokens().count())
		{
			m_unit->pushError(errf(m_allocator, Location{}, "expected '{}' but found 'EOF'"_sv, kind));
			return Token{Token::KIND_NONE, ""_sv, Location{}};
		}

		auto token = eat();
		if (token.kind() == kind)
			return token;

		m_unit->pushError(errf(m_allocator, token.location(), "expected '{}' but found '{}'"_sv, kind, token.text()));
		return Token{Token::KIND_NONE, ""_sv, Location{}};
	}

	core::Unique<BlockStmt> Parser::parseBlockStmt()
	{
		return nullptr;
	}

	core::Unique<MainClass> Parser::parseMainClass()
	{
		eatMust(Token::KIND_KEYWORD_CLASS);
		auto name = eatMust(Token::KIND_ID);
		Token args{};
		core::Unique<BlockStmt> blockStmt;

		eatMust(Token::KIND_OPEN_BRACE);
		{
			eatMust(Token::KIND_KEYWORD_PUBLIC);
			eatMust(Token::KIND_KEYWORD_STATIC);
			eatMust(Token::KIND_KEYWORD_VOID);
			eatMust(Token::KIND_KEYWORD_MAIN);
			eatMust(Token::KIND_OPEN_PAREN);
			{
				eatMust(Token::KIND_KEYWORD_STRING);
				eatMust(Token::KIND_OPEN_BRACKET);
				eatMust(Token::KIND_CLOSE_BRACKET);
				args = eatMust(Token::KIND_ID);
			}
			eatMust(Token::KIND_CLOSE_PAREN);

			blockStmt = parseBlockStmt();
		}
		eatMust(Token::KIND_CLOSE_BRACE);

		return core::unique_from<MainClass>(m_allocator, Identifier{name}, Identifier{args}, std::move(blockStmt));
	}

	Parser::Parser(Unit* unit, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_unit(unit)
	{

	}

	core::Unique<Program> Parser::parse()
	{
//		auto mainClass = parseMainClass();
//		Program program{}
		return nullptr;
	}
}