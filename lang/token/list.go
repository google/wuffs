// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package token

// MaxIntBits is the largest size (in bits) of the i8, u8, i16, u16, etc.
// integer types.
const MaxIntBits = 64

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
	FlagsOther      = Flags(0x0001)
	FlagsLiteral    = Flags(0x0010)
	FlagsNumLiteral = Flags(0x0020)
	FlagsStrLiteral = Flags(0x0040)
	FlagsIdent      = Flags(0x0080)
	flagsUnused     = Flags(0x8000)
)

// Key is the high 16 bits of an ID. It is the map key for a Map.
type Key uint32

func (k Key) isXOp() bool { return minXOpKey <= k && k <= maxXOpKey }

const (
	FlagsBits = 16
	FlagsMask = 1<<FlagsBits - 1
	KeyBits   = 32 - FlagsBits
	KeyShift  = FlagsBits
	maxKey    = 1<<KeyBits - 1
)

// ID combines a Key and Flags.
type ID uint32

// Str returns a string form of x.
func (x ID) Str(m *Map) string { return m.ByID(x) }

// TODO: the XxxForm methods should return 0 if x > 0xFF.
func (x ID) AmbiguousForm() ID   { return ambiguousForms[0xFF&(x>>KeyShift)] }
func (x ID) UnaryForm() ID       { return unaryForms[0xFF&(x>>KeyShift)] }
func (x ID) BinaryForm() ID      { return binaryForms[0xFF&(x>>KeyShift)] }
func (x ID) AssociativeForm() ID { return associativeForms[0xFF&(x>>KeyShift)] }

func (x ID) Key() Key     { return Key(x >> KeyShift) }
func (x ID) Flags() Flags { return Flags(x & FlagsMask) }

func (x ID) IsBuiltIn() bool { return Key(x>>KeyShift) < nBuiltInKeys }

func (x ID) IsUnaryOp() bool { return minOpKey <= x.Key() && x.Key() <= maxOpKey && x.UnaryForm() != 0 }
func (x ID) IsBinaryOp() bool {
	return minOpKey <= x.Key() && x.Key() <= maxOpKey && x.BinaryForm() != 0
}
func (x ID) IsAssociativeOp() bool {
	return minOpKey <= x.Key() && x.Key() <= maxOpKey && x.AssociativeForm() != 0
}

func (x ID) IsLiteral() bool    { return Flags(x)&FlagsLiteral != 0 }
func (x ID) IsNumLiteral() bool { return Flags(x)&FlagsNumLiteral != 0 }
func (x ID) IsStrLiteral() bool { return Flags(x)&FlagsStrLiteral != 0 }
func (x ID) IsIdent() bool      { return Flags(x)&FlagsIdent != 0 }

func (x ID) IsOpen() bool       { return x.Key() < Key(len(isOpen)) && isOpen[x.Key()] }
func (x ID) IsClose() bool      { return x.Key() < Key(len(isClose)) && isClose[x.Key()] }
func (x ID) IsTightLeft() bool  { return x.Key() < Key(len(isTightLeft)) && isTightLeft[x.Key()] }
func (x ID) IsTightRight() bool { return x.Key() < Key(len(isTightRight)) && isTightRight[x.Key()] }

func (x ID) IsAssign() bool  { return minAssignKey <= x.Key() && x.Key() <= maxAssignKey }
func (x ID) IsNumType() bool { return minNumTypeKey <= x.Key() && x.Key() <= maxNumTypeKey }

func (x ID) IsImplicitSemicolon() bool {
	return x.IsLiteral() || x.IsIdent() ||
		(x.Key() < Key(len(isImplicitSemicolon)) && isImplicitSemicolon[x.Key()])
}

func (x ID) IsXUnaryOp() bool       { return x.Key().isXOp() && x.IsUnaryOp() }
func (x ID) IsXBinaryOp() bool      { return x.Key().isXOp() && x.IsBinaryOp() }
func (x ID) IsXAssociativeOp() bool { return x.Key().isXOp() && x.IsAssociativeOp() }

// QID is a qualified ID, such as "foo.bar". QID[0] is "foo"'s ID and QID[1] is
// "bar"'s. QID[0] may be 0 for a plain "bar".
type QID [2]ID

func (x QID) IsZero() bool { return x == QID{} }

// Str returns a string form of x.
func (x QID) Str(m *Map) string {
	if x[0] != 0 {
		return m.ByID(x[0]) + "." + m.ByID(x[1])
	}
	return m.ByID(x[1])
}

// QQID is a double-qualified ID, such as "receiverPkg.receiverName.funcName".
type QQID [3]ID

func (x QQID) IsZero() bool { return x == QQID{} }

// Str returns a string form of x.
func (x QQID) Str(m *Map) string {
	if x[0] != 0 {
		return m.ByID(x[0]) + "." + m.ByID(x[1]) + "." + m.ByID(x[2])
	}
	if x[1] != 0 {
		return m.ByID(x[1]) + "." + m.ByID(x[2])
	}
	return m.ByID(x[2])
}

// Token combines an ID and the line number it was seen.
type Token struct {
	ID   ID
	Line uint32
}

// nBuiltInKeys is the number of built-in Keys. The packing is:
//  - Zero is invalid.
//  - [0x01, 0x0F] are reserved for implementation details.
//  - [0x10, 0x1F] are squiggly punctuation, such as "(", ")" and ";".
//  - [0x20, 0x2F] are squiggly assignments, such as "=" and "+=".
//  - [0x30, 0x4F] are operators, such as "+", "==" and "not".
//  - [0x50, 0x6F] are keywords, such as "if" and "return".
//  - [0x70, 0x73] are type modifiers, such as "ptr" and "slice".
//  - [0x74, 0x77] are literals, such as "false" and "true".
//  - [0x78, 0xCF] are identifiers, such as "bool", "u32" and "read_u8".
//  - [0xD0, 0xFF] are disambiguation forms, e.g. unary "+" vs binary "+".
// TODO: move the ops and x-ops together.
//
// "Squiggly" means a sequence of non-alpha-numeric characters, such as "+" and
// "&=". Roughly speaking, their Keys range in [0x01, 0x4F], or disambiguation
// forms range in [0xD0, 0xFF], but vice versa does not necessarily hold. For
// example, the "and" operator is not "squiggly" but it is within [0x01, 0x4F].
const nBuiltInKeys = Key(256)

