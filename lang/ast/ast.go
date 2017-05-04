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

	// KExpr is an expression, such as "i", "+j" or "k + l[m].n".
	//	- ID0:   <0|operator|IDOpenBracket|IDDot>
	//	- ID2:   <0|identifier name|literal>
	//	- LHS:   <nil|KExpr>
	//	- RHS:   <nil|KExpr|KType>
	//
	// A zero ID0 means an identifier or literal in ID2, like "foo" or "42".
	//
	// For unary operators, ID0 is the operator and RHS is the operand.
	//
	// For binary operators, ID0 is the operator and LHS and RHS are the
	// operands. The LHS may be null for the "-" and "+" operators, which are
	// also unary operators.
	//
	// For function calls, like "lhs(rhs)", ID0 is IDOpenParen.
	//
	// For indexes, like "lhs[rhs]", ID0 is IDOpenBracket.
	//
	// For selectors, like "lhs.rhs", ID0 is IDDot.
	KExpr

	// KAssign is "Left = Right".
	//	- LHS:   Left
	//	- RHS:   Right
	KAssign

	// KParam is a "name type" parameter:
	//	- ID0:   name
	//	- RHS:   <KType>
	KParam

	// KType is a type, such as "u32", "pkg.foo", "ptr T" or "[8] T".
	//	- ID0:   <0|IDPtr>
	//	- ID1:   <0|package name>
	//	- ID2:   <0|type name>
	//	- LHS:   TODO.
	//	- RHS:   <nil|KType>
	//
	// A zero ID0 means an undecorated type like "u32" or "pkg.foo". ID1 is
	// zero or "pkg". ID2 is "u32" or "foo". RHS is nil.
	//
	// A non-zero ID0 means a decorated type like "ptr T" or "[8] T". ID0 is
	// the decoration. LHS is nil or "8". RHS is the T. ID1 and ID2 are zero.
	//
	// TODO: LHS for arrays (and slices?), such as "[8] T".
	// TODO: refinements, such as "u32~[:4096]".
	KType

	// KFunc is "func ID0.ID1 (List0) (List1) { List2 }":
	//	- ID0:   <0|receiver>
	//	- ID1:   name
	//	- List0: <KParam> in-parameters
	//	- List1: <KParam> out-parameters
	//	- List2: <KAssign> function body
	//	- FlagsSuspendible is (List1) vs ?(List1)
	KFunc

	// KFile is a file of source code.
	//	- List0: <KFunc> top-level declarations
	KFile
)

func (k Kind) String() string {
	if uint(k) < uint(len(kindStrings)) {
		return kindStrings[k]
	}
	return "KUnknown"
}

var kindStrings = [...]string{
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
	ID2   token.ID
	LHS   *Node // Left Hand Side.
	RHS   *Node // Right Hand Side.
	List0 []*Node
	List1 []*Node
	List2 []*Node
}
