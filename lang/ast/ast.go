// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package ast

import (
	"github.com/google/puffs/lang/token"
)

// Kind is what kind of node it is. For example, a top-level func or a numeric
// constant. Kind is different from Type; the latter is used for type-checking
// in the programming language sense.
type Kind uint32

const (
	KInvalid = Kind(iota)

	// KExpr is an expression, such as "i", "+j" or "k + l[m(n, o)].p":
	//  - ID0:   <0|operator|IDOpenParen|IDOpenBracket|IDColon|IDDot>
	//  - ID1:   <0|identifier name|literal>
	//  - LHS:   <nil|KExpr>
	//  - MHS:   <nil|KExpr>
	//  - RHS:   <nil|KExpr|KType>
	//  - List0: <KExpr> function call arguments or associative op arguments
	//  - FlagsSuspendible is "f(x)" vs "f?(x)"
	//
	// A zero ID0 means an identifier or literal in ID1, like "foo" or "42".
	//
	// For unary operators, ID0 is the operator and RHS is the operand.
	//
	// For binary operators, ID0 is the operator and LHS and RHS are the
	// operands.
	//
	// For associative operators, ID0 is the operator and List0 holds the
	// operands.
	//
	// The ID0 operator is in disambiguous form. For example, IDXUnaryPlus,
	// IDXBinaryPlus or IDXAssociativePlus, not a bare IDPlus.
	//
	// For function calls, like "LHS(List0)", ID0 is IDOpenParen.
	//
	// For indexes, like "LHS[RHS]", ID0 is IDOpenBracket.
	//
	// For slices, like "LHS[MHS:RHS]", ID0 is IDColon.
	//
	// For selectors, like "LHS.ID1", ID0 is IDDot.
	KExpr

	// KAssert is "assert RHS":
	//  - RHS:   <KExpr>
	KAssert

	// KAssign is "LHS = RHS" or "LHS op= RHS:
	//  - ID0:   operator
	//  - LHS:   <KExpr>
	//  - RHS:   <KExpr>
	KAssign

	// KParam is a "name type" parameter:
	//  - ID0:   name
	//  - RHS:   <KType>
	KParam

	// KFor is "for { List2 }" or "for LHS { List2 }":
	//  - LHS:   <nil|KExpr>
	//  - List2: <*> loop body
	KFor

	// KIf is "if LHS { List1 } else RHS" or "if LHS { List1 } else { List2 }":
	//  - LHS:   <KExpr>
	//  - RHS:   <nil|KIf>
	//  - List1: <*> if-true body
	//  - List2: <*> if-false body
	KIf

	// KReturn is "return LHS":
	//  - LHS:   <nil|KExpr>
	KReturn

	// KBreak is "break":
	// TODO: label?
	KBreak

	// KContinue is "continue":
	// TODO: label?
	KContinue

	// KType is a type, such as "u32", "pkg.foo", "ptr T" or "[8] T":
	//  - ID0:   <0|package name|IDPtr>
	//  - ID1:   <0|type name>
	//  - LHS:   <nil|KExpr>
	//  - MHS:   <nil|KExpr>
	//  - RHS:   <nil|KExpr|KType>
	//
	// An IDPtr ID0 means "ptr RHS". RHS is a KType.
	//
	// An IDOpenBracket ID0 means "[LHS] RHS". RHS is a KType.
	//
	// Other ID0 values mean a (possibly package-qualified) type like "pkg.foo"
	// or "foo". ID0 is the "pkg" or zero, ID1 is the "foo". Such a type can be
	// refined as "pkg.foo[MHS:RHS]". MHS and RHS are KExpr's, possibly nil.
	// For example, the MHS for "u32[:4096]" is nil.
	KType

	// KFunc is "func ID0.ID1(List0) (List1) { List2 }":
	//  - ID0:   <0|receiver>
	//  - ID1:   name
	//  - List0: <KParam> in-parameters
	//  - List1: <KParam> out-parameters
	//  - List2: <*> function body
	//  - FlagsSuspendible is "ID1" vs "ID1?"
	KFunc

	// KStruct is "struct ID1(List0)":
	//  - ID1:   name
	//  - List0: <KParam> fields
	//  - FlagsSuspendible is "ID1" vs "ID1?"
	KStruct

	// KUse is "use ID1":
	//  - ID1:   <string literal> package path
	KUse

	// KFile is a file of source code:
	//  - List0: <KFunc|KStruct|KUse> top-level declarations
	KFile
)

func (k Kind) String() string {
	if uint(k) < uint(len(kindStrings)) {
		return kindStrings[k]
	}
	return "KUnknown"
}

var kindStrings = [...]string{
	KAssert:   "KAssert",
	KAssign:   "KAssign",
	KBreak:    "KBreak",
	KContinue: "KContinue",
	KExpr:     "KExpr",
	KFile:     "KFile",
	KFor:      "KFor",
	KFunc:     "KFunc",
	KIf:       "KIf",
	KInvalid:  "KInvalid",
	KParam:    "KParam",
	KReturn:   "KReturn",
	KStruct:   "KStruct",
	KType:     "KType",
	KUse:      "KUse",
}

type Flags uint32

const (
	FlagsSuspendible = Flags(0x00000001)
)

type Node struct {
	Kind  Kind
	Flags Flags

	Type *Node

	Filename string
	Line     uint32

	ID0   token.ID
	ID1   token.ID
	LHS   *Node // Left Hand Side.
	MHS   *Node // Middle Hand Side.
	RHS   *Node // Right Hand Side.
	List0 []*Node
	List1 []*Node
	List2 []*Node
}