const (
	KeyInvalid = Key(0)

	KeyDoubleZ = Key(IDDoubleZ >> KeyShift)
	KeyDiamond = Key(IDDiamond >> KeyShift)

	KeyOpenParen    = Key(IDOpenParen >> KeyShift)
	KeyCloseParen   = Key(IDCloseParen >> KeyShift)
	KeyOpenBracket  = Key(IDOpenBracket >> KeyShift)
	KeyCloseBracket = Key(IDCloseBracket >> KeyShift)
	KeyOpenCurly    = Key(IDOpenCurly >> KeyShift)
	KeyCloseCurly   = Key(IDCloseCurly >> KeyShift)

	KeyDot       = Key(IDDot >> KeyShift)
	KeyDotDot    = Key(IDDotDot >> KeyShift)
	KeyComma     = Key(IDComma >> KeyShift)
	KeyExclam    = Key(IDExclam >> KeyShift)
	KeyQuestion  = Key(IDQuestion >> KeyShift)
	KeyColon     = Key(IDColon >> KeyShift)
	KeySemicolon = Key(IDSemicolon >> KeyShift)
	KeyDollar    = Key(IDDollar >> KeyShift)

	KeyEq          = Key(IDEq >> KeyShift)
	KeyPlusEq      = Key(IDPlusEq >> KeyShift)
	KeyMinusEq     = Key(IDMinusEq >> KeyShift)
	KeyStarEq      = Key(IDStarEq >> KeyShift)
	KeySlashEq     = Key(IDSlashEq >> KeyShift)
	KeyShiftLEq    = Key(IDShiftLEq >> KeyShift)
	KeyShiftREq    = Key(IDShiftREq >> KeyShift)
	KeyAmpEq       = Key(IDAmpEq >> KeyShift)
	KeyAmpHatEq    = Key(IDAmpHatEq >> KeyShift)
	KeyPipeEq      = Key(IDPipeEq >> KeyShift)
	KeyHatEq       = Key(IDHatEq >> KeyShift)
	KeyPercentEq   = Key(IDPercentEq >> KeyShift)
	KeyTildePlusEq = Key(IDTildePlusEq >> KeyShift)

	KeyPlus      = Key(IDPlus >> KeyShift)
	KeyMinus     = Key(IDMinus >> KeyShift)
	KeyStar      = Key(IDStar >> KeyShift)
	KeySlash     = Key(IDSlash >> KeyShift)
	KeyShiftL    = Key(IDShiftL >> KeyShift)
	KeyShiftR    = Key(IDShiftR >> KeyShift)
	KeyAmp       = Key(IDAmp >> KeyShift)
	KeyAmpHat    = Key(IDAmpHat >> KeyShift)
	KeyPipe      = Key(IDPipe >> KeyShift)
	KeyHat       = Key(IDHat >> KeyShift)
	KeyPercent   = Key(IDPercent >> KeyShift)
	KeyTildePlus = Key(IDTildePlus >> KeyShift)

	KeyNotEq       = Key(IDNotEq >> KeyShift)
	KeyLessThan    = Key(IDLessThan >> KeyShift)
	KeyLessEq      = Key(IDLessEq >> KeyShift)
	KeyEqEq        = Key(IDEqEq >> KeyShift)
	KeyGreaterEq   = Key(IDGreaterEq >> KeyShift)
	KeyGreaterThan = Key(IDGreaterThan >> KeyShift)

	// TODO: sort these by name, when the list has stabilized.
	KeyAnd   = Key(IDAnd >> KeyShift)
	KeyOr    = Key(IDOr >> KeyShift)
	KeyNot   = Key(IDNot >> KeyShift)
	KeyAs    = Key(IDAs >> KeyShift)
	KeyRef   = Key(IDRef >> KeyShift)
	KeyDeref = Key(IDDeref >> KeyShift)

	// TODO: sort these by name, when the list has stabilized.
	KeyFunc       = Key(IDFunc >> KeyShift)
	KeyAssert     = Key(IDAssert >> KeyShift)
	KeyWhile      = Key(IDWhile >> KeyShift)
	KeyIf         = Key(IDIf >> KeyShift)
	KeyElse       = Key(IDElse >> KeyShift)
	KeyReturn     = Key(IDReturn >> KeyShift)
	KeyBreak      = Key(IDBreak >> KeyShift)
	KeyContinue   = Key(IDContinue >> KeyShift)
	KeyStruct     = Key(IDStruct >> KeyShift)
	KeyUse        = Key(IDUse >> KeyShift)
	KeyVar        = Key(IDVar >> KeyShift)
	KeyPre        = Key(IDPre >> KeyShift)
	KeyInv        = Key(IDInv >> KeyShift)
	KeyPost       = Key(IDPost >> KeyShift)
	KeyVia        = Key(IDVia >> KeyShift)
	KeyPub        = Key(IDPub >> KeyShift)
	KeyPri        = Key(IDPri >> KeyShift)
	KeyError      = Key(IDError >> KeyShift)
	KeySuspension = Key(IDSuspension >> KeyShift)
	KeyPackageID  = Key(IDPackageID >> KeyShift)
	KeyConst      = Key(IDConst >> KeyShift)
	KeyTry        = Key(IDTry >> KeyShift)
	KeyIterate    = Key(IDIterate >> KeyShift)
	KeyYield      = Key(IDYield >> KeyShift)

	KeyArray = Key(IDArray >> KeyShift)
	KeyNptr  = Key(IDNptr >> KeyShift)
	KeyPtr   = Key(IDPtr >> KeyShift)
	KeySlice = Key(IDSlice >> KeyShift)

	KeyFalse = Key(IDFalse >> KeyShift)
	KeyTrue  = Key(IDTrue >> KeyShift)
	KeyZero  = Key(IDZero >> KeyShift)

	KeyUnderscore = Key(IDUnderscore >> KeyShift)
	KeyThis       = Key(IDThis >> KeyShift)
	KeyIn         = Key(IDIn >> KeyShift)
	KeyOut        = Key(IDOut >> KeyShift)
	KeySLICE      = Key(IDSLICE >> KeyShift)
	KeyBase       = Key(IDBase >> KeyShift)

	KeyI8          = Key(IDI8 >> KeyShift)
	KeyI16         = Key(IDI16 >> KeyShift)
	KeyI32         = Key(IDI32 >> KeyShift)
	KeyI64         = Key(IDI64 >> KeyShift)
	KeyU8          = Key(IDU8 >> KeyShift)
	KeyU16         = Key(IDU16 >> KeyShift)
	KeyU32         = Key(IDU32 >> KeyShift)
	KeyU64         = Key(IDU64 >> KeyShift)
	KeyBool        = Key(IDBool >> KeyShift)
	KeyIOReader    = Key(IDIOReader >> KeyShift)
	KeyIOWriter    = Key(IDIOWriter >> KeyShift)
	KeyStatus      = Key(IDStatus >> KeyShift)
	KeyImageConfig = Key(IDImageConfig >> KeyShift)

	KeyMark       = Key(IDMark >> KeyShift)
	KeyReadU8     = Key(IDReadU8 >> KeyShift)
	KeyReadU16BE  = Key(IDReadU16BE >> KeyShift)
	KeyReadU16LE  = Key(IDReadU16LE >> KeyShift)
	KeyReadU32BE  = Key(IDReadU32BE >> KeyShift)
	KeyReadU32LE  = Key(IDReadU32LE >> KeyShift)
	KeyReadU64BE  = Key(IDReadU64BE >> KeyShift)
	KeyReadU64LE  = Key(IDReadU64LE >> KeyShift)
	KeySinceMark  = Key(IDSinceMark >> KeyShift)
	KeyWriteU8    = Key(IDWriteU8 >> KeyShift)
	KeyWriteU16BE = Key(IDWriteU16BE >> KeyShift)
	KeyWriteU16LE = Key(IDWriteU16LE >> KeyShift)
	KeyWriteU32BE = Key(IDWriteU32BE >> KeyShift)
	KeyWriteU32LE = Key(IDWriteU32LE >> KeyShift)
	KeyWriteU64BE = Key(IDWriteU64BE >> KeyShift)
	KeyWriteU64LE = Key(IDWriteU64LE >> KeyShift)

	KeyIsError           = Key(IDIsError >> KeyShift)
	KeyIsOK              = Key(IDIsOK >> KeyShift)
	KeyIsSuspension      = Key(IDIsSuspension >> KeyShift)
	KeyCopyFromHistory32 = Key(IDCopyFromHistory32 >> KeyShift)
	KeyCopyFromReader32  = Key(IDCopyFromReader32 >> KeyShift)
	KeyCopyFromSlice     = Key(IDCopyFromSlice >> KeyShift)
	KeyCopyFromSlice32   = Key(IDCopyFromSlice32 >> KeyShift)
	KeySkip32            = Key(IDSkip32 >> KeyShift)
	KeySkip64            = Key(IDSkip64 >> KeyShift)
	KeyLength            = Key(IDLength >> KeyShift)
	KeyAvailable         = Key(IDAvailable >> KeyShift)
	KeyPrefix            = Key(IDPrefix >> KeyShift)
	KeySuffix            = Key(IDSuffix >> KeyShift)
	KeyLimit             = Key(IDLimit >> KeyShift)
	KeyLowBits           = Key(IDLowBits >> KeyShift)
	KeyHighBits          = Key(IDHighBits >> KeyShift)
	KeyUnreadU8          = Key(IDUnreadU8 >> KeyShift)
	KeyIsMarked          = Key(IDIsMarked >> KeyShift)

	KeyXUnaryPlus  = Key(IDXUnaryPlus >> KeyShift)
	KeyXUnaryMinus = Key(IDXUnaryMinus >> KeyShift)
	KeyXUnaryNot   = Key(IDXUnaryNot >> KeyShift)
	KeyXUnaryRef   = Key(IDXUnaryRef >> KeyShift)
	KeyXUnaryDeref = Key(IDXUnaryDeref >> KeyShift)

	KeyXBinaryPlus        = Key(IDXBinaryPlus >> KeyShift)
	KeyXBinaryMinus       = Key(IDXBinaryMinus >> KeyShift)
	KeyXBinaryStar        = Key(IDXBinaryStar >> KeyShift)
	KeyXBinarySlash       = Key(IDXBinarySlash >> KeyShift)
	KeyXBinaryShiftL      = Key(IDXBinaryShiftL >> KeyShift)
	KeyXBinaryShiftR      = Key(IDXBinaryShiftR >> KeyShift)
	KeyXBinaryAmp         = Key(IDXBinaryAmp >> KeyShift)
	KeyXBinaryAmpHat      = Key(IDXBinaryAmpHat >> KeyShift)
	KeyXBinaryPipe        = Key(IDXBinaryPipe >> KeyShift)
	KeyXBinaryHat         = Key(IDXBinaryHat >> KeyShift)
	KeyXBinaryPercent     = Key(IDXBinaryPercent >> KeyShift)
	KeyXBinaryNotEq       = Key(IDXBinaryNotEq >> KeyShift)
	KeyXBinaryLessThan    = Key(IDXBinaryLessThan >> KeyShift)
	KeyXBinaryLessEq      = Key(IDXBinaryLessEq >> KeyShift)
	KeyXBinaryEqEq        = Key(IDXBinaryEqEq >> KeyShift)
	KeyXBinaryGreaterEq   = Key(IDXBinaryGreaterEq >> KeyShift)
	KeyXBinaryGreaterThan = Key(IDXBinaryGreaterThan >> KeyShift)
	KeyXBinaryAnd         = Key(IDXBinaryAnd >> KeyShift)
	KeyXBinaryOr          = Key(IDXBinaryOr >> KeyShift)
	KeyXBinaryAs          = Key(IDXBinaryAs >> KeyShift)
	KeyXBinaryTildePlus   = Key(IDXBinaryTildePlus >> KeyShift)

	KeyXAssociativePlus = Key(IDXAssociativePlus >> KeyShift)
	KeyXAssociativeStar = Key(IDXAssociativeStar >> KeyShift)
	KeyXAssociativeAmp  = Key(IDXAssociativeAmp >> KeyShift)
	KeyXAssociativePipe = Key(IDXAssociativePipe >> KeyShift)
	KeyXAssociativeHat  = Key(IDXAssociativeHat >> KeyShift)
	KeyXAssociativeAnd  = Key(IDXAssociativeAnd >> KeyShift)
	KeyXAssociativeOr   = Key(IDXAssociativeOr >> KeyShift)
)

