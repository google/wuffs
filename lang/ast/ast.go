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
	//  - ID0:   <0|operator|IDOpenParen|IDOpenBracket|IDDot>
	//  - ID1:   <0|identifier name|literal>
	//  - LHS:   <nil|KExpr>
	//  - RHS:   <nil|KExpr|KType>
	//  - List0: <KExpr> function call arguments
	//
	// A zero ID0 means an identifier or literal in ID1, like "foo" or "42".
	//
	// For unary operators, ID0 is the operator and RHS is the operand.
	//
	// For binary operators, ID0 is the operator and LHS and RHS are the
	// operands. The LHS may be null for the "-" and "+" operators, which are
	// also unary operators.
	//
	// For function calls, like "lhs(list0)", ID0 is IDOpenParen.
	//
	// For indexes, like "lhs[rhs]", ID0 is IDOpenBracket.
	//
	// For selectors, like "lhs.id1", ID0 is IDDot.
	KExpr

	// KAssert is "assert RHS":
	//  - RHS:   <KExpr>
	KAssert

	// KAssign is "LHS = RHS":
	//  - LHS:   <KExpr>
	//  - RHS:   <KExpr>
	KAssign

	// KParam is a "name type" parameter:
	//  - ID0:   name
	//  - RHS:   <KType>
	KParam

	// KType is a type, such as "u32", "pkg.foo", "ptr T" or "[8] T":
	//  - ID0:   <0|package name|IDPtr>
	//  - ID1:   <0|type name>
	//  - LHS:   <nil|KExpr>
	//  - RHS:   <nil|KExpr|KType>
	//
	// An IDPtr ID0 means "ptr RHS". RHS is a KType.
	//
	// An IDOpenBracket ID0 means "[LHS] RHS". RHS is a KType.
	//
	// Other ID0 values mean a (possibly package-qualified) type like "pkg.foo"
	// or "foo". ID0 is the "pkg" or zero, ID1 is the "foo". Such a type can be
	// refined as "pkg.foo[LHS:RHS]". LHS and RHS are KExpr's, possibly nil.
	// For example, the LHS for "u32[:4096]" is nil.
	KType

	// KFunc is "func ID0.ID1 (List0) (List1) { List2 }":
	//  - ID0:   <0|receiver>
	//  - ID1:   name
	//  - List0: <KParam> in-parameters
	//  - List1: <KParam> out-parameters
	//  - List2: <KAssign> function body
	//  - FlagsSuspendible is (List1) vs ?(List1)
	KFunc

	// KFile is a file of source code:
	//  - List0: <KFunc> top-level declarations
	KFile
)

func (k Kind) String() string {
	if uint(k) < uint(len(kindStrings)) {
		return kindStrings[k]
	}
	return "KUnknown"
}

var kindStrings = [...]string{
	KAssert:  "KAssert",
	KAssign:  "KAssign",
	KExpr:    "KExpr",
	KFile:    "KFile",
	KFunc:    "KFunc",
	KInvalid: "KInvalid",
	KParam:   "KParam",
	KType:    "KType",
}

type Flags uint32

const (
	FlagsSuspendible = Flags(0x00000001)
)

type Node struct {
	Kind  Kind
	Flags Flags
	ID0   token.ID
	ID1   token.ID
	LHS   *Node // Left Hand Side.
	RHS   *Node // Right Hand Side.
	List0 []*Node
	List1 []*Node
	List2 []*Node
}
