// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package token

// Flags are the low 14 bits of an ID. For example, they signify whether a
// token is an operator, an identifier, etc.
//
// The flags are not exclusive. For example, the "+" token is both a unary and
// binary operator. It is also associative: (a + b) + c equals a + (b + c).
//
// A valid token will have non-zero flags. If none of the other flags apply,
// the FlagsOther bit will be set.
type Flags uint32

const (
	FlagsOther             = Flags(0x0001)
	FlagsUnaryOp           = Flags(0x0002)
	FlagsBinaryOp          = Flags(0x0004)
	FlagsAssociativeOp     = Flags(0x0008)
	FlagsLiteral           = Flags(0x0010)
	FlagsNumLiteral        = Flags(0x0020)
	FlagsStrLiteral        = Flags(0x0040)
	FlagsIdent             = Flags(0x0080)
	FlagsOpen              = Flags(0x0100)
	FlagsClose             = Flags(0x0200)
	FlagsTightLeft         = Flags(0x0400)
	FlagsTightRight        = Flags(0x0800)
	FlagsAssign            = Flags(0x1000)
	FlagsImplicitSemicolon = Flags(0x2000)
)

// Key is the high 18 bits of an ID. It is the map key for an IDMap.
type Key uint32

const (
	idShift = 14
	idMask  = 1<<14 - 1
	maxKey  = 1<<18 - 1
)

// ID combines a Key and Flags.
type ID uint32

func (x ID) UnaryForm() ID       { return unaryForms[0xFF&(x>>idShift)] }
func (x ID) BinaryForm() ID      { return binaryForms[0xFF&(x>>idShift)] }
func (x ID) AssociativeForm() ID { return associativeForms[0xFF&(x>>idShift)] }

func (x ID) Key() Key     { return Key(x >> idShift) }
func (x ID) Flags() Flags { return Flags(x & idMask) }

func (x ID) IsUnaryOp() bool           { return Flags(x)&FlagsUnaryOp != 0 }
func (x ID) IsBinaryOp() bool          { return Flags(x)&FlagsBinaryOp != 0 }
func (x ID) IsAssociativeOp() bool     { return Flags(x)&FlagsAssociativeOp != 0 }
func (x ID) IsLiteral() bool           { return Flags(x)&FlagsLiteral != 0 }
func (x ID) IsNumLiteral() bool        { return Flags(x)&FlagsNumLiteral != 0 }
func (x ID) IsStrLiteral() bool        { return Flags(x)&FlagsStrLiteral != 0 }
func (x ID) IsIdent() bool             { return Flags(x)&FlagsIdent != 0 }
func (x ID) IsOpen() bool              { return Flags(x)&FlagsOpen != 0 }
func (x ID) IsClose() bool             { return Flags(x)&FlagsClose != 0 }
func (x ID) IsTightLeft() bool         { return Flags(x)&FlagsTightLeft != 0 }
func (x ID) IsTightRight() bool        { return Flags(x)&FlagsTightRight != 0 }
func (x ID) IsAssign() bool            { return Flags(x)&FlagsAssign != 0 }
func (x ID) IsImplicitSemicolon() bool { return Flags(x)&FlagsImplicitSemicolon != 0 }

// Token combines an ID and the line number it was seen.
type Token struct {
	ID   ID
	Line uint32
}

func (t Token) Key() Key     { return Key(t.ID >> idShift) }
func (t Token) Flags() Flags { return Flags(t.ID & idMask) }

func (t Token) IsUnaryOp() bool           { return Flags(t.ID)&FlagsUnaryOp != 0 }
func (t Token) IsBinaryOp() bool          { return Flags(t.ID)&FlagsBinaryOp != 0 }
func (t Token) IsAssociativeOp() bool     { return Flags(t.ID)&FlagsAssociativeOp != 0 }
func (t Token) IsLiteral() bool           { return Flags(t.ID)&FlagsLiteral != 0 }
func (t Token) IsNumLiteral() bool        { return Flags(t.ID)&FlagsNumLiteral != 0 }
func (t Token) IsStrLiteral() bool        { return Flags(t.ID)&FlagsStrLiteral != 0 }
func (t Token) IsIdent() bool             { return Flags(t.ID)&FlagsIdent != 0 }
func (t Token) IsOpen() bool              { return Flags(t.ID)&FlagsOpen != 0 }
func (t Token) IsClose() bool             { return Flags(t.ID)&FlagsClose != 0 }
func (t Token) IsTightLeft() bool         { return Flags(t.ID)&FlagsTightLeft != 0 }
func (t Token) IsTightRight() bool        { return Flags(t.ID)&FlagsTightRight != 0 }
func (t Token) IsAssign() bool            { return Flags(t.ID)&FlagsAssign != 0 }
func (t Token) IsImplicitSemicolon() bool { return Flags(t.ID)&FlagsImplicitSemicolon != 0 }

