// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package token

// Flags are the low 16 bits of an ID. For example, they signify whether a
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
	FlagsNumType           = Flags(0x4000)
	flagsUnused            = Flags(0x8000)
)

// Key is the high 16 bits of an ID. It is the map key for an IDMap.
type Key uint32

const (
	FlagsBits = 16
	FlagsMask = 1<<FlagsBits - 1
	KeyBits   = 32 - FlagsBits
	KeyShift  = FlagsBits
	maxKey    = 1<<KeyBits - 1
)

// ID combines a Key and Flags.
type ID uint32

func (x ID) UnaryForm() ID       { return unaryForms[0xFF&(x>>KeyShift)] }
func (x ID) BinaryForm() ID      { return binaryForms[0xFF&(x>>KeyShift)] }
func (x ID) AssociativeForm() ID { return associativeForms[0xFF&(x>>KeyShift)] }

func (x ID) Key() Key     { return Key(x >> KeyShift) }
func (x ID) Flags() Flags { return Flags(x & FlagsMask) }

func (x ID) IsBuiltIn() bool { return Key(x>>KeyShift) < nBuiltInKeys }

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
func (x ID) IsNumType() bool           { return Flags(x)&FlagsNumType != 0 }

// Token combines an ID and the line number it was seen.
type Token struct {
	ID   ID
	Line uint32
}

func (t Token) Key() Key     { return Key(t.ID >> KeyShift) }
func (t Token) Flags() Flags { return Flags(t.ID & FlagsMask) }

func (t Token) IsBuiltIn() bool { return Key(t.ID>>KeyShift) < nBuiltInKeys }

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
func (t Token) IsNumType() bool           { return Flags(t.ID)&FlagsNumType != 0 }

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
const nBuiltInKeys = Key(256)

