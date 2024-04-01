package main

import (
	"bufio"
	"io"
)

type TokenType int

const (
	TokenTypeNone TokenType = iota
	TokenTypeKeywordClass
	TokenTypeKeywordPublic
	TokenTypeKeywordStatic
	TokenTypeKeywordVoid
	TokenTypeKeywordString
	TokenTypeKeywordExtends
	TokenTypeKeywordReturn
	TokenTypeKeywordInt
	TokenTypeKeywordBoolean
	TokenTypeKeywordIf
	TokenTypeKeywordElse
	TokenTypeKeywordWhile
	TokenTypeKeywordTrue
	TokenTypeKeywordFalse
	TokenTypeKeywordThis
	TokenTypeKeywordNew
	TokenTypeOpenBrace
	TokenTypeCloseBrace
	TokenTypeOpenBracket
	TokenTypeCloseBracket
	TokenTypeOpenParen
	TokenTypeCloseParen
	TokenTypeSemicolon
	TokenTypeDot
	TokenTypeNot
	TokenTypeEqual
	TokenTypeInt
	TokenTypePlus
	TokenTypeMinus
	TokenTypeMul
	TokenTypeLessThan
	TokenTypeID
)

type Position struct {
	Line, Column int
}

type Scanner struct {
	reader io.Reader
	bufReader *bufio.Reader
	c rune
	pos Position
}

func NewScanner(reader io.Reader) (*Scanner, error) {
	res := &Scanner{
		reader: reader,
		bufReader: bufio.NewReader(reader),
		pos: Position{Line: 1},
	}
	c, _, err := res.bufReader.ReadRune()
	if err != nil {
		return nil, err
	}
	res.c = c
	return res, nil
}
