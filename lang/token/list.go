// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package token

const (
	IDInvalid = ID(0x00)

	IDOpenParen    = ID(0x10)
	IDCloseParen   = ID(0x11)
	IDOpenBracket  = ID(0x12)
	IDCloseBracket = ID(0x13)
	IDOpenCurly    = ID(0x14)
	IDCloseCurly   = ID(0x15)

	IDDot       = ID(0x20)
	IDComma     = ID(0x21)
	IDQuestion  = ID(0x22)
	IDColon     = ID(0x23)
	IDSemicolon = ID(0x24)

	IDPlus   = ID(0x31)
	IDMinus  = ID(0x32)
	IDStar   = ID(0x33)
	IDSlash  = ID(0x34)
	IDShiftL = ID(0x35)
	IDShiftR = ID(0x36)
	IDAmp    = ID(0x37)
	IDAmpHat = ID(0x38)
	IDPipe   = ID(0x39)
	IDHat    = ID(0x3A)

	IDEq       = ID(0x40)
	IDPlusEq   = ID(0x41)
	IDMinusEq  = ID(0x42)
	IDStarEq   = ID(0x43)
	IDSlashEq  = ID(0x44)
	IDShiftLEq = ID(0x45)
	IDShiftREq = ID(0x46)
	IDAmpEq    = ID(0x47)
	IDAmpHatEq = ID(0x48)
	IDPipeEq   = ID(0x49)
	IDHatEq    = ID(0x4A)

	IDNotEq       = ID(0x50)
	IDLessThan    = ID(0x51)
	IDLessEq      = ID(0x52)
	IDEqEq        = ID(0x53)
	IDGreaterEq   = ID(0x54)
	IDGreaterThan = ID(0x55)

	// TODO: sort these keywords by name, when the list has stabilized.
	IDAnd = ID(0x80)
	IDOr  = ID(0x81)
	IDNot = ID(0x82)

	lexerBase = ID(0x100)
)

var builtInsByID = [nBuiltIns]string{
	IDOpenParen:    "(",
	IDCloseParen:   ")",
	IDOpenBracket:  "[",
	IDCloseBracket: "]",
	IDOpenCurly:    "{",
	IDCloseCurly:   "}",

	IDDot:       ".",
	IDComma:     ",",
	IDQuestion:  "?",
	IDColon:     ":",
	IDSemicolon: ";",

	IDPlus:   "+",
	IDMinus:  "-",
	IDStar:   "*",
	IDSlash:  "/",
	IDShiftL: "<<",
	IDShiftR: ">>",
	IDAmp:    "&",
	IDAmpHat: "&^",
	IDPipe:   "|",
	IDHat:    "^",

	IDEq:       "=",
	IDPlusEq:   "+=",
	IDMinusEq:  "-=",
	IDStarEq:   "*=",
	IDSlashEq:  "/=",
	IDShiftLEq: "<<=",
	IDShiftREq: ">>=",
	IDAmpEq:    "&=",
	IDAmpHatEq: "&^=",
	IDPipeEq:   "|=",
	IDHatEq:    "^=",

	IDNotEq:       "!=",
	IDLessThan:    "<",
	IDLessEq:      "<=",
	IDEqEq:        "==",
	IDGreaterEq:   ">=",
	IDGreaterThan: ">",

	IDAnd: "and",
	IDOr:  "or",
	IDNot: "not",
}

var builtInsByName = map[string]ID{}

func init() {
	for i, s := range builtInsByID {
		if s != "" {
			builtInsByName[s] = ID(i)
		}
	}
}

var builtInsImplicitSemicolons = [256]bool{
	IDCloseParen:   true,
	IDCloseBracket: true,
	IDCloseCurly:   true,

	// TODO: break, continue, fallthrough, return, a la Go.
}

// squiggles are built-in tokens that aren't alpha-numeric.
var squiggles = [256]ID{
	'(': IDOpenParen,
	')': IDCloseParen,
	'[': IDOpenBracket,
	']': IDCloseBracket,
	'{': IDOpenCurly,
	'}': IDCloseCurly,

	'.': IDDot,
	',': IDComma,
	'?': IDQuestion,
	':': IDColon,
	';': IDSemicolon,

	'&': lexerBase + '&',
	'|': lexerBase + '|',
	'^': lexerBase + '^',
	'+': lexerBase + '+',
	'-': lexerBase + '-',
	'*': lexerBase + '*',
	'/': lexerBase + '/',
	'=': lexerBase + '=',
	'<': lexerBase + '<',
	'>': lexerBase + '>',
}

type suffixLexer struct {
	suffix string
	id     ID
}

// The order of the []suffixLexer elements matters. The first match wins. Since
// we want to lex greedily, longer suffixes should be earlier in the slice.

var lexers = [256][]suffixLexer{
	'!': {
		{"=", IDNotEq},
	},
	'&': {
		{"^=", IDAmpHatEq},
		{"^", IDAmpHat},
		{"=", IDAmpEq},
		{"", IDAmp},
	},
	'|': {
		{"=", IDPipeEq},
		{"", IDPipe},
	},
	'^': {
		{"=", IDHatEq},
		{"", IDHat},
	},
	'+': {
		{"=", IDPlusEq},
		{"", IDPlus},
	},
	'-': {
		{"=", IDMinusEq},
		{"", IDMinus},
	},
	'*': {
		{"=", IDStarEq},
		{"", IDStar},
	},
	'/': {
		{"=", IDSlashEq},
		{"", IDSlash},
	},
	'=': {
		{"=", IDEqEq},
		{"", IDEq},
	},
	'<': {
		{"<", IDShiftL},
		{"=", IDLessEq},
		{"", IDLessThan},
	},
	'>': {
		{">", IDShiftR},
		{"=", IDGreaterEq},
		{"", IDGreaterThan},
	},
}