// nBuiltInKeys is the number of built-in Keys. The packing is:
//  - Zero is invalid.
//  - [0x01, 0x2F] are squiggly punctuation, such as "(", ")" and ";".
//  - [0x30, 0x3F] are squiggly assignments, such as "=" and "+=".
//  - [0x40, 0x5F] are operators, such as "+", "==" and "not".
//  - [0x60, 0x8F] are keywords, such as "if" and "return".
//  - [0x90, 0x9F] are literals, such as "false" and "true".
//  - [0xA0, 0xBF] are identifiers, such as "bool" and "u32".
//  - [0xC0, 0xFF] are disambiguation forms, e.g. unary "+" vs binary "+".
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
	IDQuestion  = ID(0x22<<idShift | FlagsTightLeft | FlagsTightRight)
	IDColon     = ID(0x23<<idShift | FlagsTightLeft | FlagsTightRight)
	IDSemicolon = ID(0x24<<idShift | FlagsTightLeft)

	IDEq       = ID(0x30<<idShift | FlagsAssign)
	IDPlusEq   = ID(0x31<<idShift | FlagsAssign)
	IDMinusEq  = ID(0x32<<idShift | FlagsAssign)
	IDStarEq   = ID(0x33<<idShift | FlagsAssign)
	IDSlashEq  = ID(0x34<<idShift | FlagsAssign)
	IDShiftLEq = ID(0x35<<idShift | FlagsAssign)
	IDShiftREq = ID(0x36<<idShift | FlagsAssign)
	IDAmpEq    = ID(0x37<<idShift | FlagsAssign)
	IDAmpHatEq = ID(0x38<<idShift | FlagsAssign)
	IDPipeEq   = ID(0x39<<idShift | FlagsAssign)
	IDHatEq    = ID(0x3A<<idShift | FlagsAssign)

	IDPlus   = ID(0x41<<idShift | FlagsBinaryOp | FlagsUnaryOp | FlagsAssociativeOp)
	IDMinus  = ID(0x42<<idShift | FlagsBinaryOp | FlagsUnaryOp)
	IDStar   = ID(0x43<<idShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDSlash  = ID(0x44<<idShift | FlagsBinaryOp)
	IDShiftL = ID(0x45<<idShift | FlagsBinaryOp)
	IDShiftR = ID(0x46<<idShift | FlagsBinaryOp)
	IDAmp    = ID(0x47<<idShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDAmpHat = ID(0x48<<idShift | FlagsBinaryOp)
	IDPipe   = ID(0x49<<idShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDHat    = ID(0x4A<<idShift | FlagsBinaryOp | FlagsAssociativeOp)

	IDNotEq       = ID(0x50<<idShift | FlagsBinaryOp)
	IDLessThan    = ID(0x51<<idShift | FlagsBinaryOp)
	IDLessEq      = ID(0x52<<idShift | FlagsBinaryOp)
	IDEqEq        = ID(0x53<<idShift | FlagsBinaryOp)
	IDGreaterEq   = ID(0x54<<idShift | FlagsBinaryOp)
	IDGreaterThan = ID(0x55<<idShift | FlagsBinaryOp)

	// TODO: sort these by name, when the list has stabilized.
	IDAnd = ID(0x58<<idShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDOr  = ID(0x59<<idShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDNot = ID(0x5A<<idShift | FlagsUnaryOp)
	IDAs  = ID(0x5B<<idShift | FlagsBinaryOp)

	// TODO: sort these by name, when the list has stabilized.
	IDFunc     = ID(0x60<<idShift | FlagsOther)
	IDPtr      = ID(0x61<<idShift | FlagsOther)
	IDAssert   = ID(0x62<<idShift | FlagsOther)
	IDFor      = ID(0x63<<idShift | FlagsOther)
	IDIf       = ID(0x64<<idShift | FlagsOther)
	IDElse     = ID(0x65<<idShift | FlagsOther)
	IDReturn   = ID(0x66<<idShift | FlagsOther | FlagsImplicitSemicolon)
	IDBreak    = ID(0x67<<idShift | FlagsOther | FlagsImplicitSemicolon)
	IDContinue = ID(0x68<<idShift | FlagsOther | FlagsImplicitSemicolon)
	IDStruct   = ID(0x69<<idShift | FlagsOther)
	IDUse      = ID(0x6A<<idShift | FlagsOther)

	IDFalse = ID(0x90<<idShift | FlagsLiteral | FlagsImplicitSemicolon)
	IDTrue  = ID(0x91<<idShift | FlagsLiteral | FlagsImplicitSemicolon)

	IDI8    = ID(0xA0<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDI16   = ID(0xA1<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDI32   = ID(0xA2<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDI64   = ID(0xA3<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU8    = ID(0xA4<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU16   = ID(0xA5<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU32   = ID(0xA6<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDU64   = ID(0xA7<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDUsize = ID(0xA8<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBool  = ID(0xA9<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBuf1  = ID(0xAA<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBuf2  = ID(0xAB<<idShift | FlagsIdent | FlagsImplicitSemicolon)

	IDUnderscore = ID(0xB0<<idShift | FlagsIdent | FlagsImplicitSemicolon)
	IDThis       = ID(0xB1<<idShift | FlagsIdent | FlagsImplicitSemicolon)
)

// The IDXFoo IDs are not returned by the tokenizer. They are used by the
// ast.Node ID-typed fields to disambiguate e.g. unary vs binary plus.
const (
	minXKey = 0xC0
	maxXKey = 0xFF

	IDXUnaryPlus  = ID(0xC0<<idShift | FlagsUnaryOp)
	IDXUnaryMinus = ID(0xC1<<idShift | FlagsUnaryOp)
	IDXUnaryNot   = ID(0xC2<<idShift | FlagsUnaryOp)

	IDXBinaryPlus        = ID(0xD0<<idShift | FlagsBinaryOp)
	IDXBinaryMinus       = ID(0xD1<<idShift | FlagsBinaryOp)
	IDXBinaryStar        = ID(0xD2<<idShift | FlagsBinaryOp)
	IDXBinarySlash       = ID(0xD3<<idShift | FlagsBinaryOp)
	IDXBinaryShiftL      = ID(0xD4<<idShift | FlagsBinaryOp)
	IDXBinaryShiftR      = ID(0xD5<<idShift | FlagsBinaryOp)
	IDXBinaryAmp         = ID(0xD6<<idShift | FlagsBinaryOp)
	IDXBinaryAmpHat      = ID(0xD7<<idShift | FlagsBinaryOp)
	IDXBinaryPipe        = ID(0xD8<<idShift | FlagsBinaryOp)
	IDXBinaryHat         = ID(0xD9<<idShift | FlagsBinaryOp)
	IDXBinaryNotEq       = ID(0xDA<<idShift | FlagsBinaryOp)
	IDXBinaryLessThan    = ID(0xDB<<idShift | FlagsBinaryOp)
	IDXBinaryLessEq      = ID(0xDC<<idShift | FlagsBinaryOp)
	IDXBinaryEqEq        = ID(0xDD<<idShift | FlagsBinaryOp)
	IDXBinaryGreaterEq   = ID(0xDE<<idShift | FlagsBinaryOp)
	IDXBinaryGreaterThan = ID(0xDF<<idShift | FlagsBinaryOp)
	IDXBinaryAnd         = ID(0xE0<<idShift | FlagsBinaryOp)
	IDXBinaryOr          = ID(0xE1<<idShift | FlagsBinaryOp)
	IDXBinaryAs          = ID(0xE2<<idShift | FlagsBinaryOp)

	IDXAssociativePlus = ID(0xF0<<idShift | FlagsAssociativeOp)
	IDXAssociativeStar = ID(0xF1<<idShift | FlagsAssociativeOp)
	IDXAssociativeAmp  = ID(0xF2<<idShift | FlagsAssociativeOp)
	IDXAssociativePipe = ID(0xF3<<idShift | FlagsAssociativeOp)
	IDXAssociativeHat  = ID(0xF4<<idShift | FlagsAssociativeOp)
	IDXAssociativeAnd  = ID(0xF5<<idShift | FlagsAssociativeOp)
	IDXAssociativeOr   = ID(0xF6<<idShift | FlagsAssociativeOp)
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
	IDStruct >> idShift:   {"struct", IDStruct},
	IDUse >> idShift:      {"use", IDUse},

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

var unaryForms = [256]ID{
	IDPlus >> idShift:  IDXUnaryPlus,
	IDMinus >> idShift: IDXUnaryMinus,
	IDNot >> idShift:   IDXUnaryNot,
}

var binaryForms = [256]ID{
	IDPlus >> idShift:        IDXBinaryPlus,
	IDMinus >> idShift:       IDXBinaryMinus,
	IDStar >> idShift:        IDXBinaryStar,
	IDSlash >> idShift:       IDXBinarySlash,
	IDShiftL >> idShift:      IDXBinaryShiftL,
	IDShiftR >> idShift:      IDXBinaryShiftR,
	IDAmp >> idShift:         IDXBinaryAmp,
	IDAmpHat >> idShift:      IDXBinaryAmpHat,
	IDPipe >> idShift:        IDXBinaryPipe,
	IDHat >> idShift:         IDXBinaryHat,
	IDNotEq >> idShift:       IDXBinaryNotEq,
	IDLessThan >> idShift:    IDXBinaryLessThan,
	IDLessEq >> idShift:      IDXBinaryLessEq,
	IDEqEq >> idShift:        IDXBinaryEqEq,
	IDGreaterEq >> idShift:   IDXBinaryGreaterEq,
	IDGreaterThan >> idShift: IDXBinaryGreaterThan,
	IDAnd >> idShift:         IDXBinaryAnd,
	IDOr >> idShift:          IDXBinaryOr,
	IDAs >> idShift:          IDXBinaryAs,
}

var associativeForms = [256]ID{
	IDPlus >> idShift: IDXAssociativePlus,
	IDStar >> idShift: IDXAssociativeStar,
	IDAmp >> idShift:  IDXAssociativeAmp,
	IDPipe >> idShift: IDXAssociativePipe,
	IDHat >> idShift:  IDXAssociativeHat,
	IDAnd >> idShift:  IDXAssociativeAnd,
	IDOr >> idShift:   IDXAssociativeOr,
}
