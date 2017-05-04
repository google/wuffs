// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package token

// Flags are the low 12 bits of an ID. For example, they signify whether a
// token is an operator, an identifier, etc.
//
// The flags are not exclusive. For example, the "+" token is both a unary and
// binary operator. It is also associative: (a + b) + c equals a + (b + c).
//
// A valid token will have non-zero flags. If none of the other flags apply,
// the FlagsOther bit will be set.
type Flags uint32

const (
	FlagsOther             = Flags(0x001)
	FlagsUnaryOp           = Flags(0x002)
	FlagsBinaryOp          = Flags(0x004)
	FlagsAssociative       = Flags(0x008)
	FlagsAssign            = Flags(0x010)
	FlagsLiteral           = Flags(0x020)
	FlagsIdent             = Flags(0x040)
	FlagsImplicitSemicolon = Flags(0x080)
	FlagsOpen              = Flags(0x100)
	FlagsClose             = Flags(0x200)
	FlagsTightLeft         = Flags(0x400)
	FlagsTightRight        = Flags(0x800)
)

// Key is the high 20 bits of an ID. It is the map key for an IDMap.
type Key uint32

const (
	idShift = 12
	idMask  = 1<<12 - 1
	maxKey  = 1<<20 - 1
)

// ID combines a Key and Flags.
type ID uint32

func (x ID) Key() Key     { return Key(x >> idShift) }
func (x ID) Flags() Flags { return Flags(x & idMask) }

func (x ID) IsUnaryOp() bool           { return Flags(x)&FlagsUnaryOp != 0 }
func (x ID) IsBinaryOp() bool          { return Flags(x)&FlagsBinaryOp != 0 }
func (x ID) IsAssociative() bool       { return Flags(x)&FlagsAssociative != 0 }
func (x ID) IsAssign() bool            { return Flags(x)&FlagsAssign != 0 }
func (x ID) IsLiteral() bool           { return Flags(x)&FlagsLiteral != 0 }
func (x ID) IsIdent() bool             { return Flags(x)&FlagsIdent != 0 }
func (x ID) IsImplicitSemicolon() bool { return Flags(x)&FlagsImplicitSemicolon != 0 }
func (x ID) IsOpen() bool              { return Flags(x)&FlagsOpen != 0 }
func (x ID) IsClose() bool             { return Flags(x)&FlagsClose != 0 }
func (x ID) IsTightLeft() bool         { return Flags(x)&FlagsTightLeft != 0 }
func (x ID) IsTightRight() bool        { return Flags(x)&FlagsTightRight != 0 }

// Token combines an ID and the line number it was seen.
type Token struct {
	ID   ID
	Line uint32
}

func (t Token) Key() Key     { return Key(t.ID >> idShift) }
func (t Token) Flags() Flags { return Flags(t.ID & idMask) }

func (t Token) IsUnaryOp() bool           { return Flags(t.ID)&FlagsUnaryOp != 0 }
func (t Token) IsBinaryOp() bool          { return Flags(t.ID)&FlagsBinaryOp != 0 }
func (t Token) IsAssociative() bool       { return Flags(t.ID)&FlagsAssociative != 0 }
func (t Token) IsAssign() bool            { return Flags(t.ID)&FlagsAssign != 0 }
func (t Token) IsLiteral() bool           { return Flags(t.ID)&FlagsLiteral != 0 }
func (t Token) IsIdent() bool             { return Flags(t.ID)&FlagsIdent != 0 }
func (t Token) IsImplicitSemicolon() bool { return Flags(t.ID)&FlagsImplicitSemicolon != 0 }
func (t Token) IsOpen() bool              { return Flags(t.ID)&FlagsOpen != 0 }
func (t Token) IsClose() bool             { return Flags(t.ID)&FlagsClose != 0 }
func (t Token) IsTightLeft() bool         { return Flags(t.ID)&FlagsTightLeft != 0 }
func (t Token) IsTightRight() bool        { return Flags(t.ID)&FlagsTightRight != 0 }

