#pragma once

#include "minijava/Token.h"

#include <core/Unique.h>

namespace minijava
{
	class AndExpr;
	class LessThanExpr;
	class PlusExpr;
	class MinusExpr;
	class TimesExpr;
	class NotExpr;
	class NewObjectExpr;
	class NewArrayExpr;
	class ThisExpr;
	class IdentifierExpr;
	class FalseExpr;
	class TrueExpr;
	class IntLiteralExpr;
	class ArrayLookupExpr;
	class ArrayLengthExpr;
	class CallExpr;

	class ASTVisitor
	{
	public:
		virtual void visitAndExpr(AndExpr*) {}
		virtual void visitLessThanExpr(LessThanExpr*) {}
		virtual void visitPlusExpr(PlusExpr*) {}
		virtual void visitMinusExpr(MinusExpr*) {}
		virtual void visitTimesExpr(TimesExpr*) {}
		virtual void visitNotExpr(NotExpr*) {}
		virtual void visitNewObjectExpr(NewObjectExpr*) {}
		virtual void visitNewArrayExpr(NewArrayExpr*) {}
		virtual void visitThisExpr(ThisExpr*) {}
		virtual void visitIdentifierExpr(IdentifierExpr*) {}
		virtual void visitFalseExpr(FalseExpr*) {}
		virtual void visitTrueExpr(TrueExpr*) {}
		virtual void visitIntLiteralExpr(IntLiteralExpr*) {}
		virtual void visitArrayLookupExpr(ArrayLookupExpr*) {}
		virtual void visitArrayLengthExpr(ArrayLengthExpr*) {}
		virtual void visitCallExpr(CallExpr*) {}
	};

	class Identifier
	{
		Token m_token;

	public:
		explicit Identifier(Token token)
			: m_token(std::move(token))
		{}

		Token token() const
		{
			return m_token;
		}
	};

	// expr
	class Expr
	{
	public:
		virtual ~Expr() = default;
		virtual void visit(ASTVisitor* visitor) = 0;
	};

	class AndExpr: public Expr
	{
		core::Unique<Expr> m_operand1;
		core::Unique<Expr> m_operand2;

	public:
		AndExpr(core::Unique<Expr> op1, core::Unique<Expr> op2)
			: m_operand1(std::move(op1)),
			  m_operand2(std::move(op2))
		{}

		Expr* operand1() const
		{
			return m_operand1.get();
		}
		Expr* operand2() const
		{
			return m_operand2.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitAndExpr(this);
		}
	};

	class LessThanExpr: public Expr
	{
		core::Unique<Expr> m_operand1;
		core::Unique<Expr> m_operand2;

	public:
		LessThanExpr(core::Unique<Expr> op1, core::Unique<Expr> op2)
			: m_operand1(std::move(op1)),
			  m_operand2(std::move(op2))
		{}

		Expr* operand1() const
		{
			return m_operand1.get();
		}
		Expr* operand2() const
		{
			return m_operand2.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitLessThanExpr(this);
		}
	};

	class PlusExpr: public Expr
	{
		core::Unique<Expr> m_operand1;
		core::Unique<Expr> m_operand2;

	public:
		PlusExpr(core::Unique<Expr> op1, core::Unique<Expr> op2)
			: m_operand1(std::move(op1)),
			  m_operand2(std::move(op2))
		{}

		Expr* operand1() const
		{
			return m_operand1.get();
		}
		Expr* operand2() const
		{
			return m_operand2.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitPlusExpr(this);
		}
	};

	class MinusExpr: public Expr
	{
		core::Unique<Expr> m_operand1;
		core::Unique<Expr> m_operand2;

	public:
		MinusExpr(core::Unique<Expr> op1, core::Unique<Expr> op2)
			: m_operand1(std::move(op1)),
			  m_operand2(std::move(op2))
		{}

		Expr* operand1() const
		{
			return m_operand1.get();
		}
		Expr* operand2() const
		{
			return m_operand2.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitMinusExpr(this);
		}
	};

