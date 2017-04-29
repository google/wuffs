// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package token

// nBuiltIns is the number of built-in IDs. The packing is:
//	- Zero is invalid.
//	- [0x001, 0x03F] are squiggly punctuation, such as "(", ")" and ";".
//	- [0x040, 0x05F] are squiggly assignments, such as "=" and "+=".
//	- [0x060, 0x07F] are squiggly operators, such as "+" and "==".
//	- [0x080, 0x09F] are alpha-numeric operators, such as "and" and "as".
//	- [0x0A0, 0x0CF] are keywords, such as "if" and "return".
//	- [0x0D0, 0x0DF] are literals, such as "false" and "true".
//	- [0x0E0, 0x0FF] are identifiers, such as "bool" and "u32".
//	- [0x100, 0x1FF] aren't returned by Tokenize. The space encodes ambiguous
//	  1-byte squiggles. For example, & might be the start of &^ or &=.
//
// "Squiggly" means a sequence of non-alpha-numeric characters, such as "+" and
// "&=". Their IDs range in [0x001, 0x07F].
const nBuiltIns = 512

type ID uint32

func (x ID) IsBuiltIn() bool { return x < nBuiltIns }

func (x ID) implicitSemicolon() bool { return x >= nBuiltIns || builtInsImplicitSemicolons[x&0xFF] }

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

	IDPlus   = ID(0x61)
	IDMinus  = ID(0x62)
	IDStar   = ID(0x63)
	IDSlash  = ID(0x64)
	IDShiftL = ID(0x65)
	IDShiftR = ID(0x66)
	IDAmp    = ID(0x67)
	IDAmpHat = ID(0x68)
	IDPipe   = ID(0x69)
	IDHat    = ID(0x6A)

	IDNotEq       = ID(0x70)
	IDLessThan    = ID(0x71)
	IDLessEq      = ID(0x72)
	IDEqEq        = ID(0x73)
	IDGreaterEq   = ID(0x74)
	IDGreaterThan = ID(0x75)

	// TODO: sort these by name, when the list has stabilized.
	IDAnd = ID(0x80)
	IDOr  = ID(0x81)
	IDNot = ID(0x82)
	IDAs  = ID(0x83)

	// TODO: sort these by name, when the list has stabilized.
	IDFunc = ID(0xA0)

	IDFalse = ID(0xD0)
	IDTrue  = ID(0xD1)

	IDI8    = ID(0xE0)
	IDI16   = ID(0xE1)
	IDI32   = ID(0xE2)
	IDI64   = ID(0xE3)
	IDU8    = ID(0xE4)
	IDU16   = ID(0xE5)
	IDU32   = ID(0xE6)
	IDU64   = ID(0xE7)
	IDUsize = ID(0xE8)
	IDBool  = ID(0xE9)
	IDBuf1  = ID(0xEA)
	IDBuf2  = ID(0xEB)

	IDUnderscore = ID(0xFF)

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

	IDNotEq:       "!=",
	IDLessThan:    "<",
	IDLessEq:      "<=",
	IDEqEq:        "==",
	IDGreaterEq:   ">=",
	IDGreaterThan: ">",

	IDAnd: "and",
	IDOr:  "or",
	IDNot: "not",
	IDAs:  "as",

	IDFunc: "func",

	IDFalse: "false",
	IDTrue:  "true",

	IDI8:    "i8",
	IDI16:   "i16",
	IDI32:   "i32",
	IDI64:   "i64",
	IDU8:    "u8",
	IDU16:   "u16",
	IDU32:   "u32",
	IDU64:   "u64",
	IDUsize: "usize",
	IDBool:  "bool",
	IDBuf1:  "buf1",
	IDBuf2:  "buf2",

	IDUnderscore: "_",
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
		{"<=", IDShiftLEq},
		{"<", IDShiftL},
		{"=", IDLessEq},
		{"", IDLessThan},
	},
	'>': {
		{">=", IDShiftREq},
		{">", IDShiftR},
		{"=", IDGreaterEq},
		{"", IDGreaterThan},
	},
}
