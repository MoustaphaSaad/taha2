package minijava

import (
	"fmt"
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

	// ID
	TokenTypeId

	// Comments
	TokenTypeComment
)

func (t TokenType) String() string {
	switch t {
	case TokenTypeNone:
		return "<NONE>"
	case TokenTypeEOF:
		return "<EOF>"

	case TokenTypeKeywordBoolean:
		return "boolean"
	case TokenTypeKeywordClass:
		return "class"
	case TokenTypeKeywordElse:
		return "else"
	case TokenTypeKeywordFalse:
		return "false"
	case TokenTypeKeywordIf:
		return "if"
	case TokenTypeKeywordInt:
		return "int"
	case TokenTypeKeywordLength:
		return "length"
	case TokenTypeKeywordMain:
		return "main"
	case TokenTypeKeywordNew:
		return "new"
	case TokenTypeKeywordPublic:
		return "public"
	case TokenTypeKeywordReturn:
		return "return"
	case TokenTypeKeywordStatic:
		return "static"
	case TokenTypeKeywordThis:
		return "this"
	case TokenTypeKeywordTrue:
		return "true"
	case TokenTypeKeywordVoid:
		return "void"
	case TokenTypeKeywordWhile:
		return "while"

	case TokenTypeOperatorPlus:
		return "+"
	case TokenTypeOperatorMinus:
		return "-"
	case TokenTypeOperatorMultiply:
		return "*"
	case TokenTypeOperatorLessThan:
		return "<"
	case TokenTypeOperatorAssign:
		return "="
	case TokenTypeOperatorLogicAnd:
		return "&&"
	case TokenTypeOperatorLogicNot:
		return "!"

	case TokenTypeSemicolon:
		return ";"
	case TokenTypeDot:
		return "."
	case TokenTypeComma:
		return ","
	case TokenTypeOpenBrace:
		return "{"
	case TokenTypeCloseBrace:
		return "}"
	case TokenTypeOpenBracket:
		return "["
	case TokenTypeCloseBracket:
		return "]"
	case TokenTypeOpenParen:
		return "("
	case TokenTypeCloseParen:
		return ")"

	case TokenTypeLiteralInt:
		return "<LITERAL_INT>"

	case TokenTypeId:
		return "id"

	case TokenTypeComment:
		return "comment"

	default:
		return "<UNKNOWN>"
	}
}

type Range struct {
	Begin, End int
}

type Position struct {
	Line, Column int
}

type Location struct {
	Pos  Position
	Rng  Range
	Unit *Unit
}

type Error struct {
	Loc Location
	Msg string
}

func errf(loc Location, format string, a ...interface{}) Error {
	return Error{
		Loc: loc,
		Msg: fmt.Sprintf(format, a...),
	}
}

type Token struct {
	Type TokenType
	Text string
	Loc  Location
}

type Scanner struct {
	unit      *Unit
	it        int
	c         rune
	pos       Position
	lineBegin int
}

func NewScanner(unit *Unit) (*Scanner, error) {
	res := &Scanner{
		unit: unit,
		pos:  Position{Line: 1},
	}
	res.c, _ = utf8.DecodeRune([]byte(res.unit.Content[res.it:]))
	return res, nil
}

