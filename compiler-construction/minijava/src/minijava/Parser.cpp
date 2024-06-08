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

	core::Array<core::Unique<Expr>> Parser::parseArgs()
	{
		core::Array<core::Unique<Expr>> arguments{m_allocator};
		if (eatKind(Token::KIND_OPEN_PAREN).kind() == Token::KIND_OPEN_PAREN)
		{
			if (eatKind(Token::KIND_CLOSE_PAREN).kind() != Token::KIND_CLOSE_PAREN)
			{
				while (true)
				{
					if (auto arg = parseExpr())
						arguments.push(std::move(arg));

					if (eatKind(Token::KIND_COMMA).kind() != Token::KIND_COMMA)
						break;
				}
				eatMust(Token::KIND_CLOSE_PAREN);
			}
		}
		return arguments;
	}

	core::Unique<Expr> Parser::parseAtomExpr()
	{
		auto token = look();
		core::Unique<Expr> expr;
		if (token.kind() == Token::KIND_LITERAL_INT)
		{
			expr = core::unique_from<IntLiteralExpr>(m_allocator, eat());
		}
		else if (token.kind() == Token::KIND_KEYWORD_TRUE)
		{
			expr = core::unique_from<TrueExpr>(m_allocator, eat());
		}
		else if (token.kind() == Token::KIND_KEYWORD_FALSE)
		{
			expr = core::unique_from<FalseExpr>(m_allocator, eat());
		}
		else if (token.kind() == Token::KIND_KEYWORD_THIS)
		{
			expr = core::unique_from<ThisExpr>(m_allocator, eat());
		}
		else if (token.kind() == Token::KIND_ID)
		{
			expr = core::unique_from<IdentifierExpr>(m_allocator, eat());
		}
		else if (token.kind() == Token::KIND_KEYWORD_NEW)
		{
			if (auto className = eatMust(Token::KIND_ID); className.kind() == Token::KIND_ID)
			{
				eatMust(Token::KIND_OPEN_PAREN);
				eatMust(Token::KIND_CLOSE_PAREN);
				expr = core::unique_from<NewObjectExpr>(m_allocator, Identifier{className});
			}
			else if (auto intToken = eatMust(Token::KIND_KEYWORD_INT); intToken.kind() == Token::KIND_KEYWORD_INT)
			{
				eatMust(Token::KIND_OPEN_BRACKET);
				auto arrayCount = parseExpr();
				eatMust(Token::KIND_CLOSE_BRACKET);
				expr = core::unique_from<NewArrayExpr>(m_allocator, std::move(arrayCount));
			}
			else
			{
				auto token = look();
				m_unit->pushError(errf(m_allocator, token.location(), "unknown new expression type '{}'"_sv, token.text()));
			}
		}
		else if (token.kind() == Token::KIND_OPEN_PAREN)
		{
			eat();
			expr = parseExpr();
			eatMust(Token::KIND_CLOSE_PAREN);
		}
		else
		{
			m_unit->pushError(errf(m_allocator, token.location(), "unknown expression '{}'"_sv, token.text()));
		}
		return expr;
	}

	core::Unique<Expr> Parser::parseBaseExpr()
	{
		auto expr = parseAtomExpr();

		while (true)
		{
			if (eatKind(Token::KIND_DOT).kind() == Token::KIND_DOT)
			{
				if (eatKind(Token::KIND_KEYWORD_LENGTH).kind() == Token::KIND_KEYWORD_LENGTH)
				{
					expr = core::unique_from<ArrayLengthExpr>(m_allocator, std::move(expr));
				}
				else if (auto call = eatKind(Token::KIND_ID); call.kind() == Token::KIND_ID)
				{
					expr = core::unique_from<CallExpr>(m_allocator, std::move(expr), Identifier{call}, parseArgs());
				}
				else
				{
					break;
				}
			}
			else if (eatKind(Token::KIND_OPEN_BRACKET).kind() == Token::KIND_OPEN_BRACKET)
			{
				auto index = parseExpr();
				eatMust(Token::KIND_CLOSE_BRACKET);
				expr = core::unique_from<ArrayLookupExpr>(m_allocator, std::move(expr), std::move(index));
			}
			else
			{
				break;
			}
		}

		return expr;
	}

	core::Unique<Expr> Parser::parseUnaryExpr()
	{
		core::Unique<Expr> expr;

		if (look().kind() == Token::KIND_OPERATOR_LOGIC_NOT)
		{
			auto op = eat();
			expr = core::unique_from<NotExpr>(m_allocator, parseUnaryExpr());
		}
		else
		{
			expr = parseBaseExpr();
		}

		return expr;
	}

	core::Unique<Expr> Parser::parseMulExpr()
	{
		auto expr = parseUnaryExpr();

		while (look().kind() == Token::KIND_OPERATOR_MULTIPLY)
		{
			auto op = eat();
			auto rhs = parseUnaryExpr();
			if (rhs == nullptr)
			{
				m_unit->pushError(errf(m_allocator, op.location(), "missing right hand side"_sv));
				break;
			}

			expr = core::unique_from<TimesExpr>(m_allocator, std::move(expr), std::move(rhs));
		}

		return expr;
	}

	core::Unique<Expr> Parser::parseAddExpr()
	{
		auto isAddToken = +[](Token::KIND kind) {
			return (
				kind == Token::KIND_OPERATOR_PLUS ||
				kind == Token::KIND_OPERATOR_MINUS
			);
		};
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

	core::Unique<Expr> Parser::parseExpr()
	{
		return parseAndExpr();
	}
}