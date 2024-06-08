#pragma once

#include "minijava/AST.h"

namespace minijava
{
	class Unit;

	class Parser
	{
		core::Allocator* m_allocator = nullptr;
		Unit* m_unit = nullptr;
		size_t m_it = 0;

		Token lookahead(size_t k);
		Token look();
		Token eat();
		Token eatKind(Token::KIND kind);
		Token eatMust(Token::KIND kind);
		core::Array<core::Unique<Expr>> parseArgs();
		core::Unique<Expr> parseAtomExpr();
		core::Unique<Expr> parseBaseExpr();
		core::Unique<Expr> parseUnaryExpr();
		core::Unique<Expr> parseMulExpr();
		core::Unique<Expr> parseAddExpr();
		core::Unique<Expr> parseCmpExpr();
		core::Unique<Expr> parseAndExpr();
		core::Unique<Stmt> parseStmt();
		core::Unique<BlockStmt> parseBlockStmt();
		core::Unique<IfStmt> parseIfStmt();
		core::Unique<MainClass> parseMainClass();
	public:
		explicit Parser(Unit* unit, core::Allocator* allocator);

		core::Unique<Program> parse();
		core::Unique<Expr> parseExpr();
	};
}