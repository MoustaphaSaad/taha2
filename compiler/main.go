package main

import "fmt"

type Table struct {
	Values map[string]int
}

func NewTable() Table {
	return Table {
		Values: make(map[string]int),
	}
}

func (t Table) getValueByName(name string) int {
	return t.Values[name]
}

func (t *Table) setValue(name string, value int) {
	t.Values[name] = value
}

type Exp interface {
	Eval(table Table) int
}

type IDExp struct {
	ID string
}

func (e IDExp) Eval(table Table) int {
	return table.getValueByName(e.ID)
}

type NumExp struct {
	Num int
}

func (e NumExp) Eval(Table) int {
	return e.Num
}

type BinaryOp int

const (
	BinaryOpPlus BinaryOp = iota
	BinaryOpMinus
	BinaryOpTimes
	BinaryOpDiv
)

type OpExp struct {
	Left  Exp
	Op    BinaryOp
	Right Exp
}

func (e OpExp) Eval(table Table) int {
	leftVal := e.Left.Eval(table)
	rightVal := e.Right.Eval(table)

	switch e.Op {
	case BinaryOpPlus:
		return leftVal + rightVal
	case BinaryOpMinus:
		return leftVal - rightVal
	case BinaryOpTimes:
		return leftVal * rightVal
	case BinaryOpDiv:
		return leftVal / rightVal
	default:
		panic("invalid op")
	}
}

type ESeqExp struct {
	S Stm
	E Exp
}

func (e ESeqExp) Eval(table Table) int {
	e.S.Eval(&table)
	return e.E.Eval(table)
}

type Stm interface {
	Eval(*Table)
}

type CompoundStm struct {
	Stm1, Stm2 Stm
}

func (s CompoundStm) Eval(table *Table) {
	s.Stm1.Eval(table)
	s.Stm2.Eval(table)
}

type AssignStm struct {
	ID    string
	Value Exp
}

func (s AssignStm) Eval(table *Table) {
	val := s.Value.Eval(*table)
	table.setValue(s.ID, val)
}

type PrintStm struct {
	Values []Exp
}

func (s PrintStm) Eval(table *Table) {
	for i := range s.Values {
		if i > 0 {
			fmt.Print(" ")
		}
		val := s.Values[i].Eval(*table)
		fmt.Print(val)
	}
	fmt.Print("\n")
}

func main() {
	prog := CompoundStm{
		AssignStm{"a", OpExp{NumExp{5}, BinaryOpPlus, NumExp{3}}},
		CompoundStm{
			AssignStm{"b",
				ESeqExp{
					PrintStm{[]Exp{IDExp{"a"}, OpExp{IDExp{"a"}, BinaryOpMinus, NumExp{1}}}},
					OpExp{NumExp{10}, BinaryOpTimes, IDExp{"a"}},
				},
			},
			PrintStm{[]Exp{IDExp{"b"}}},
		},
	}

	table := NewTable()
	prog.Eval(&table)
}