func (s *Scanner) Scan() Token {
	s.skipWhitespace()

	tkn := Token{
		Loc: Location{
			Pos:  s.pos,
			Rng:  Range{Begin: s.it},
			Unit: s.unit,
		},
	}
	defer func() { tkn.Loc.Rng.End = s.it }()

	if s.eof() {
		s.unit.Lines = append(s.unit.Lines, Range{s.lineBegin, len(s.unit.Content)})
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
		tkn.Type = TokenTypeLiteralInt
		tkn.Text = s.scanNumber()
	} else {
		c := s.c
		s.eat()

		switch c {
		case '+':
			tkn.Text = "+"
			tkn.Type = TokenTypeOperatorPlus
		case '-':
			tkn.Text = "-"
			tkn.Type = TokenTypeOperatorMinus
		case '*':
			tkn.Text = "*"
			tkn.Type = TokenTypeOperatorMultiply
		case '<':
			tkn.Text = "<"
			tkn.Type = TokenTypeOperatorLessThan
		case '=':
			tkn.Text = "="
			tkn.Type = TokenTypeOperatorAssign
		case '&':
			if s.c == '&' {
				s.eat()
				tkn.Text = "&&"
				tkn.Type = TokenTypeOperatorLogicAnd
			}
		case '!':
			tkn.Text = "!"
			tkn.Type = TokenTypeOperatorLogicNot
		case ';':
			tkn.Text = ";"
			tkn.Type = TokenTypeSemicolon
		case '.':
			tkn.Text = "."
			tkn.Type = TokenTypeDot
		case ',':
			tkn.Text = ","
			tkn.Type = TokenTypeComma
		case '{':
			tkn.Text = "{"
			tkn.Type = TokenTypeOpenBrace
		case '}':
			tkn.Text = "}"
			tkn.Type = TokenTypeCloseBrace
		case '[':
			tkn.Text = "["
			tkn.Type = TokenTypeOpenBracket
		case ']':
			tkn.Text = "]"
			tkn.Type = TokenTypeCloseBracket
		case '(':
			tkn.Text = "("
			tkn.Type = TokenTypeOpenParen
		case ')':
			tkn.Text = ")"
			tkn.Type = TokenTypeCloseParen
		case '/':
			if s.c == '/' {
				s.eat()
				tkn.Text = s.scanSingleLineComment()
				tkn.Type = TokenTypeComment
			} else if s.c == '*' {
				s.eat()
				tkn.Text = s.scanMultiLineComment()
				tkn.Type = TokenTypeComment
			}
		default:
			s.unit.errf(Location{Pos: tkn.Loc.Pos, Unit: s.unit}, "illegal rune '%c'", c)
		}
	}

	return tkn
}

func (s *Scanner) eof() bool {
	return s.it >= len(s.unit.Content)
}

func (s *Scanner) eat() bool {
	if s.eof() {
		s.unit.Lines = append(s.unit.Lines, Range{s.lineBegin, len(s.unit.Content)})
		return false
	}

	prev, prevIt := s.c, s.it
	_, size := utf8.DecodeRune([]byte(s.unit.Content[s.it:]))
	s.it += size
	newC, _ := utf8.DecodeRune([]byte(s.unit.Content[s.it:]))
	s.c = newC

	s.pos.Column++
	if prev == '\n' {
		s.pos.Column = 0
		s.pos.Line++
		s.unit.Lines = append(s.unit.Lines, Range{s.lineBegin, prevIt})
		s.lineBegin = s.it
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

func (s *Scanner) scanNumber() string {
	beginIt := s.it
	for unicode.IsNumber(s.c) {
		if !s.eat() {
			break
		}
	}
	return s.unit.Content[beginIt:s.it]
}

func (s *Scanner) scanSingleLineComment() string {
	beginIt := s.it
	for s.c != '\n' {
		if s.c == '\r' {
			if !s.eat() || s.c == '\n' {
				break
			}
		}

		if !s.eat() {
			break
		}
	}
	return s.unit.Content[beginIt:s.it]
}

func (s *Scanner) scanMultiLineComment() string {
	beginIt := s.it
	for {
		if s.c == '*' {
			if !s.eat() {
				break
			}

			if s.c == '/' {
				s.eat()
				break
			}
		} else if !s.eat() {
			break
		}
	}
	return s.unit.Content[beginIt:s.it]
}

func stringGetKeywordTokenType(str string) TokenType {
	switch str {
	case "boolean":
		return TokenTypeKeywordBoolean
	case "class":
		return TokenTypeKeywordClass
	case "else":
		return TokenTypeKeywordElse
	case "false":
		return TokenTypeKeywordFalse
	case "if":
		return TokenTypeKeywordIf
	case "int":
		return TokenTypeKeywordInt
	case "length":
		return TokenTypeKeywordLength
	case "main":
		return TokenTypeKeywordMain
	case "new":
		return TokenTypeKeywordNew
	case "public":
		return TokenTypeKeywordPublic
	case "return":
		return TokenTypeKeywordReturn
	case "static":
		return TokenTypeKeywordStatic
	case "this":
		return TokenTypeKeywordThis
	case "true":
		return TokenTypeKeywordTrue
	case "void":
		return TokenTypeKeywordVoid
	case "while":
		return TokenTypeKeywordWhile
	default:
		return TokenTypeNone
	}
}
