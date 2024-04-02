package minijava

import (
	"io"
	"os"
	"path/filepath"
)

type CompilationStage int
const (
	CompilationStageNone CompilationStage = iota
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
}

func LoadUnitFromFile(path string) (*Unit, error) {
	absolutePath, err := filepath.Abs(path)
	if err != nil { return nil, err }

	file, err := os.Open(absolutePath)
	if err != nil { return nil, err }
	defer file.Close()

	content, err := io.ReadAll(file)
	if err != nil { return nil, err }

	res := &Unit {
		Filepath: path,
		AbsolutePath: absolutePath,
		Content: string(content),
		Stage: CompilationStageNone,
	}

	return res, nil
}

func (u *Unit) Scan() error {
	scanner, err := NewScanner(u)
	if err != nil {
		return err
	}

	for {
		tkn := scanner.Scan()

		if tkn.Type != TokenTypeEOF || tkn.Type != TokenTypeNone {
			break
		}

		u.Tokens = append(u.Tokens, tkn)
	}

	u.Stage = CompilationStageScaned
	return nil
}