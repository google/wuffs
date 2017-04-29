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

	// KLiteral is a literal value.
	//	- ID0:   value
	KLiteral

	// KIdent is an identifier.
	//	- ID0:   name
	KIdent

	// KAssign is "Left = Right".
	//	- LHS:   Left
	//	- RHS:   Right
	KAssign

	// KParam is a "name package.type" parameter:
	//	- ID0:   name
	//	- ID1:   package (optional)
	//	- ID2:   type
	KParam

	// KFunc is "func ID0.ID1 (List0) (List1) { List2 }":
	//	- ID0:   receiver (optional)
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
	KFile:    "KFile",
	KFunc:    "KFunc",
	KIdent:   "KIdent",
	KInvalid: "KInvalid",
	KLiteral: "KLiteral",
	KParam:   "KParam",
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
