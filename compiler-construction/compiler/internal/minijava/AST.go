package minijava

type Program struct {
	Main      MainClass
	ClassList []ClassDecl
}

type MainClass struct {
	Name     Identifier
	ArgsName Identifier
	Body     BlockStmt
}

// Abstract ClassDecl
type ClassDecl interface {
	dummyClassDecl()
}

type SimpleClassDecl struct {
	Name      Identifier
	Variables []VarDecl
	Methods   []MethodDecl
}

func (*SimpleClassDecl) dummyClassDecl() {}

type ExtendsClassDecl struct {
	Name              Identifier
	ExtendedClassName Identifier
	Variables         []VarDecl
	Methods           []MethodDecl
}

func (*ExtendsClassDecl) dummyClassDecl() {}

type VarDecl struct {
	Type Type
	Name Identifier
}

type MethodDecl struct {
	Type       Type
	Name       Identifier
	ParamsList []Formal
	Variables  []VarDecl
	Statements []Stmt
	Return     Expr
}

type Formal struct {
	Type Type
	Name Identifier
}

// Abstract Type
type Type interface {
	dummyType()
}

type IntArray struct {
}

func (*IntArray) dummyType() {}

type BooleanType struct {
}

func (*BooleanType) dummyType() {}

type IntegerType struct {
}

func (*IntegerType) dummyType() {}

type IndentifierType struct {
	Name Identifier
}

func (*IndentifierType) dummyType() {}

// Abstract Statement
type Stmt interface {
	dummyStmt()
}

type BlockStmt struct {
	Statements []Stmt
}

func (*BlockStmt) dummyStmt() {}

type IfStmt struct {
	Condition   Expr
	TrueBranch  Stmt
	FalseBranch Stmt
}

func (*IfStmt) dummyStmt() {}

type WhileStmt struct {
	Condition Expr
	Body      Stmt
}

func (*WhileStmt) dummyStmt() {}

type PrintStmt struct {
	Value Expr
}

func (*PrintStmt) dummyStmt() {}

type AssignStmt struct {
	VariableName Identifier
	Value        Expr
}

func (*AssignStmt) dummyStmt() {}

type ArrayAssignStmt struct {
	ArrayName Identifier
	Index     Expr
	Value     Expr
}

func (*ArrayAssignStmt) dummyStmt() {}

// Abstract Expr
type Expr interface {
	dummyExpr()
}

type AndExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (*AndExpr) dummyExpr() {}

type LessThanExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (*LessThanExpr) dummyExpr() {}

type PlusExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (*PlusExpr) dummyExpr() {}

type MinusExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (*MinusExpr) dummyExpr() {}

type TimesExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (*TimesExpr) dummyExpr() {}

type ArrayLookupExpr struct {
	Name  Expr
	Index Expr
}

func (*ArrayLookupExpr) dummyExpr() {}

type ArrayLengthExpr struct {
	Name Expr
}

func (*ArrayLengthExpr) dummyExpr() {}

type CallExpr struct {
	Base      Expr
	Name      Identifier
	Arguments []Expr
}

func (*CallExpr) dummyExpr() {}

type IntegerLiteralExpr struct {
	Value int
}

func (*IntegerLiteralExpr) dummyExpr() {}

type TrueExpr struct {
}

func (*TrueExpr) dummyExpr() {}

type FalseExpr struct {
}

func (*FalseExpr) dummyExpr() {}

type IdentifierExpr struct {
	Name Token
}

func (*IdentifierExpr) dummyExpr() {}

type ThisExpr struct {
}

func (*ThisExpr) dummyExpr() {}

type NewArrayExpr struct {
	Length Expr
}

func (*NewArrayExpr) dummyExpr() {}

type NewObjectExpr struct {
	Name Identifier
}

func (*NewObjectExpr) dummyExpr() {}

type NotExpr struct {
	Value Expr
}

func (*NotExpr) dummyExpr() {}

type Identifier struct {
	Name Token
}