const (
	IDInvalid = ID(0)

	IDOpenParen    = ID(0x10<<KeyShift | FlagsOpen | FlagsTightRight)
	IDCloseParen   = ID(0x11<<KeyShift | FlagsClose | FlagsTightLeft | FlagsImplicitSemicolon)
	IDOpenBracket  = ID(0x12<<KeyShift | FlagsOpen | FlagsTightLeft | FlagsTightRight)
	IDCloseBracket = ID(0x13<<KeyShift | FlagsClose | FlagsTightLeft | FlagsImplicitSemicolon)
	IDOpenCurly    = ID(0x14<<KeyShift | FlagsOpen)
	IDCloseCurly   = ID(0x15<<KeyShift | FlagsClose | FlagsImplicitSemicolon)

	IDDot       = ID(0x20<<KeyShift | FlagsTightLeft | FlagsTightRight)
	IDComma     = ID(0x21<<KeyShift | FlagsTightLeft)
	IDQuestion  = ID(0x22<<KeyShift | FlagsTightLeft | FlagsTightRight)
	IDColon     = ID(0x23<<KeyShift | FlagsTightLeft | FlagsTightRight)
	IDSemicolon = ID(0x24<<KeyShift | FlagsTightLeft)

	IDEq       = ID(0x30<<KeyShift | FlagsAssign)
	IDPlusEq   = ID(0x31<<KeyShift | FlagsAssign)
	IDMinusEq  = ID(0x32<<KeyShift | FlagsAssign)
	IDStarEq   = ID(0x33<<KeyShift | FlagsAssign)
	IDSlashEq  = ID(0x34<<KeyShift | FlagsAssign)
	IDShiftLEq = ID(0x35<<KeyShift | FlagsAssign)
	IDShiftREq = ID(0x36<<KeyShift | FlagsAssign)
	IDAmpEq    = ID(0x37<<KeyShift | FlagsAssign)
	IDAmpHatEq = ID(0x38<<KeyShift | FlagsAssign)
	IDPipeEq   = ID(0x39<<KeyShift | FlagsAssign)
	IDHatEq    = ID(0x3A<<KeyShift | FlagsAssign)

	IDPlus   = ID(0x41<<KeyShift | FlagsBinaryOp | FlagsUnaryOp | FlagsAssociativeOp)
	IDMinus  = ID(0x42<<KeyShift | FlagsBinaryOp | FlagsUnaryOp)
	IDStar   = ID(0x43<<KeyShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDSlash  = ID(0x44<<KeyShift | FlagsBinaryOp)
	IDShiftL = ID(0x45<<KeyShift | FlagsBinaryOp)
	IDShiftR = ID(0x46<<KeyShift | FlagsBinaryOp)
	IDAmp    = ID(0x47<<KeyShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDAmpHat = ID(0x48<<KeyShift | FlagsBinaryOp)
	IDPipe   = ID(0x49<<KeyShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDHat    = ID(0x4A<<KeyShift | FlagsBinaryOp | FlagsAssociativeOp)

	IDNotEq       = ID(0x50<<KeyShift | FlagsBinaryOp)
	IDLessThan    = ID(0x51<<KeyShift | FlagsBinaryOp)
	IDLessEq      = ID(0x52<<KeyShift | FlagsBinaryOp)
	IDEqEq        = ID(0x53<<KeyShift | FlagsBinaryOp)
	IDGreaterEq   = ID(0x54<<KeyShift | FlagsBinaryOp)
	IDGreaterThan = ID(0x55<<KeyShift | FlagsBinaryOp)

	// TODO: sort these by name, when the list has stabilized.
	IDAnd = ID(0x58<<KeyShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDOr  = ID(0x59<<KeyShift | FlagsBinaryOp | FlagsAssociativeOp)
	IDNot = ID(0x5A<<KeyShift | FlagsUnaryOp)
	IDAs  = ID(0x5B<<KeyShift | FlagsBinaryOp)

	// TODO: sort these by name, when the list has stabilized.
	IDFunc     = ID(0x60<<KeyShift | FlagsOther)
	IDPtr      = ID(0x61<<KeyShift | FlagsOther)
	IDAssert   = ID(0x62<<KeyShift | FlagsOther)
	IDFor      = ID(0x63<<KeyShift | FlagsOther)
	IDIf       = ID(0x64<<KeyShift | FlagsOther)
	IDElse     = ID(0x65<<KeyShift | FlagsOther)
	IDReturn   = ID(0x66<<KeyShift | FlagsOther | FlagsImplicitSemicolon)
	IDBreak    = ID(0x67<<KeyShift | FlagsOther | FlagsImplicitSemicolon)
	IDContinue = ID(0x68<<KeyShift | FlagsOther | FlagsImplicitSemicolon)
	IDStruct   = ID(0x69<<KeyShift | FlagsOther)
	IDUse      = ID(0x6A<<KeyShift | FlagsOther)

	IDFalse = ID(0x90<<KeyShift | FlagsLiteral | FlagsImplicitSemicolon)
	IDTrue  = ID(0x91<<KeyShift | FlagsLiteral | FlagsImplicitSemicolon)

	IDI8    = ID(0xA0<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDI16   = ID(0xA1<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDI32   = ID(0xA2<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDI64   = ID(0xA3<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDU8    = ID(0xA4<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDU16   = ID(0xA5<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDU32   = ID(0xA6<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDU64   = ID(0xA7<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDUsize = ID(0xA8<<KeyShift | FlagsIdent | FlagsImplicitSemicolon | FlagsNumType)
	IDBool  = ID(0xA9<<KeyShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBuf1  = ID(0xAA<<KeyShift | FlagsIdent | FlagsImplicitSemicolon)
	IDBuf2  = ID(0xAB<<KeyShift | FlagsIdent | FlagsImplicitSemicolon)

	IDUnderscore = ID(0xB0<<KeyShift | FlagsIdent | FlagsImplicitSemicolon)
	IDThis       = ID(0xB1<<KeyShift | FlagsIdent | FlagsImplicitSemicolon)
)

// The IDXFoo IDs are not returned by the tokenizer. They are used by the
// ast.Node ID-typed fields to disambiguate e.g. unary vs binary plus.
const (
	minXKey = 0xC0
	maxXKey = 0xFF

	IDXUnaryPlus  = ID(0xC0<<KeyShift | FlagsUnaryOp)
	IDXUnaryMinus = ID(0xC1<<KeyShift | FlagsUnaryOp)
	IDXUnaryNot   = ID(0xC2<<KeyShift | FlagsUnaryOp)

	IDXBinaryPlus        = ID(0xD0<<KeyShift | FlagsBinaryOp)
	IDXBinaryMinus       = ID(0xD1<<KeyShift | FlagsBinaryOp)
	IDXBinaryStar        = ID(0xD2<<KeyShift | FlagsBinaryOp)
	IDXBinarySlash       = ID(0xD3<<KeyShift | FlagsBinaryOp)
	IDXBinaryShiftL      = ID(0xD4<<KeyShift | FlagsBinaryOp)
	IDXBinaryShiftR      = ID(0xD5<<KeyShift | FlagsBinaryOp)
	IDXBinaryAmp         = ID(0xD6<<KeyShift | FlagsBinaryOp)
	IDXBinaryAmpHat      = ID(0xD7<<KeyShift | FlagsBinaryOp)
	IDXBinaryPipe        = ID(0xD8<<KeyShift | FlagsBinaryOp)
	IDXBinaryHat         = ID(0xD9<<KeyShift | FlagsBinaryOp)
	IDXBinaryNotEq       = ID(0xDA<<KeyShift | FlagsBinaryOp)
	IDXBinaryLessThan    = ID(0xDB<<KeyShift | FlagsBinaryOp)
	IDXBinaryLessEq      = ID(0xDC<<KeyShift | FlagsBinaryOp)
	IDXBinaryEqEq        = ID(0xDD<<KeyShift | FlagsBinaryOp)
	IDXBinaryGreaterEq   = ID(0xDE<<KeyShift | FlagsBinaryOp)
	IDXBinaryGreaterThan = ID(0xDF<<KeyShift | FlagsBinaryOp)
	IDXBinaryAnd         = ID(0xE0<<KeyShift | FlagsBinaryOp)
	IDXBinaryOr          = ID(0xE1<<KeyShift | FlagsBinaryOp)
	IDXBinaryAs          = ID(0xE2<<KeyShift | FlagsBinaryOp)

	IDXAssociativePlus = ID(0xF0<<KeyShift | FlagsAssociativeOp)
	IDXAssociativeStar = ID(0xF1<<KeyShift | FlagsAssociativeOp)
	IDXAssociativeAmp  = ID(0xF2<<KeyShift | FlagsAssociativeOp)
	IDXAssociativePipe = ID(0xF3<<KeyShift | FlagsAssociativeOp)
	IDXAssociativeHat  = ID(0xF4<<KeyShift | FlagsAssociativeOp)
	IDXAssociativeAnd  = ID(0xF5<<KeyShift | FlagsAssociativeOp)
	IDXAssociativeOr   = ID(0xF6<<KeyShift | FlagsAssociativeOp)
)

var builtInsByKey = [nBuiltInKeys]struct {
	name string
	id   ID
}{
	IDOpenParen >> KeyShift:    {"(", IDOpenParen},
	IDCloseParen >> KeyShift:   {")", IDCloseParen},
	IDOpenBracket >> KeyShift:  {"[", IDOpenBracket},
	IDCloseBracket >> KeyShift: {"]", IDCloseBracket},
	IDOpenCurly >> KeyShift:    {"{", IDOpenCurly},
	IDCloseCurly >> KeyShift:   {"}", IDCloseCurly},

	IDDot >> KeyShift:       {".", IDDot},
	IDComma >> KeyShift:     {",", IDComma},
	IDQuestion >> KeyShift:  {"?", IDQuestion},
	IDColon >> KeyShift:     {":", IDColon},
	IDSemicolon >> KeyShift: {";", IDSemicolon},

	IDEq >> KeyShift:       {"=", IDEq},
	IDPlusEq >> KeyShift:   {"+=", IDPlusEq},
	IDMinusEq >> KeyShift:  {"-=", IDMinusEq},
	IDStarEq >> KeyShift:   {"*=", IDStarEq},
	IDSlashEq >> KeyShift:  {"/=", IDSlashEq},
	IDShiftLEq >> KeyShift: {"<<=", IDShiftLEq},
	IDShiftREq >> KeyShift: {">>=", IDShiftREq},
	IDAmpEq >> KeyShift:    {"&=", IDAmpEq},
	IDAmpHatEq >> KeyShift: {"&^=", IDAmpHatEq},
	IDPipeEq >> KeyShift:   {"|=", IDPipeEq},
	IDHatEq >> KeyShift:    {"^=", IDHatEq},

	IDPlus >> KeyShift:   {"+", IDPlus},
	IDMinus >> KeyShift:  {"-", IDMinus},
	IDStar >> KeyShift:   {"*", IDStar},
	IDSlash >> KeyShift:  {"/", IDSlash},
	IDShiftL >> KeyShift: {"<<", IDShiftL},
	IDShiftR >> KeyShift: {">>", IDShiftR},
	IDAmp >> KeyShift:    {"&", IDAmp},
	IDAmpHat >> KeyShift: {"&^", IDAmpHat},
	IDPipe >> KeyShift:   {"|", IDPipe},
	IDHat >> KeyShift:    {"^", IDHat},

	IDNotEq >> KeyShift:       {"!=", IDNotEq},
	IDLessThan >> KeyShift:    {"<", IDLessThan},
	IDLessEq >> KeyShift:      {"<=", IDLessEq},
	IDEqEq >> KeyShift:        {"==", IDEqEq},
	IDGreaterEq >> KeyShift:   {">=", IDGreaterEq},
	IDGreaterThan >> KeyShift: {">", IDGreaterThan},

	IDAnd >> KeyShift: {"and", IDAnd},
	IDOr >> KeyShift:  {"or", IDOr},
	IDNot >> KeyShift: {"not", IDNot},
	IDAs >> KeyShift:  {"as", IDAs},

	IDFunc >> KeyShift:     {"func", IDFunc},
	IDPtr >> KeyShift:      {"ptr", IDPtr},
	IDAssert >> KeyShift:   {"assert", IDAssert},
	IDFor >> KeyShift:      {"for", IDFor},
	IDIf >> KeyShift:       {"if", IDIf},
	IDElse >> KeyShift:     {"else", IDElse},
	IDReturn >> KeyShift:   {"return", IDReturn},
	IDBreak >> KeyShift:    {"break", IDBreak},
	IDContinue >> KeyShift: {"continue", IDContinue},
	IDStruct >> KeyShift:   {"struct", IDStruct},
	IDUse >> KeyShift:      {"use", IDUse},

	IDFalse >> KeyShift: {"false", IDFalse},
	IDTrue >> KeyShift:  {"true", IDTrue},

	IDI8 >> KeyShift:    {"i8", IDI8},
	IDI16 >> KeyShift:   {"i16", IDI16},
	IDI32 >> KeyShift:   {"i32", IDI32},
	IDI64 >> KeyShift:   {"i64", IDI64},
	IDU8 >> KeyShift:    {"u8", IDU8},
	IDU16 >> KeyShift:   {"u16", IDU16},
	IDU32 >> KeyShift:   {"u32", IDU32},
	IDU64 >> KeyShift:   {"u64", IDU64},
	IDUsize >> KeyShift: {"usize", IDUsize},
	IDBool >> KeyShift:  {"bool", IDBool},
	IDBuf1 >> KeyShift:  {"buf1", IDBuf1},
	IDBuf2 >> KeyShift:  {"buf2", IDBuf2},

	IDUnderscore >> KeyShift: {"_", IDUnderscore},
	IDThis >> KeyShift:       {"this", IDThis},
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

	// 1<<KeyShift is a non-zero ID with zero Flags. It is an invalid ID,
	// signifying that lookup should continue in the lexers array below.

	'&': 1 << KeyShift,
	'|': 1 << KeyShift,
	'^': 1 << KeyShift,
	'+': 1 << KeyShift,
	'-': 1 << KeyShift,
	'*': 1 << KeyShift,
	'/': 1 << KeyShift,
	'=': 1 << KeyShift,
	'<': 1 << KeyShift,
	'>': 1 << KeyShift,
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
	IDPlus >> KeyShift:  IDXUnaryPlus,
	IDMinus >> KeyShift: IDXUnaryMinus,
	IDNot >> KeyShift:   IDXUnaryNot,
}

var binaryForms = [256]ID{
	IDPlus >> KeyShift:        IDXBinaryPlus,
	IDMinus >> KeyShift:       IDXBinaryMinus,
	IDStar >> KeyShift:        IDXBinaryStar,
	IDSlash >> KeyShift:       IDXBinarySlash,
	IDShiftL >> KeyShift:      IDXBinaryShiftL,
	IDShiftR >> KeyShift:      IDXBinaryShiftR,
	IDAmp >> KeyShift:         IDXBinaryAmp,
	IDAmpHat >> KeyShift:      IDXBinaryAmpHat,
	IDPipe >> KeyShift:        IDXBinaryPipe,
	IDHat >> KeyShift:         IDXBinaryHat,
	IDNotEq >> KeyShift:       IDXBinaryNotEq,
	IDLessThan >> KeyShift:    IDXBinaryLessThan,
	IDLessEq >> KeyShift:      IDXBinaryLessEq,
	IDEqEq >> KeyShift:        IDXBinaryEqEq,
	IDGreaterEq >> KeyShift:   IDXBinaryGreaterEq,
	IDGreaterThan >> KeyShift: IDXBinaryGreaterThan,
	IDAnd >> KeyShift:         IDXBinaryAnd,
	IDOr >> KeyShift:          IDXBinaryOr,
	IDAs >> KeyShift:          IDXBinaryAs,
}

var associativeForms = [256]ID{
	IDPlus >> KeyShift: IDXAssociativePlus,
	IDStar >> KeyShift: IDXAssociativeStar,
	IDAmp >> KeyShift:  IDXAssociativeAmp,
	IDPipe >> KeyShift: IDXAssociativePipe,
	IDHat >> KeyShift:  IDXAssociativeHat,
	IDAnd >> KeyShift:  IDXAssociativeAnd,
	IDOr >> KeyShift:   IDXAssociativeOr,
}