// nBuiltInKeys is the number of built-in Keys. The packing is:
//  - Zero is invalid.
//  - [0x01, 0x3F] are squiggly punctuation, such as "(", ")" and ";".
//  - [0x40, 0x5F] are squiggly assignments, such as "=" and "+=".
//  - [0x60, 0x7F] are squiggly operators, such as "+" and "==".
//  - [0x80, 0x9F] are alpha-numeric operators, such as "and" and "as".
//  - [0xA0, 0xCF] are keywords, such as "if" and "return".
//  - [0xD0, 0xDF] are literals, such as "false" and "true".
//  - [0xE0, 0xFF] are identifiers, such as "bool" and "u32".
//
// "Squiggly" means a sequence of non-alpha-numeric characters, such as "+" and
// "&=". Their Keys range in [0x01, 0x7F].
const nBuiltInKeys = 256

const (
	IDInvalid = ID(0)

	IDOpenParen    = ID(0x10<<idShift | FlagsOpen | FlagsTightRight)
	IDCloseParen   = ID(0x11<<idShift | FlagsClose | FlagsTightLeft | FlagsImplicitSemicolon)
	IDOpenBracket  = ID(0x12<<idShift | FlagsOpen | FlagsTightLeft | FlagsTightRight)
	IDCloseBracket = ID(0x13<<idShift | FlagsClose | FlagsTightLeft | FlagsImplicitSemicolon)
	IDOpenCurly    = ID(0x14<<idShift | FlagsOpen)
	IDCloseCurly   = ID(0x15<<idShift | FlagsClose | FlagsImplicitSemicolon)

	IDDot       = ID(0x20<<idShift | FlagsTightLeft | FlagsTightRight)
	IDComma     = ID(0x21<<idShift | FlagsTightLeft)
	IDQuestion  = ID(0x22<<idShift | FlagsTightRight | FlagsUnaryOp)
	IDColon     = ID(0x23<<idShift | FlagsTightLeft | FlagsTightRight)
	IDSemicolon = ID(0x24<<idShift | FlagsTightLeft)

	IDEq       = ID(0x40<<idShift | FlagsAssign)
	IDPlusEq   = ID(0x41<<idShift | FlagsAssign)
	IDMinusEq  = ID(0x42<<idShift | FlagsAssign)
	IDStarEq   = ID(0x43<<idShift | FlagsAssign)
	IDSlashEq  = ID(0x44<<idShift | FlagsAssign)
	IDShiftLEq = ID(0x45<<idShift | FlagsAssign)
	IDShiftREq = ID(0x46<<idShift | FlagsAssign)
	IDAmpEq    = ID(0x47<<idShift | FlagsAssign)
	IDAmpHatEq = ID(0x48<<idShift | FlagsAssign)
	IDPipeEq   = ID(0x49<<idShift | FlagsAssign)
	IDHatEq    = ID(0x4A<<idShift | FlagsAssign)

	IDPlus   = ID(0x61<<idShift | FlagsBinaryOp | FlagsUnaryOp | FlagsAssociative)
	IDMinus  = ID(0x62<<idShift | FlagsBinaryOp | FlagsUnaryOp)
	IDStar   = ID(0x63<<idShift | FlagsBinaryOp | FlagsAssociative)
	IDSlash  = ID(0x64<<idShift | FlagsBinaryOp)
	IDShiftL = ID(0x65<<idShift | FlagsBinaryOp)
	IDShiftR = ID(0x66<<idShift | FlagsBinaryOp)
	IDAmp    = ID(0x67<<idShift | FlagsBinaryOp | FlagsAssociative)
	IDAmpHat = ID(0x68<<idShift | FlagsBinaryOp)
	IDPipe   = ID(0x69<<idShift | FlagsBinaryOp | FlagsAssociative)
	IDHat    = ID(0x6A<<idShift | FlagsBinaryOp | FlagsAssociative)

	IDNotEq       = ID(0x70<<idShift | FlagsBinaryOp)
	IDLessThan    = ID(0x71<<idShift | FlagsBinaryOp)
	IDLessEq      = ID(0x72<<idShift | FlagsBinaryOp)
	IDEqEq        = ID(0x73<<idShift | FlagsBinaryOp)
	IDGreaterEq   = ID(0x74<<idShift | FlagsBinaryOp)
	IDGreaterThan = ID(0x75<<idShift | FlagsBinaryOp)

	// TODO: sort these by name, when the list has stabilized.
	IDAnd = ID(0x80<<idShift | FlagsBinaryOp)
	IDOr  = ID(0x81<<idShift | FlagsBinaryOp)
	IDNot = ID(0x82<<idShift | FlagsUnaryOp)
	IDAs  = ID(0x83<<idShift | FlagsBinaryOp)

	// TODO: sort these by name, when the list has stabilized.
	IDFunc     = ID(0xA0<<idShift | FlagsOther)
	IDPtr      = ID(0xA1<<idShift | FlagsOther)
	IDAssert   = ID(0xA2<<idShift | FlagsOther)
	IDFor      = ID(0xA3<<idShift | FlagsOther)
	IDIf       = ID(0xA4<<idShift | FlagsOther)
	IDElse     = ID(0xA5<<idShift | FlagsOther)
	IDReturn   = ID(0xA6<<idShift | FlagsOther | FlagsImplicitSemicolon)
	IDBreak    = ID(0xA7<<idShift | FlagsOther | FlagsImplicitSemicolon)
	IDContinue = ID(0xA8<<idShift | FlagsOther | FlagsImplicitSemicolon)

	IDFalse = ID(0xD0<<idShift | FlagsLiteral | FlagsImplicitSemicolon)
	IDTrue  = ID(0xD1<<idShift | FlagsLiteral | FlagsImplicitSemicolon)

	IDI8    = ID(0xE0<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDI16   = ID(0xE1<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDI32   = ID(0xE2<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDI64   = ID(0xE3<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU8    = ID(0xE4<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU16   = ID(0xE5<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU32   = ID(0xE6<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU64   = ID(0xE7<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDUsize = ID(0xE8<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBool  = ID(0xE9<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBuf1  = ID(0xEA<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBuf2  = ID(0xEB<<idShift | FlagsIdent | FlagsImplicitSemicolon)

	IDUnderscore = ID(0xF0<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDThis       = ID(0xF1<<idShift | FlagsIdent | FlagsImplicitSemicolon)
)

var builtInsByKey = [nBuiltInKeys]struct {
	name string
	id   ID
}{
	IDOpenParen >> idShift:    {"(", IDOpenParen},
	IDCloseParen >> idShift:   {")", IDCloseParen},
	IDOpenBracket >> idShift:  {"[", IDOpenBracket},
	IDCloseBracket >> idShift: {"]", IDCloseBracket},
	IDOpenCurly >> idShift:    {"{", IDOpenCurly},
	IDCloseCurly >> idShift:   {"}", IDCloseCurly},

	IDDot >> idShift:       {".", IDDot},
	IDComma >> idShift:     {",", IDComma},
	IDQuestion >> idShift:  {"?", IDQuestion},
	IDColon >> idShift:     {":", IDColon},
	IDSemicolon >> idShift: {";", IDSemicolon},

	IDEq >> idShift:       {"=", IDEq},
	IDPlusEq >> idShift:   {"+=", IDPlusEq},
	IDMinusEq >> idShift:  {"-=", IDMinusEq},
	IDStarEq >> idShift:   {"*=", IDStarEq},
	IDSlashEq >> idShift:  {"/=", IDSlashEq},
	IDShiftLEq >> idShift: {"<<=", IDShiftLEq},
	IDShiftREq >> idShift: {">>=", IDShiftREq},
	IDAmpEq >> idShift:    {"&=", IDAmpEq},
	IDAmpHatEq >> idShift: {"&^=", IDAmpHatEq},
	IDPipeEq >> idShift:   {"|=", IDPipeEq},
	IDHatEq >> idShift:    {"^=", IDHatEq},

	IDPlus >> idShift:   {"+", IDPlus},
	IDMinus >> idShift:  {"-", IDMinus},
	IDStar >> idShift:   {"*", IDStar},
	IDSlash >> idShift:  {"/", IDSlash},
	IDShiftL >> idShift: {"<<", IDShiftL},
	IDShiftR >> idShift: {">>", IDShiftR},
	IDAmp >> idShift:    {"&", IDAmp},
	IDAmpHat >> idShift: {"&^", IDAmpHat},
	IDPipe >> idShift:   {"|", IDPipe},
	IDHat >> idShift:    {"^", IDHat},

	IDNotEq >> idShift:       {"!=", IDNotEq},
	IDLessThan >> idShift:    {"<", IDLessThan},
	IDLessEq >> idShift:      {"<=", IDLessEq},
	IDEqEq >> idShift:        {"==", IDEqEq},
	IDGreaterEq >> idShift:   {">=", IDGreaterEq},
	IDGreaterThan >> idShift: {">", IDGreaterThan},

	IDAnd >> idShift: {"and", IDAnd},
	IDOr >> idShift:  {"or", IDOr},
	IDNot >> idShift: {"not", IDNot},
	IDAs >> idShift:  {"as", IDAs},

	IDFunc >> idShift:     {"func", IDFunc},
	IDPtr >> idShift:      {"ptr", IDPtr},
	IDAssert >> idShift:   {"assert", IDAssert},
	IDFor >> idShift:      {"for", IDFor},
	IDIf >> idShift:       {"if", IDIf},
	IDElse >> idShift:     {"else", IDElse},
	IDReturn >> idShift:   {"return", IDReturn},
	IDBreak >> idShift:    {"break", IDBreak},
	IDContinue >> idShift: {"continue", IDContinue},

	IDFalse >> idShift: {"false", IDFalse},
	IDTrue >> idShift:  {"true", IDTrue},

	IDI8 >> idShift:    {"i8", IDI8},
	IDI16 >> idShift:   {"i16", IDI16},
	IDI32 >> idShift:   {"i32", IDI32},
	IDI64 >> idShift:   {"i64", IDI64},
	IDU8 >> idShift:    {"u8", IDU8},
	IDU16 >> idShift:   {"u16", IDU16},
	IDU32 >> idShift:   {"u32", IDU32},
	IDU64 >> idShift:   {"u64", IDU64},
	IDUsize >> idShift: {"usize", IDUsize},
	IDBool >> idShift:  {"bool", IDBool},
	IDBuf1 >> idShift:  {"buf1", IDBuf1},
	IDBuf2 >> idShift:  {"buf2", IDBuf2},

	IDUnderscore >> idShift: {"_", IDUnderscore},
	IDThis >> idShift:       {"this", IDThis},
}

var builtInsByName = map[string]ID{}

func init() {
	for _, x := range builtInsByKey {
		if x.name != "" {
			builtInsByName[x.name] = x.id
		}
	}
}

// squiggles are built-in IDs that aren't alpha-numeric.
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

	// 1<<idShift is a non-zero ID with zero Flags. It is an invalid ID,
	// signifying that lookup should continue in the lexers array below.

	'&': 1 << idShift,
	'|': 1 << idShift,
	'^': 1 << idShift,
	'+': 1 << idShift,
	'-': 1 << idShift,
	'*': 1 << idShift,
	'/': 1 << idShift,
	'=': 1 << idShift,
	'<': 1 << idShift,
	'>': 1 << idShift,
}

type suffixLexer struct {
	suffix string
	id     ID
}

// lexers lex ambiguous 1-byte squiggles. For example, "&" might be the start
// of "&^" or "&=".
//
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