const (
	IDInvalid = ID(0)

	IDDoubleZ = ID(0x01<<KeyShift | FlagsOther)
	IDDiamond = ID(0x02<<KeyShift | FlagsIdent)

	IDOpenParen    = ID(0x10<<KeyShift | FlagsOther)
	IDCloseParen   = ID(0x11<<KeyShift | FlagsOther)
	IDOpenBracket  = ID(0x12<<KeyShift | FlagsOther)
	IDCloseBracket = ID(0x13<<KeyShift | FlagsOther)
	IDOpenCurly    = ID(0x14<<KeyShift | FlagsOther)
	IDCloseCurly   = ID(0x15<<KeyShift | FlagsOther)

	IDDot       = ID(0x18<<KeyShift | FlagsOther)
	IDDotDot    = ID(0x19<<KeyShift | FlagsOther)
	IDComma     = ID(0x1A<<KeyShift | FlagsOther)
	IDExclam    = ID(0x1B<<KeyShift | FlagsOther)
	IDQuestion  = ID(0x1C<<KeyShift | FlagsOther)
	IDColon     = ID(0x1D<<KeyShift | FlagsOther)
	IDSemicolon = ID(0x1E<<KeyShift | FlagsOther)
	IDDollar    = ID(0x1F<<KeyShift | FlagsOther)

	minAssignKey = 0x20
	maxAssignKey = 0x2F

	IDEq          = ID(0x20<<KeyShift | FlagsOther)
	IDPlusEq      = ID(0x21<<KeyShift | FlagsOther)
	IDMinusEq     = ID(0x22<<KeyShift | FlagsOther)
	IDStarEq      = ID(0x23<<KeyShift | FlagsOther)
	IDSlashEq     = ID(0x24<<KeyShift | FlagsOther)
	IDShiftLEq    = ID(0x25<<KeyShift | FlagsOther)
	IDShiftREq    = ID(0x26<<KeyShift | FlagsOther)
	IDAmpEq       = ID(0x27<<KeyShift | FlagsOther)
	IDAmpHatEq    = ID(0x28<<KeyShift | FlagsOther)
	IDPipeEq      = ID(0x29<<KeyShift | FlagsOther)
	IDHatEq       = ID(0x2A<<KeyShift | FlagsOther)
	IDPercentEq   = ID(0x2B<<KeyShift | FlagsOther)
	IDTildePlusEq = ID(0x2C<<KeyShift | FlagsOther)

	minOpKey = 0x30
	maxOpKey = 0xFF

	IDPlus      = ID(0x31<<KeyShift | FlagsOther)
	IDMinus     = ID(0x32<<KeyShift | FlagsOther)
	IDStar      = ID(0x33<<KeyShift | FlagsOther)
	IDSlash     = ID(0x34<<KeyShift | FlagsOther)
	IDShiftL    = ID(0x35<<KeyShift | FlagsOther)
	IDShiftR    = ID(0x36<<KeyShift | FlagsOther)
	IDAmp       = ID(0x37<<KeyShift | FlagsOther)
	IDAmpHat    = ID(0x38<<KeyShift | FlagsOther)
	IDPipe      = ID(0x39<<KeyShift | FlagsOther)
	IDHat       = ID(0x3A<<KeyShift | FlagsOther)
	IDPercent   = ID(0x3B<<KeyShift | FlagsOther)
	IDTildePlus = ID(0x3C<<KeyShift | FlagsOther)

	IDNotEq       = ID(0x40<<KeyShift | FlagsOther)
	IDLessThan    = ID(0x41<<KeyShift | FlagsOther)
	IDLessEq      = ID(0x42<<KeyShift | FlagsOther)
	IDEqEq        = ID(0x43<<KeyShift | FlagsOther)
	IDGreaterEq   = ID(0x44<<KeyShift | FlagsOther)
	IDGreaterThan = ID(0x45<<KeyShift | FlagsOther)

	// TODO: sort these by name, when the list has stabilized.
	IDAnd   = ID(0x48<<KeyShift | FlagsOther)
	IDOr    = ID(0x49<<KeyShift | FlagsOther)
	IDNot   = ID(0x4A<<KeyShift | FlagsOther)
	IDAs    = ID(0x4B<<KeyShift | FlagsOther)
	IDRef   = ID(0x4C<<KeyShift | FlagsOther)
	IDDeref = ID(0x4D<<KeyShift | FlagsOther)

	// TODO: sort these by name, when the list has stabilized.
	IDFunc       = ID(0x50<<KeyShift | FlagsOther)
	IDAssert     = ID(0x51<<KeyShift | FlagsOther)
	IDWhile      = ID(0x52<<KeyShift | FlagsOther)
	IDIf         = ID(0x53<<KeyShift | FlagsOther)
	IDElse       = ID(0x54<<KeyShift | FlagsOther)
	IDReturn     = ID(0x55<<KeyShift | FlagsOther)
	IDBreak      = ID(0x56<<KeyShift | FlagsOther)
	IDContinue   = ID(0x57<<KeyShift | FlagsOther)
	IDStruct     = ID(0x58<<KeyShift | FlagsOther)
	IDUse        = ID(0x59<<KeyShift | FlagsOther)
	IDVar        = ID(0x5A<<KeyShift | FlagsOther)
	IDPre        = ID(0x5B<<KeyShift | FlagsOther)
	IDInv        = ID(0x5C<<KeyShift | FlagsOther)
	IDPost       = ID(0x5D<<KeyShift | FlagsOther)
	IDVia        = ID(0x5E<<KeyShift | FlagsOther)
	IDPub        = ID(0x5F<<KeyShift | FlagsOther)
	IDPri        = ID(0x60<<KeyShift | FlagsOther)
	IDError      = ID(0x61<<KeyShift | FlagsOther)
	IDSuspension = ID(0x62<<KeyShift | FlagsOther)
	IDPackageID  = ID(0x63<<KeyShift | FlagsOther)
	IDConst      = ID(0x64<<KeyShift | FlagsOther)
	IDTry        = ID(0x65<<KeyShift | FlagsOther)
	IDIterate    = ID(0x66<<KeyShift | FlagsOther)
	IDYield      = ID(0x67<<KeyShift | FlagsOther)

	IDArray = ID(0x70<<KeyShift | FlagsOther)
	IDNptr  = ID(0x71<<KeyShift | FlagsOther)
	IDPtr   = ID(0x72<<KeyShift | FlagsOther)
	IDSlice = ID(0x73<<KeyShift | FlagsOther)

	IDFalse = ID(0x74<<KeyShift | FlagsLiteral)
	IDTrue  = ID(0x75<<KeyShift | FlagsLiteral)
	IDZero  = ID(0x76<<KeyShift | FlagsLiteral | FlagsNumLiteral)

	IDUnderscore = ID(0x78<<KeyShift | FlagsIdent)
	IDThis       = ID(0x79<<KeyShift | FlagsIdent)
	IDIn         = ID(0x7A<<KeyShift | FlagsIdent)
	IDOut        = ID(0x7B<<KeyShift | FlagsIdent)
	IDSLICE      = ID(0x7C<<KeyShift | FlagsIdent)
	IDBase       = ID(0x7D<<KeyShift | FlagsIdent)

	minNumTypeKey = 0x80
	maxNumTypeKey = 0x87

	IDI8  = ID(0x80<<KeyShift | FlagsIdent)
	IDI16 = ID(0x81<<KeyShift | FlagsIdent)
	IDI32 = ID(0x82<<KeyShift | FlagsIdent)
	IDI64 = ID(0x83<<KeyShift | FlagsIdent)
	IDU8  = ID(0x84<<KeyShift | FlagsIdent)
	IDU16 = ID(0x85<<KeyShift | FlagsIdent)
	IDU32 = ID(0x86<<KeyShift | FlagsIdent)
	IDU64 = ID(0x87<<KeyShift | FlagsIdent)

	IDBool        = ID(0x88<<KeyShift | FlagsIdent)
	IDIOReader    = ID(0x89<<KeyShift | FlagsIdent)
	IDIOWriter    = ID(0x8A<<KeyShift | FlagsIdent)
	IDStatus      = ID(0x8B<<KeyShift | FlagsIdent)
	IDImageConfig = ID(0x8C<<KeyShift | FlagsIdent)

	IDMark       = ID(0x90<<KeyShift | FlagsIdent)
	IDReadU8     = ID(0x91<<KeyShift | FlagsIdent)
	IDReadU16BE  = ID(0x92<<KeyShift | FlagsIdent)
	IDReadU16LE  = ID(0x93<<KeyShift | FlagsIdent)
	IDReadU32BE  = ID(0x94<<KeyShift | FlagsIdent)
	IDReadU32LE  = ID(0x95<<KeyShift | FlagsIdent)
	IDReadU64BE  = ID(0x96<<KeyShift | FlagsIdent)
	IDReadU64LE  = ID(0x97<<KeyShift | FlagsIdent)
	IDSinceMark  = ID(0x98<<KeyShift | FlagsIdent)
	IDWriteU8    = ID(0x99<<KeyShift | FlagsIdent)
	IDWriteU16BE = ID(0x9A<<KeyShift | FlagsIdent)
	IDWriteU16LE = ID(0x9B<<KeyShift | FlagsIdent)
	IDWriteU32BE = ID(0x9C<<KeyShift | FlagsIdent)
	IDWriteU32LE = ID(0x9D<<KeyShift | FlagsIdent)
	IDWriteU64BE = ID(0x9E<<KeyShift | FlagsIdent)
	IDWriteU64LE = ID(0x9F<<KeyShift | FlagsIdent)

	IDIsError           = ID(0xA0<<KeyShift | FlagsIdent)
	IDIsOK              = ID(0xA1<<KeyShift | FlagsIdent)
	IDIsSuspension      = ID(0xA2<<KeyShift | FlagsIdent)
	IDCopyFromHistory32 = ID(0xA3<<KeyShift | FlagsIdent)
	IDCopyFromReader32  = ID(0xA4<<KeyShift | FlagsIdent)
	IDCopyFromSlice     = ID(0xA5<<KeyShift | FlagsIdent)
	IDCopyFromSlice32   = ID(0xA6<<KeyShift | FlagsIdent)
	IDSkip32            = ID(0xA7<<KeyShift | FlagsIdent)
	IDSkip64            = ID(0xA8<<KeyShift | FlagsIdent)
	IDLength            = ID(0xA9<<KeyShift | FlagsIdent)
	IDAvailable         = ID(0xAA<<KeyShift | FlagsIdent)
	IDPrefix            = ID(0xAB<<KeyShift | FlagsIdent)
	IDSuffix            = ID(0xAC<<KeyShift | FlagsIdent)
	IDLimit             = ID(0xAD<<KeyShift | FlagsIdent)
	IDLowBits           = ID(0xAE<<KeyShift | FlagsIdent)
	IDHighBits          = ID(0xAF<<KeyShift | FlagsIdent)
	IDUnreadU8          = ID(0xB0<<KeyShift | FlagsIdent)
	IDIsMarked          = ID(0xB1<<KeyShift | FlagsIdent)
)