	class TimesExpr: public Expr
	{
		core::Unique<Expr> m_operand1;
		core::Unique<Expr> m_operand2;

	public:
		TimesExpr(core::Unique<Expr> op1, core::Unique<Expr> op2)
			: m_operand1(std::move(op1)),
			  m_operand2(std::move(op2))
		{}

		Expr* operand1() const
		{
			return m_operand1.get();
		}
		Expr* operand2() const
		{
			return m_operand2.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitTimesExpr(this);
		}
	};

	class NotExpr: public Expr
	{
		core::Unique<Expr> m_value;

	public:
		NotExpr(core::Unique<Expr> value)
			: m_value(std::move(value))
		{}

		Expr* value() const
		{
			return m_value.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitNotExpr(this);
		}
	};

	class NewObjectExpr: public Expr
	{
		Identifier m_name;

	public:
		NewObjectExpr(Identifier name)
			: m_name(std::move(name))
		{}

		const Identifier* name() const
		{
			return &m_name;
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitNewObjectExpr(this);
		}
	};

	class NewArrayExpr: public Expr
	{
		core::Unique<Expr> m_length;

	public:
		NewArrayExpr(core::Unique<Expr> length)
			: m_length(std::move(length))
		{}

		Expr* length() const
		{
			return m_length.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitNewArrayExpr(this);
		}
	};

	class ThisExpr: public Expr
	{
		Token m_token;

	public:
		ThisExpr(Token token)
			: m_token(token)
		{}

		Token token() const
		{
			return m_token;
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitThisExpr(this);
		}
	};

	class IdentifierExpr: public Expr
	{
		Token m_name;

	public:
		IdentifierExpr(Token name)
			: m_name(std::move(name))
		{}

		Token name() const
		{
			return m_name;
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitIdentifierExpr(this);
		}
	};

	class FalseExpr: public Expr
	{
		Token m_token;

	public:
		FalseExpr(Token token)
			: m_token(token)
		{}

		Token token() const
		{
			return m_token;
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitFalseExpr(this);
		}
	};

	class TrueExpr: public Expr
	{
		Token m_token;

	public:
		TrueExpr(Token token)
			: m_token(token)
		{}

		Token token() const
		{
			return m_token;
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitTrueExpr(this);
		}
	};

	class IntLiteralExpr: public Expr
	{
		Token m_value;

	public:
		IntLiteralExpr(Token value)
			: m_value(value)
		{}

		Token value() const
		{
			return m_value;
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitIntLiteralExpr(this);
		}
	};

	class ArrayLookupExpr: public Expr
	{
		core::Unique<Expr> m_name;
		core::Unique<Expr> m_index;

	public:
		ArrayLookupExpr(core::Unique<Expr> op1, core::Unique<Expr> op2)
			: m_name(std::move(op1)),
			  m_index(std::move(op2))
		{}

		Expr* name() const
		{
			return m_name.get();
		}
		Expr* index() const
		{
			return m_index.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitArrayLookupExpr(this);
		}
	};

	class ArrayLengthExpr: public Expr
	{
		core::Unique<Expr> m_name;

	public:
		ArrayLengthExpr(core::Unique<Expr> name)
			: m_name(std::move(name))
		{}

		Expr* name() const
		{
			return m_name.get();
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitArrayLengthExpr(this);
		}
	};

	class CallExpr: public Expr
	{
		core::Unique<Expr> m_base;
		Identifier m_name;
		core::Array<core::Unique<Expr>> m_arguments;

	public:
		CallExpr(core::Unique<Expr> base, Identifier name, core::Array<core::Unique<Expr>> arguments)
			: m_base(std::move(base)),
			  m_name(name),
			  m_arguments(std::move(arguments))
		{}

		Expr* base() const
		{
			return m_base.get();
		}
		const Identifier* name() const
		{
			return &m_name;
		}
		const core::Array<core::Unique<Expr>>& arguments() const
		{
			return m_arguments;
		}

		void visit(ASTVisitor* visitor) override
		{
			visitor->visitCallExpr(this);
		}
	};

