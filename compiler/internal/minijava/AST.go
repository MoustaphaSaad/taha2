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

func (_ SimpleClassDecl) dummyClassDecl() {
}

type ExtendsClassDecl struct {
	Name              Identifier
	ExtendedClassName Identifier
	Variables         []VarDecl
	Methods           []MethodDecl
}

func (_ ExtendsClassDecl) dummyClassDecl() {
}

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

func (_ IntArray) dummyType() {
}

type BooleanType struct {
}

func (_ BooleanType) dummyType() {
}

type IntegerType struct {
}

func (_ IntegerType) dummyType() {
}

type IndentifierType struct {
	Name Identifier
}

func (_ IndentifierType) dummyType() {
}

// Abstract Statement
type Stmt interface {
	dummyStmt()
}

type BlockStmt struct {
	Statements []Stmt
}

func (_ BlockStmt) dummyStmt() {
}

type IfStmt struct {
	Condition   Expr
	TrueBranch  Stmt
	FalseBranch Stmt
}

func (_ IfStmt) dummyStmt() {
}

type WhileStmt struct {
	Condition Expr
	Body      Stmt
}

func (_ WhileStmt) dummyStmt() {
}

type PrintStmt struct {
	Value Expr
}

func (_ PrintStmt) dummyStmt() {
}

type AssignStmt struct {
	VariableName Identifier
	Value        Expr
}

func (_ AssignStmt) dummyStmt() {
}

type ArrayAssignStmt struct {
	ArrayName Identifier
	Index     Expr
	Value     Expr
}

func (_ ArrayAssignStmt) dummyStmt() {
}

// Abstract Expr
type Expr interface {
	dummyExpr()
}

type AndExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (_ AndExpr) dummyExpr() {
}

type LessThanExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (_ LessThanExpr) dummyExpr() {
}

type PlusExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (_ PlusExpr) dummyExpr() {
}

type MinusExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (_ MinusExpr) dummyExpr() {
}

type TimesExpr struct {
	Operand1 Expr
	Operand2 Expr
}

func (_ TimesExpr) dummyExpr() {
}

type ArrayLookupExpr struct {
	Name  Expr
	Index Expr
}

func (_ ArrayLookupExpr) dummyExpr() {
}

type ArrayLengthExpr struct {
	Name Expr
}

func (_ ArrayLengthExpr) dummyExpr() {
}

type CallExpr struct {
	Base      Expr
	Name      Identifier
	Arguments []Expr
}

func (_ CallExpr) dummyExpr() {
}

type IntegerLiteralExpr struct {
	Value int
}

func (_ IntegerLiteralExpr) dummyExpr() {
}

type TrueExpr struct {
}

func (_ TrueExpr) dummyExpr() {
}

type FalseExpr struct {
}

func (_ FalseExpr) dummyExpr() {
}

type IdentifierExpr struct {
	Name Token
}

func (_ IdentifierExpr) dummyExpr() {
}

type ThisExpr struct {
}

func (_ ThisExpr) dummyExpr() {
}

type NewArrayExpr struct {
	Length Expr
}

func (_ NewArrayExpr) dummyExpr() {
}

type NewObjectExpr struct {
	Name Identifier
}

func (_ NewObjectExpr) dummyExpr() {
}

type NotExpr struct {
	Value Expr
}

func (_ NotExpr) dummyExpr() {
}

type Identifier struct {
	Name Token
}