// The IDXFoo IDs are not returned by the tokenizer. They are used by the
// ast.Node ID-typed fields to disambiguate e.g. unary vs binary plus.
const (
	minXOpKey = 0xD0
	maxXOpKey = 0xFF

	IDXUnaryPlus  = ID(0xD0<<KeyShift | FlagsOther)
	IDXUnaryMinus = ID(0xD1<<KeyShift | FlagsOther)
	IDXUnaryNot   = ID(0xD2<<KeyShift | FlagsOther)
	IDXUnaryRef   = ID(0xD3<<KeyShift | FlagsOther)
	IDXUnaryDeref = ID(0xD4<<KeyShift | FlagsOther)

	IDXBinaryPlus        = ID(0xD8<<KeyShift | FlagsOther)
	IDXBinaryMinus       = ID(0xD9<<KeyShift | FlagsOther)
	IDXBinaryStar        = ID(0xDA<<KeyShift | FlagsOther)
	IDXBinarySlash       = ID(0xDB<<KeyShift | FlagsOther)
	IDXBinaryShiftL      = ID(0xDC<<KeyShift | FlagsOther)
	IDXBinaryShiftR      = ID(0xDD<<KeyShift | FlagsOther)
	IDXBinaryAmp         = ID(0xDE<<KeyShift | FlagsOther)
	IDXBinaryAmpHat      = ID(0xDF<<KeyShift | FlagsOther)
	IDXBinaryPipe        = ID(0xE0<<KeyShift | FlagsOther)
	IDXBinaryHat         = ID(0xE1<<KeyShift | FlagsOther)
	IDXBinaryPercent     = ID(0xE2<<KeyShift | FlagsOther)
	IDXBinaryNotEq       = ID(0xE3<<KeyShift | FlagsOther)
	IDXBinaryLessThan    = ID(0xE4<<KeyShift | FlagsOther)
	IDXBinaryLessEq      = ID(0xE5<<KeyShift | FlagsOther)
	IDXBinaryEqEq        = ID(0xE6<<KeyShift | FlagsOther)
	IDXBinaryGreaterEq   = ID(0xE7<<KeyShift | FlagsOther)
	IDXBinaryGreaterThan = ID(0xE8<<KeyShift | FlagsOther)
	IDXBinaryAnd         = ID(0xE9<<KeyShift | FlagsOther)
	IDXBinaryOr          = ID(0xEA<<KeyShift | FlagsOther)
	IDXBinaryAs          = ID(0xEB<<KeyShift | FlagsOther)
	IDXBinaryTildePlus   = ID(0xEC<<KeyShift | FlagsOther)

	IDXAssociativePlus = ID(0xF0<<KeyShift | FlagsOther)
	IDXAssociativeStar = ID(0xF1<<KeyShift | FlagsOther)
	IDXAssociativeAmp  = ID(0xF2<<KeyShift | FlagsOther)
	IDXAssociativePipe = ID(0xF3<<KeyShift | FlagsOther)
	IDXAssociativeHat  = ID(0xF4<<KeyShift | FlagsOther)
	IDXAssociativeAnd  = ID(0xF5<<KeyShift | FlagsOther)
	IDXAssociativeOr   = ID(0xF6<<KeyShift | FlagsOther)
)