	class Stmt
	{
	public:
		virtual ~Stmt() = default;
	};

	class BlockStmt: public Stmt
	{
		core::Array<core::Unique<Stmt>> m_statements;

	public:
		BlockStmt(core::Array<core::Unique<Stmt>> statements)
			: m_statements(std::move(statements))
		{}

		const core::Array<core::Unique<Stmt>>& statements() const
		{
			return m_statements;
		}
	};

	class IfStmt: public Stmt
	{
		core::Unique<Expr> m_condition;
		core::Unique<Stmt> m_trueBranch;
		core::Unique<Stmt> m_falseBranch;

	public:
		IfStmt(core::Unique<Expr> condition, core::Unique<Stmt> trueBranch, core::Unique<Stmt> falseBranch)
			: m_condition(std::move(condition)),
			  m_trueBranch(std::move(trueBranch)),
			  m_falseBranch(std::move(falseBranch))
		{}

		Expr* condition() const
		{
			return m_condition.get();
		}
		Stmt* trueBranch() const
		{
			return m_trueBranch.get();
		}
		Stmt* falseBranch() const
		{
			return m_falseBranch.get();
		}
	};

	class WhileStmt: public Stmt
	{
		core::Unique<Expr> m_condition;
		core::Unique<Stmt> m_body;

	public:
		WhileStmt(core::Unique<Expr> condition, core::Unique<Stmt> body)
			: m_condition(std::move(condition)),
			  m_body(std::move(body))
		{}

		Expr* condition() const
		{
			return m_condition.get();
		}
		Stmt* body() const
		{
			return m_body.get();
		}
	};

	class PrintStmt: public Stmt
	{
		core::Unique<Expr> m_value;

	public:
		PrintStmt(core::Unique<Expr> value)
			: m_value(std::move(value))
		{}

		Expr* value() const
		{
			return m_value.get();
		}
	};

	class AssignStmt: public Stmt
	{
		Identifier m_variableName;
		core::Unique<Expr> m_value;

	public:
		AssignStmt(Identifier variableName, core::Unique<Expr> value)
			: m_variableName(variableName),
			  m_value(std::move(value))
		{}

		const Identifier* variableName() const
		{
			return &m_variableName;
		}
		Expr* value() const
		{
			return m_value.get();
		}
	};

	class ArrayAssignStmt: public Stmt
	{
		Identifier m_arrayName;
		core::Unique<Expr> m_index;
		core::Unique<Expr> m_value;

	public:
		ArrayAssignStmt(Identifier arrayName, core::Unique<Expr> index, core::Unique<Expr> value)
			: m_arrayName(arrayName),
			  m_index(std::move(index)),
			  m_value(std::move(value))
		{}

		const Identifier* arrayName() const
		{
			return &m_arrayName;
		}
		Expr* index() const
		{
			return m_index.get();
		}
		Expr* value() const
		{
			return m_value.get();
		}
	};

	class Type
	{
	public:
		virtual ~Type() = default;
	};

	class IntArray: public Type
	{};

	class BooleanType: public Type
	{};

	class IntType: public Type
	{};

	class IdentifierType: public Type
	{
		Identifier m_name;

	public:
		IdentifierType(Identifier name)
			: m_name(name)
		{}

		const Identifier* name() const
		{
			return &m_name;
		}
	};

	class Formal
	{
		core::Unique<Type> m_type;
		Identifier m_name;

	public:
		Formal(core::Unique<Type> type, Identifier name)
			: m_type(std::move(type)),
			  m_name(name)
		{}

		Type* type() const
		{
			return m_type.get();
		}
		const Identifier* name() const
		{
			return &m_name;
		}
	};

	class VarDecl
	{
		core::Unique<Type> m_type;
		Identifier m_name;

	public:
		VarDecl(core::Unique<Type> type, Identifier name)
			: m_type(std::move(type)),
			  m_name(name)
		{}

		Type* type() const
		{
			return m_type.get();
		}
		const Identifier* name() const
		{
			return &m_name;
		}
	};

