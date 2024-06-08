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

	core::Unique<Expr> Parser::parseMulExpr()
	{
		return nullptr;
	}

	core::Unique<Expr> Parser::parseAddExpr()
	{
		auto isAddToken = +[](Token::KIND kind) {
			return (
				kind == Token::KIND_OPERATOR_PLUS ||
				kind == Token::KIND_OPERATOR_MINUS
			);
		};
		auto token = look();
		auto expr = parseMulExpr();

		while (isAddToken(look().kind()))
		{
			auto op = eat();
			auto rhs = parseMulExpr();
			if (rhs == nullptr)
			{
				m_unit->pushError(errf(m_allocator, op.location(), "missing right hand side"_sv));
				break;
			}
			if (op.kind() == Token::KIND_OPERATOR_PLUS)
			{
				expr = core::unique_from<PlusExpr>(m_allocator, std::move(expr), std::move(rhs));
			}
			else if (op.kind() == Token::KIND_OPERATOR_MINUS)
			{
				expr = core::unique_from<MinusExpr>(m_allocator, std::move(expr), std::move(rhs));
			}
			else
			{
				coreUnreachable();
			}
		}

		return expr;
	}

	core::Unique<Expr> Parser::parseCmpExpr()
	{
		auto token = look();
		auto expr = parseAddExpr();

		if (look().kind() == Token::KIND_OPERATOR_LESS_THAN)
		{
			auto op = eat();
			auto rhs = parseAddExpr();
			if (rhs == nullptr)
			{
				m_unit->pushError(errf(m_allocator, op.location(), "missing right hand side"_sv));
			}
			else
			{
				expr = core::unique_from<LessThanExpr>(m_allocator, std::move(expr), std::move(rhs));
			}
		}

		return expr;
	}

	core::Unique<Expr> Parser::parseAndExpr()
	{
		auto token = look();
		auto expr = parseCmpExpr();
		while (true)
		{
			auto andToken = eatKind(Token::KIND_OPERATOR_LOGIC_AND);
			if (andToken.kind() == Token::KIND_OPERATOR_LOGIC_AND)
			{
				auto rhs = parseCmpExpr();
				if (rhs == nullptr)
				{
					m_unit->pushError(errf(m_allocator, andToken.location(), "missing right hand side"_sv));
					break;
				}

				expr = core::unique_from<AndExpr>(m_allocator, std::move(expr), std::move(rhs));
			}
			else
			{
				break;
			}
		}

		return expr;
	}

	core::Unique<Expr> Parser::parseExpr()
	{
		return parseAndExpr();
	}

	core::Unique<Stmt> Parser::parseStmt()
	{
		auto token = look();
		if (token.kind() == Token::KIND_OPEN_BRACE)
		{
			return parseBlockStmt();
		}
		else if (token.kind() == Token::KIND_KEYWORD_IF)
		{
			// parse if statements
			return nullptr;
		}
		else if (token.kind() == Token::KIND_KEYWORD_WHILE)
		{
			// parse while statements
			return nullptr;
		}
		else
		{
			return nullptr;
		}
	}

	core::Unique<BlockStmt> Parser::parseBlockStmt()
	{
		core::Array<core::Unique<Stmt>> statements{m_allocator};

		eatMust(Token::KIND_OPEN_BRACE);
		{
			while (auto stmt = parseStmt())
			{
				statements.push(std::move(stmt));
				eatMust(Token::KIND_SEMICOLON);
			}
		}
		eatMust(Token::KIND_CLOSE_BRACE);

		return core::unique_from<BlockStmt>(m_allocator, std::move(statements));
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