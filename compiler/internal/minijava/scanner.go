package minijava

import (
	"unicode"
	"unicode/utf8"
)

type TokenType int

const (
	TokenTypeNone TokenType = iota
	TokenTypeEOF

	// Keywords
	TokenTypeKeywordBoolean
	TokenTypeKeywordClass
	TokenTypeKeywordElse
	TokenTypeKeywordFalse
	TokenTypeKeywordIf
	TokenTypeKeywordInt
	TokenTypeKeywordLength
	TokenTypeKeywordMain
	TokenTypeKeywordNew
	TokenTypeKeywordPublic
	TokenTypeKeywordReturn
	TokenTypeKeywordStatic
	TokenTypeKeywordThis
	TokenTypeKeywordTrue
	TokenTypeKeywordVoid
	TokenTypeKeywordWhile

	// Operators
	TokenTypeOperatorPlus
	TokenTypeOperatorMinus
	TokenTypeOperatorMultiply
	TokenTypeOperatorLessThan
	TokenTypeOperatorAssign
	TokenTypeOperatorLogicAnd
	TokenTypeOperatorLogicNot

	// Separators
	TokenTypeSemicolon
	TokenTypeDot
	TokenTypeComma
	TokenTypeOpenBrace
	TokenTypeCloseBrace
	TokenTypeOpenBracket
	TokenTypeCloseBracket
	TokenTypeOpenParen
	TokenTypeCloseParen

	// Literals
	TokenTypeLiteralInt
	TokenTypeLiteralBool

	// ID
	TokenTypeId

	// Comments
	TokenTypeComment
)

type Position struct {
	Line, Column int
}

type Location struct {
	Pos Position
	Unit *Unit
}

type Token struct {
	Type TokenType
	Text string
	Loc Location
}

type Scanner struct {
	unit *Unit
	it int
	c rune
	pos Position
}

func NewScanner(unit *Unit) (*Scanner, error) {
	res := &Scanner{
		unit: unit,
		pos: Position{Line: 1},
	}
	res.c, _ = utf8.DecodeRune([]byte(res.unit.Content[res.it:]))
	return res, nil
}

func (s *Scanner) Scan() Token {
	s.skipWhitespace()

	tkn := Token{
		Loc: Location{
			Pos: s.pos,
			Unit: s.unit,
		},
	}

	if s.eof() {
		tkn.Type = TokenTypeEOF
		return tkn
	}

	if unicode.IsLetter(s.c) || s.c == '_' {
		tkn.Type = TokenTypeId
		tkn.Text = s.scanId()

		keywordType := stringGetKeywordTokenType(tkn.Text)
		if keywordType != TokenTypeNone {
			tkn.Type = keywordType
		}
	} else if unicode.IsNumber(s.c) {
		// scan numbers
	} else {
		// scan others
	}

	return tkn
}

func (s *Scanner) eof() bool {
	return s.it >= len(s.unit.Content)
}

func (s *Scanner) eat() bool {
	if s.eof() {
		return false
	}

	prev, _ := s.c, s.it
	newC, size := utf8.DecodeRune([]byte(s.unit.Content[s.it:]))
	s.c = newC
	s.it += size

	s.pos.Column++
	if prev == '\n' {
		s.pos.Column = 0
		s.pos.Line++
	}
	return true
}

func (s *Scanner) skipWhitespace() {
	for unicode.IsSpace(s.c) {
		if !s.eat() {
			break
		}
	}
}

func (s *Scanner) scanId() string {
	beginIt := s.it
	for unicode.IsLetter(s.c) || unicode.IsNumber(s.c) || s.c == '_' {
		if !s.eat() {
			break
		}
	}
	return s.unit.Content[beginIt:s.it]
}

func stringGetKeywordTokenType(str string) TokenType {
	switch str {
	case "boolean": return TokenTypeKeywordBoolean
	case "class": return TokenTypeKeywordClass
	case "else": return TokenTypeKeywordElse
	case "false": return TokenTypeKeywordFalse
	case "if": return TokenTypeKeywordIf
	case "int": return TokenTypeKeywordInt
	case "length": return TokenTypeKeywordLength
	case "main": return TokenTypeKeywordMain
	case "new": return TokenTypeKeywordNew
	case "public": return TokenTypeKeywordPublic
	case "return": return TokenTypeKeywordReturn
	case "static": return TokenTypeKeywordStatic
	case "this": return TokenTypeKeywordThis
	case "true": return TokenTypeKeywordTrue
	case "void": return TokenTypeKeywordVoid
	case "while": return TokenTypeKeywordWhile
	default: return TokenTypeNone
	}
}