	class MethodDecl
	{
		core::Unique<Type> m_type;
		Identifier m_name;
		core::Array<Formal> m_paramsList;
		core::Array<VarDecl> m_variables;
		core::Array<core::Unique<Stmt>> m_statements;
		core::Unique<Expr> m_returnValue;

	public:
		MethodDecl(
			core::Unique<Type> type,
			Identifier name,
			core::Array<Formal> paramsList,
			core::Array<VarDecl> variables,
			core::Array<core::Unique<Stmt>> statements,
			core::Unique<Expr> returnValue)
			: m_type(std::move(type)),
			  m_name(name),
			  m_paramsList(std::move(paramsList)),
			  m_variables(std::move(variables)),
			  m_statements(std::move(statements)),
			  m_returnValue(std::move(returnValue))
		{}

		Type* type() const
		{
			return m_type.get();
		}
		const Identifier* name() const
		{
			return &m_name;
		}
		const core::Array<Formal>& paramsList() const
		{
			return m_paramsList;
		}
		const core::Array<VarDecl>& variables() const
		{
			return m_variables;
		}
		const core::Array<core::Unique<Stmt>>& statements() const
		{
			return m_statements;
		}
		Expr* returnValue() const
		{
			return m_returnValue.get();
		}
	};

	class ClassDecl
	{
	public:
		virtual ~ClassDecl() = default;
	};

	class SimpleClassDecl: public ClassDecl
	{
		Identifier m_name;
		core::Array<VarDecl> m_variables;
		core::Array<MethodDecl> m_methods;

	public:
		SimpleClassDecl(Identifier name, core::Array<VarDecl> variables, core::Array<MethodDecl> methods)
			: m_name(name),
			  m_variables(std::move(variables)),
			  m_methods(std::move(methods))
		{}

		const Identifier* name() const
		{
			return &m_name;
		}
		const core::Array<VarDecl>& variables() const
		{
			return m_variables;
		}
		const core::Array<MethodDecl>& methods() const
		{
			return m_methods;
		}
	};

	class ExtendsClassDecl: public ClassDecl
	{
		Identifier m_name;
		Identifier m_extendedClassName;
		core::Array<VarDecl> m_variables;
		core::Array<MethodDecl> m_methods;

	public:
		ExtendsClassDecl(
			Identifier name,
			Identifier extendedClassName,
			core::Array<VarDecl> variables,
			core::Array<MethodDecl> methods)
			: m_name(name),
			  m_extendedClassName(extendedClassName),
			  m_variables(std::move(variables)),
			  m_methods(std::move(methods))
		{}

		const Identifier* name() const
		{
			return &m_name;
		}
		const Identifier* extendedClassName() const
		{
			return &m_extendedClassName;
		}
		const core::Array<VarDecl>& variables() const
		{
			return m_variables;
		}
		const core::Array<MethodDecl>& methods() const
		{
			return m_methods;
		}
	};

	class MainClass
	{
		Identifier m_name;
		Identifier m_argsName;
		core::Unique<BlockStmt> m_body;

	public:
		MainClass(Identifier name, Identifier argsName, core::Unique<BlockStmt> body)
			: m_name(name),
			  m_argsName(argsName),
			  m_body(std::move(body))
		{}

		const Identifier* name() const
		{
			return &m_name;
		}
		const Identifier* argsName() const
		{
			return &m_argsName;
		}
		BlockStmt* body() const
		{
			return m_body.get();
		}
	};

	class Program
	{
		core::Unique<MainClass> m_main;
		core::Array<core::Unique<ClassDecl>> m_classes;

	public:
		Program(core::Unique<MainClass> main, core::Array<core::Unique<ClassDecl>> classes)
			: m_main(std::move(main)),
			  m_classes(std::move(classes))
		{}

		MainClass* main()
		{
			return m_main.get();
		}
		const core::Array<core::Unique<ClassDecl>>& classes() const
		{
			return m_classes;
		}
	};
}