var builtInsByKey = [nBuiltInKeys]struct {
	name string
	id   ID
}{
	// KeyDoubleZ and KeyDiamond (and IDDoubleZ and IDDiamond) are never
	// returned by the tokenizer, as the tokenizer rejects non-ASCII input.
	//
	// The string representations "ℤ" and "◊" are specifically non-ASCII so
	// that no user-defined (non built-in) identifier will conflict with them.

	// KeyDoubleZ is used by the type checker as a dummy-valued built-in Key to
	// represent an ideal integer type (in mathematical terms, the integer ring
	// ℤ), as opposed to a realized integer type whose range is restricted. For
	// example, the u16 type is restricted to [0x0000, 0xFFFF].
	KeyDoubleZ: {"ℤ", IDDoubleZ}, // U+2124 DOUBLE-STRUCK CAPITAL Z

	// KeyDiamond is used by the type checker as a dummy-valued built-in Key to
	// represent a generic type.
	KeyDiamond: {"◊", IDDiamond}, // U+25C7 WHITE DIAMOND

	KeyOpenParen:    {"(", IDOpenParen},
	KeyCloseParen:   {")", IDCloseParen},
	KeyOpenBracket:  {"[", IDOpenBracket},
	KeyCloseBracket: {"]", IDCloseBracket},
	KeyOpenCurly:    {"{", IDOpenCurly},
	KeyCloseCurly:   {"}", IDCloseCurly},

	KeyDot:       {".", IDDot},
	KeyDotDot:    {"..", IDDotDot},
	KeyComma:     {",", IDComma},
	KeyExclam:    {"!", IDExclam},
	KeyQuestion:  {"?", IDQuestion},
	KeyColon:     {":", IDColon},
	KeySemicolon: {";", IDSemicolon},
	KeyDollar:    {"$", IDDollar},

	KeyEq:          {"=", IDEq},
	KeyPlusEq:      {"+=", IDPlusEq},
	KeyMinusEq:     {"-=", IDMinusEq},
	KeyStarEq:      {"*=", IDStarEq},
	KeySlashEq:     {"/=", IDSlashEq},
	KeyShiftLEq:    {"<<=", IDShiftLEq},
	KeyShiftREq:    {">>=", IDShiftREq},
	KeyAmpEq:       {"&=", IDAmpEq},
	KeyAmpHatEq:    {"&^=", IDAmpHatEq},
	KeyPipeEq:      {"|=", IDPipeEq},
	KeyHatEq:       {"^=", IDHatEq},
	KeyPercentEq:   {"%=", IDPercentEq},
	KeyTildePlusEq: {"~+=", IDTildePlusEq},

	KeyPlus:      {"+", IDPlus},
	KeyMinus:     {"-", IDMinus},
	KeyStar:      {"*", IDStar},
	KeySlash:     {"/", IDSlash},
	KeyShiftL:    {"<<", IDShiftL},
	KeyShiftR:    {">>", IDShiftR},
	KeyAmp:       {"&", IDAmp},
	KeyAmpHat:    {"&^", IDAmpHat},
	KeyPipe:      {"|", IDPipe},
	KeyHat:       {"^", IDHat},
	KeyPercent:   {"%", IDPercent},
	KeyTildePlus: {"~+", IDPercent},

	KeyNotEq:       {"!=", IDNotEq},
	KeyLessThan:    {"<", IDLessThan},
	KeyLessEq:      {"<=", IDLessEq},
	KeyEqEq:        {"==", IDEqEq},
	KeyGreaterEq:   {">=", IDGreaterEq},
	KeyGreaterThan: {">", IDGreaterThan},

	KeyAnd:   {"and", IDAnd},
	KeyOr:    {"or", IDOr},
	KeyNot:   {"not", IDNot},
	KeyAs:    {"as", IDAs},
	KeyRef:   {"ref", IDRef},
	KeyDeref: {"deref", IDDeref},

	KeyFunc:       {"func", IDFunc},
	KeyAssert:     {"assert", IDAssert},
	KeyWhile:      {"while", IDWhile},
	KeyIf:         {"if", IDIf},
	KeyElse:       {"else", IDElse},
	KeyReturn:     {"return", IDReturn},
	KeyBreak:      {"break", IDBreak},
	KeyContinue:   {"continue", IDContinue},
	KeyStruct:     {"struct", IDStruct},
	KeyUse:        {"use", IDUse},
	KeyVar:        {"var", IDVar},
	KeyPre:        {"pre", IDPre},
	KeyInv:        {"inv", IDInv},
	KeyPost:       {"post", IDPost},
	KeyVia:        {"via", IDVia},
	KeyPub:        {"pub", IDPub},
	KeyPri:        {"pri", IDPri},
	KeyError:      {"error", IDError},
	KeySuspension: {"suspension", IDSuspension},
	KeyPackageID:  {"packageid", IDPackageID},
	KeyConst:      {"const", IDConst},
	KeyTry:        {"try", IDTry},
	KeyIterate:    {"iterate", IDIterate},
	KeyYield:      {"yield", IDYield},

	KeyArray: {"array", IDArray},
	KeyNptr:  {"nptr", IDNptr},
	KeyPtr:   {"ptr", IDPtr},
	KeySlice: {"slice", IDSlice},

	KeyFalse: {"false", IDFalse},
	KeyTrue:  {"true", IDTrue},
	KeyZero:  {"0", IDZero},

	KeyUnderscore: {"_", IDUnderscore},
	KeyThis:       {"this", IDThis},
	KeyIn:         {"in", IDIn},
	KeyOut:        {"out", IDOut},
	KeySLICE:      {"SLICE", IDSLICE},
	KeyBase:       {"base", IDBase},

	// Change MaxIntBits if a future update adds an i128 or u128 type.

	KeyI8:          {"i8", IDI8},
	KeyI16:         {"i16", IDI16},
	KeyI32:         {"i32", IDI32},
	KeyI64:         {"i64", IDI64},
	KeyU8:          {"u8", IDU8},
	KeyU16:         {"u16", IDU16},
	KeyU32:         {"u32", IDU32},
	KeyU64:         {"u64", IDU64},
	KeyBool:        {"bool", IDBool},
	KeyIOReader:    {"io_reader", IDIOReader},
	KeyIOWriter:    {"io_writer", IDIOWriter},
	KeyStatus:      {"status", IDStatus},
	KeyImageConfig: {"image_config", IDImageConfig},

	KeyMark:       {"mark", IDMark},
	KeyReadU8:     {"read_u8", IDReadU8},
	KeyReadU16BE:  {"read_u16be", IDReadU16BE},
	KeyReadU16LE:  {"read_u16le", IDReadU16LE},
	KeyReadU32BE:  {"read_u32be", IDReadU32BE},
	KeyReadU32LE:  {"read_u32le", IDReadU32LE},
	KeyReadU64BE:  {"read_u64be", IDReadU64BE},
	KeyReadU64LE:  {"read_u64le", IDReadU64LE},
	KeySinceMark:  {"since_mark", IDSinceMark},
	KeyWriteU8:    {"write_u8", IDWriteU8},
	KeyWriteU16BE: {"write_u16be", IDWriteU16BE},
	KeyWriteU16LE: {"write_u16le", IDWriteU16LE},
	KeyWriteU32BE: {"write_u32be", IDWriteU32BE},
	KeyWriteU32LE: {"write_u32le", IDWriteU32LE},
	KeyWriteU64BE: {"write_u64be", IDWriteU64BE},
	KeyWriteU64LE: {"write_u64le", IDWriteU64LE},

	KeyIsError:           {"is_error", IDIsError},
	KeyIsOK:              {"is_ok", IDIsOK},
	KeyIsSuspension:      {"is_suspension", IDIsSuspension},
	KeyCopyFromHistory32: {"copy_from_history32", IDCopyFromHistory32},
	KeyCopyFromReader32:  {"copy_from_reader32", IDCopyFromReader32},
	KeyCopyFromSlice:     {"copy_from_slice", IDCopyFromSlice},
	KeyCopyFromSlice32:   {"copy_from_slice32", IDCopyFromSlice32},
	KeySkip32:            {"skip32", IDSkip32},
	KeySkip64:            {"skip64", IDSkip64},
	KeyLength:            {"length", IDLength},
	KeyAvailable:         {"available", IDAvailable},
	KeyPrefix:            {"prefix", IDPrefix},
	KeySuffix:            {"suffix", IDSuffix},
	KeyLimit:             {"limit", IDLimit},
	KeyLowBits:           {"low_bits", IDLowBits},
	KeyHighBits:          {"high_bits", IDHighBits},
	KeyUnreadU8:          {"unread_u8", IDUnreadU8},
	KeyIsMarked:          {"is_marked", IDIsMarked},
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

	',': IDComma,
	'?': IDQuestion,
	':': IDColon,
	';': IDSemicolon,
	'$': IDDollar,
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
	'.': {
		{".", IDDotDot},
		{"", IDDot},
	},
	'!': {
		{"=", IDNotEq},
		{"", IDExclam},
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
	'%': {
		{"=", IDPercentEq},
		{"", IDPercent},
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
	'~': {
		{"+=", IDTildePlusEq},
		{"+", IDTildePlus},
	},
}

var ambiguousForms = [256]ID{
	KeyXUnaryPlus:  IDPlus,
	KeyXUnaryMinus: IDMinus,
	KeyXUnaryNot:   IDNot,
	KeyXUnaryRef:   IDRef,
	KeyXUnaryDeref: IDDeref,

	KeyXBinaryPlus:        IDPlus,
	KeyXBinaryMinus:       IDMinus,
	KeyXBinaryStar:        IDStar,
	KeyXBinarySlash:       IDSlash,
	KeyXBinaryShiftL:      IDShiftL,
	KeyXBinaryShiftR:      IDShiftR,
	KeyXBinaryAmp:         IDAmp,
	KeyXBinaryAmpHat:      IDAmpHat,
	KeyXBinaryPipe:        IDPipe,
	KeyXBinaryHat:         IDHat,
	KeyXBinaryPercent:     IDPercent,
	KeyXBinaryNotEq:       IDNotEq,
	KeyXBinaryLessThan:    IDLessThan,
	KeyXBinaryLessEq:      IDLessEq,
	KeyXBinaryEqEq:        IDEqEq,
	KeyXBinaryGreaterEq:   IDGreaterEq,
	KeyXBinaryGreaterThan: IDGreaterThan,
	KeyXBinaryAnd:         IDAnd,
	KeyXBinaryOr:          IDOr,
	KeyXBinaryAs:          IDAs,
	KeyXBinaryTildePlus:   IDTildePlus,

	KeyXAssociativePlus: IDPlus,
	KeyXAssociativeStar: IDStar,
	KeyXAssociativeAmp:  IDAmp,
	KeyXAssociativePipe: IDPipe,
	KeyXAssociativeHat:  IDHat,
	KeyXAssociativeAnd:  IDAnd,
	KeyXAssociativeOr:   IDOr,
}

func init() {
	addXForms(&unaryForms)
	addXForms(&binaryForms)
	addXForms(&associativeForms)
}

// addXForms modifies table so that, if table[x] == y, then table[y] = y.
//
// For example, for the unaryForms table, the explicit entries are like:
//  KeyPlus:        IDXUnaryPlus,
// and this function implicitly addes entries like:
//  KeyXUnaryPlus:  IDXUnaryPlus,
func addXForms(table *[256]ID) {
	implicitEntries := [256]ID{}
	for _, y := range table {
		if y != 0 {
			implicitEntries[y.Key()] = y
		}
	}
	for _, y := range implicitEntries {
		if y != 0 {
			table[y.Key()] = y
		}
	}
}

var unaryForms = [256]ID{
	KeyPlus:  IDXUnaryPlus,
	KeyMinus: IDXUnaryMinus,
	KeyNot:   IDXUnaryNot,
	KeyRef:   IDXUnaryRef,
	KeyDeref: IDXUnaryDeref,
}

var binaryForms = [256]ID{
	KeyPlusEq:      IDXBinaryPlus,
	KeyMinusEq:     IDXBinaryMinus,
	KeyStarEq:      IDXBinaryStar,
	KeySlashEq:     IDXBinarySlash,
	KeyShiftLEq:    IDXBinaryShiftL,
	KeyShiftREq:    IDXBinaryShiftR,
	KeyAmpEq:       IDXBinaryAmp,
	KeyAmpHatEq:    IDXBinaryAmpHat,
	KeyPipeEq:      IDXBinaryPipe,
	KeyHatEq:       IDXBinaryHat,
	KeyPercentEq:   IDXBinaryPercent,
	KeyTildePlusEq: IDXBinaryTildePlus,

	KeyPlus:        IDXBinaryPlus,
	KeyMinus:       IDXBinaryMinus,
	KeyStar:        IDXBinaryStar,
	KeySlash:       IDXBinarySlash,
	KeyShiftL:      IDXBinaryShiftL,
	KeyShiftR:      IDXBinaryShiftR,
	KeyAmp:         IDXBinaryAmp,
	KeyAmpHat:      IDXBinaryAmpHat,
	KeyPipe:        IDXBinaryPipe,
	KeyHat:         IDXBinaryHat,
	KeyPercent:     IDXBinaryPercent,
	KeyNotEq:       IDXBinaryNotEq,
	KeyLessThan:    IDXBinaryLessThan,
	KeyLessEq:      IDXBinaryLessEq,
	KeyEqEq:        IDXBinaryEqEq,
	KeyGreaterEq:   IDXBinaryGreaterEq,
	KeyGreaterThan: IDXBinaryGreaterThan,
	KeyAnd:         IDXBinaryAnd,
	KeyOr:          IDXBinaryOr,
	KeyAs:          IDXBinaryAs,
	KeyTildePlus:   IDXBinaryTildePlus,
}

var associativeForms = [256]ID{
	KeyPlus: IDXAssociativePlus,
	KeyStar: IDXAssociativeStar,
	KeyAmp:  IDXAssociativeAmp,
	KeyPipe: IDXAssociativePipe,
	KeyHat:  IDXAssociativeHat,
	KeyAnd:  IDXAssociativeAnd,
	KeyOr:   IDXAssociativeOr,
	// TODO: KeyTildePlus?
}

var isOpen = [...]bool{
	KeyOpenParen:   true,
	KeyOpenBracket: true,
	KeyOpenCurly:   true,
}

var isClose = [...]bool{
	KeyCloseParen:   true,
	KeyCloseBracket: true,
	KeyCloseCurly:   true,
}

var isTightLeft = [...]bool{
	KeyCloseParen:   true,
	KeyOpenBracket:  true,
	KeyCloseBracket: true,

	KeyDot:       true,
	KeyDotDot:    true,
	KeyComma:     true,
	KeyExclam:    true,
	KeyQuestion:  true,
	KeyColon:     true,
	KeySemicolon: true,
}

var isTightRight = [...]bool{
	KeyOpenParen:   true,
	KeyOpenBracket: true,

	KeyDot:      true,
	KeyDotDot:   true,
	KeyExclam:   true,
	KeyQuestion: true,
	KeyColon:    true,
	KeyDollar:   true,
}

var isImplicitSemicolon = [...]bool{
	KeyCloseParen:   true,
	KeyCloseBracket: true,
	KeyCloseCurly:   true,

	KeyReturn:   true,
	KeyBreak:    true,
	KeyContinue: true,
}
