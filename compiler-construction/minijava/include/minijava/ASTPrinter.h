#pragma once

#include <core/Stream.h>

#include "minijava/AST.h"

namespace minijava
{
	class ASTPrinter: public ASTVisitor
	{
		core::Stream* m_stream = nullptr;
		size_t m_indent = 0;

		void enterScope()
		{
			++m_indent;
		}

		void leaveScope()
		{
			--m_indent;
		}

		void newLine()
		{
			core::strf(m_stream, "\n"_sv);
			for (size_t i = 0; i < m_indent; ++i)
			{
				core::strf(m_stream, "  "_sv);
			}
		}

	public:
		ASTPrinter(core::Stream* stream)
			: m_stream(stream)
		{}

		void visit(Expr* expr)
		{
			expr->visit(this);
		}

		void visitAndExpr(AndExpr* expr) override
		{
			core::strf(m_stream, "(and"_sv);
			enterScope();
			{
				newLine();
				visit(expr->operand1());
				newLine();
				visit(expr->operand2());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitLessThanExpr(LessThanExpr* expr) override
		{
			core::strf(m_stream, "(<"_sv);
			enterScope();
			{
				newLine();
				visit(expr->operand1());
				newLine();
				visit(expr->operand2());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitPlusExpr(PlusExpr* expr) override
		{
			core::strf(m_stream, "(+"_sv);
			enterScope();
			{
				newLine();
				visit(expr->operand1());
				newLine();
				visit(expr->operand2());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitMinusExpr(MinusExpr* expr) override
		{
			core::strf(m_stream, "(-"_sv);
			enterScope();
			{
				newLine();
				visit(expr->operand1());
				newLine();
				visit(expr->operand2());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitTimesExpr(TimesExpr* expr) override
		{
			core::strf(m_stream, "(*"_sv);
			enterScope();
			{
				newLine();
				visit(expr->operand1());
				newLine();
				visit(expr->operand2());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitNotExpr(NotExpr* expr) override
		{
			core::strf(m_stream, "(!"_sv);
			enterScope();
			{
				newLine();
				visit(expr->value());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitNewObjectExpr(NewObjectExpr* expr) override
		{
			core::strf(m_stream, "(new '{}')"_sv, expr->name()->token().text());
		}

		void visitNewArrayExpr(NewArrayExpr* expr) override
		{
			core::strf(m_stream, "(new int["_sv);
			enterScope();
			{
				newLine();
				visit(expr->length());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, "])"_sv);
		}

		void visitThisExpr(ThisExpr*) override
		{
			core::strf(m_stream, "(this)"_sv);
		}

		void visitIdentifierExpr(IdentifierExpr* expr) override
		{
			core::strf(m_stream, "(identifier '{}')"_sv, expr->name().text());
		}

		void visitFalseExpr(FalseExpr*) override
		{
			core::strf(m_stream, "(false)"_sv);
		}

		void visitTrueExpr(TrueExpr*) override
		{
			core::strf(m_stream, "(true)"_sv);
		}

		void visitIntLiteralExpr(IntLiteralExpr* expr) override
		{
			core::strf(m_stream, "(int '{}')"_sv, expr->value().text());
		}

		void visitArrayLookupExpr(ArrayLookupExpr* expr) override
		{
			core::strf(m_stream, "(array-lookup"_sv);
			enterScope();
			{
				newLine();
				visit(expr->name());
				newLine();
				visit(expr->index());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitArrayLengthExpr(ArrayLengthExpr* expr) override
		{
			core::strf(m_stream, "(array-length"_sv);
			enterScope();
			{
				newLine();
				visit(expr->name());
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}

		void visitCallExpr(CallExpr* expr) override
		{
			core::strf(m_stream, "(call"_sv);
			enterScope();
			{
				newLine();
				visit(expr->base());
				newLine();
				core::strf(m_stream, "{}"_sv, expr->name()->token().text());
				newLine();
				core::strf(m_stream, "(args"_sv);
				enterScope();
				{
					for (const auto& arg: expr->arguments())
					{
						newLine();
						visit(arg.get());
					}
				}
				leaveScope();
				newLine();
				core::strf(m_stream, ")"_sv);
			}
			leaveScope();
			newLine();
			core::strf(m_stream, ")"_sv);
		}
	};
}