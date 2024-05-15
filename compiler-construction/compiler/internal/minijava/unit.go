package minijava

import (
	"bytes"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"unicode/utf8"
)

type CompilationStage int

const (
	CompilationStageNone CompilationStage = iota
	CompilationStageFailed
	CompilationStageScaned
)

type Unit struct {
	// file path as supplied by the user
	Filepath string
	// absolute path to the file
	AbsolutePath string
	// content of the file
	Content string
	// stores the compilation stage that this unit has reached
	Stage CompilationStage
	// tokens of said file
	Tokens []Token
	// start and end of each line in the source code
	Lines []Range
	// list of errors that's reported while compiling this unit
	Errors []Error
}

func LoadUnitFromFile(path string) (*Unit, error) {
	absolutePath, err := filepath.Abs(path)
	if err != nil {
		return nil, err
	}

	file, err := os.Open(absolutePath)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	content, err := io.ReadAll(file)
	if err != nil {
		return nil, err
	}

	res := &Unit{
		Filepath:     path,
		AbsolutePath: absolutePath,
		Content:      string(content),
		Stage:        CompilationStageNone,
	}

	return res, nil
}

func (u *Unit) Scan() bool {
	if u.Stage != CompilationStageNone {
		return u.Stage != CompilationStageFailed
	}

	scanner, err := NewScanner(u)
	if err != nil {
		return false
	}

	for {
		tkn := scanner.Scan()

		if tkn.Type == TokenTypeEOF || tkn.Type == TokenTypeNone {
			break
		}

		u.Tokens = append(u.Tokens, tkn)
	}

	if u.HasErrors() {
		u.Stage = CompilationStageFailed
		return false
	} else {
		u.Stage = CompilationStageScaned
		return true
	}
}

func (u *Unit) HasErrors() bool {
	return len(u.Errors) > 0
}

func (u *Unit) DumpTokens() string {
	var buffer bytes.Buffer
	for _, tkn := range u.Tokens {
		fmt.Fprintf(&buffer, "Token: \"%s\", Type: \"%s\", Line: %v:%v\n", tkn.Text, tkn.Type.String(), tkn.Loc.Pos.Line, tkn.Loc.Pos.Column)
	}
	return buffer.String()
}

func (u *Unit) DumpErrors() string {
	var buffer bytes.Buffer
	for i, err := range u.Errors {
		if i > 0 {
			fmt.Fprintf(&buffer, "\n")
		}

		if err.Loc.Pos.Line > 0 {
			if err.Loc.Rng.End-err.Loc.Rng.Begin > 0 {
				l := u.Lines[err.Loc.Pos.Line-1]
				fmt.Fprintf(&buffer, ">> %s\n", u.Content[l.Begin:l.End])
				fmt.Fprintf(&buffer, ">> ")
				for i := l.Begin; i < l.End; {
					c, size := utf8.DecodeRune([]byte(u.Content[i:]))
					i += size

					if c == '\r' || c == '\n' {
						// do nothing
					} else if i >= err.Loc.Rng.Begin && i < err.Loc.Rng.End {
						fmt.Fprintf(&buffer, "^")
					} else if c == '\t' {
						fmt.Fprintf(&buffer, "\t")
					} else {
						fmt.Fprintf(&buffer, " ")
					}
				}
				fmt.Fprintf(&buffer, "\nError[%s:%v:%v]: %s", err.Loc.Unit.Filepath, err.Loc.Pos.Line, err.Loc.Pos.Column, err.Msg)
			} else {
				fmt.Fprintf(&buffer, "Error[%s:%v:%v]: %s", err.Loc.Unit.Filepath, err.Loc.Pos.Line, err.Loc.Pos.Column, err.Msg)
			}
		} else {
			if err.Loc.Unit != nil {
				fmt.Fprintf(&buffer, "Error[%s]: %s", err.Loc.Unit.Filepath, err.Msg)
			} else {
				fmt.Fprintf(&buffer, "Error: %s", err.Msg)
			}
		}
	}
	return buffer.String()
}

func (u *Unit) errf(loc Location, format string, a ...interface{}) {
	e := errf(loc, format, a...)
	u.Errors = append(u.Errors, e)
}
