// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package ast

import (
	t "github.com/google/puffs/lang/token"
)

// Kind is what kind of node it is. For example, a top-level func or a numeric
// constant. Kind is different from Type; the latter is used for type-checking
// in the programming language sense.
type Kind uint32

// XType is the explicit type, directly from the source code.
//
// MType is the implicit type, deduced for expressions during type checking.

const (
	KInvalid = Kind(iota)

	// KExpr is an expression, such as "i", "+j" or "k + l[m(n, o)].p":
	//  - ID0:   <0|operator|IDOpenParen|IDOpenBracket|IDColon|IDDot>
	//  - ID1:   <0|identifier name|literal>
	//  - LHS:   <nil|KExpr>
	//  - MHS:   <nil|KExpr>
	//  - RHS:   <nil|KExpr|KTypeExpr>
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

	// KAssign is "LHS = RHS" or "LHS op= RHS":
	//  - ID0:   operator
	//  - LHS:   <KExpr>
	//  - RHS:   <KExpr>
	KAssign

	// KVar is "var ID1 LHS" or "var ID1 LHS = RHS":
	//  - ID1:   name
	//  - LHS:   <KTypeExpr>
	//  - RHS:   <nil|KExpr>
	KVar

	// KParam is a "name type" parameter:
	//  - ID1:   name
	//  - LHS:   <KTypeExpr>
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

	// KTypeExpr is a type expression, such as "u32", "pkg.foo", "ptr T" or
	// "[8] T":
	//  - ID0:   <0|package name|IDPtr|IDOpenBracket>
	//  - ID1:   <0|type name>
	//  - LHS:   <nil|KExpr>
	//  - MHS:   <nil|KExpr>
	//  - RHS:   <nil|KExpr|KTypeExpr>
	//
	// An IDPtr ID0 means "ptr RHS". RHS is a KTypeExpr.
	//
	// An IDOpenBracket ID0 means "[LHS] RHS". RHS is a KTypeExpr.
	//
	// Other ID0 values mean a (possibly package-qualified) type like "pkg.foo"
	// or "foo". ID0 is the "pkg" or zero, ID1 is the "foo". Such a type can be
	// refined as "pkg.foo[MHS:RHS]". MHS and RHS are KExpr's, possibly nil.
	// For example, the MHS for "u32[:4096]" is nil.
	KTypeExpr

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
	KTypeExpr: "KTypeExpr",
	KUse:      "KUse",
	KVar:      "KVar",
}

type Flags uint32

const (
	FlagsSuspendible = Flags(0x00000001)
)

type Node struct {
	kind  Kind
	flags Flags

	mType *TypeExpr

	filename string
	line     uint32

	id0   t.ID
	id1   t.ID
	lhs   *Node // Left Hand Side.
	mhs   *Node // Middle Hand Side.
	rhs   *Node // Right Hand Side.
	list0 []*Node
	list1 []*Node
	list2 []*Node
}

func (n *Node) Kind() Kind { return n.kind }

func (n *Node) Assert() *Assert     { return (*Assert)(n) }
func (n *Node) Assign() *Assign     { return (*Assign)(n) }
func (n *Node) Break() *Break       { return (*Break)(n) }
func (n *Node) Continue() *Continue { return (*Continue)(n) }
func (n *Node) Expr() *Expr         { return (*Expr)(n) }
func (n *Node) File() *File         { return (*File)(n) }
func (n *Node) For() *For           { return (*For)(n) }
func (n *Node) Func() *Func         { return (*Func)(n) }
func (n *Node) If() *If             { return (*If)(n) }
func (n *Node) Param() *Param       { return (*Param)(n) }
func (n *Node) Raw() *Raw           { return (*Raw)(n) }
func (n *Node) Return() *Return     { return (*Return)(n) }
func (n *Node) Struct() *Struct     { return (*Struct)(n) }
func (n *Node) TypeExpr() *TypeExpr { return (*TypeExpr)(n) }
func (n *Node) Use() *Use           { return (*Use)(n) }
func (n *Node) Var() *Var           { return (*Var)(n) }

type Raw Node

func (n *Raw) Node() *Node                    { return (*Node)(n) }
func (n *Raw) Kind() Kind                     { return n.kind }
func (n *Raw) Flags() Flags                   { return n.flags }
func (n *Raw) MType() *TypeExpr               { return n.mType }
func (n *Raw) FilenameLine() (string, uint32) { return n.filename, n.line }
func (n *Raw) Filename() string               { return n.filename }
func (n *Raw) Line() uint32                   { return n.line }
func (n *Raw) ID0() t.ID                      { return n.id0 }
func (n *Raw) ID1() t.ID                      { return n.id1 }
func (n *Raw) SubNodes() [3]*Node             { return [3]*Node{n.lhs, n.mhs, n.rhs} }
func (n *Raw) LHS() *Node                     { return n.lhs }
func (n *Raw) MHS() *Node                     { return n.mhs }
func (n *Raw) RHS() *Node                     { return n.rhs }
func (n *Raw) SubLists() [3][]*Node           { return [3][]*Node{n.list0, n.list1, n.list2} }
func (n *Raw) List0() []*Node                 { return n.list0 }
func (n *Raw) List1() []*Node                 { return n.list1 }
func (n *Raw) List2() []*Node                 { return n.list2 }

func (n *Raw) SetFilenameLine(f string, l uint32) { n.filename, n.line = f, l }

type Expr Node

func (n *Expr) Node() *Node      { return (*Node)(n) }
func (n *Expr) MType() *TypeExpr { return n.mType }
func (n *Expr) ID0() t.ID        { return n.id0 }
func (n *Expr) ID1() t.ID        { return n.id1 }
func (n *Expr) LHS() *Node       { return n.lhs }
func (n *Expr) MHS() *Node       { return n.mhs }
func (n *Expr) RHS() *Node       { return n.rhs }
func (n *Expr) List() []*Node    { return n.list0 }

func (n *Expr) SetMType(x *TypeExpr) { n.mType = x }

func NewExpr(flags Flags, operator t.ID, nameLiteralSelector t.ID, lhs *Node, mhs *Node, rhs *Node, args []*Node) *Expr {
	return &Expr{
		kind:  KExpr,
		flags: flags,
		id0:   operator,
		id1:   nameLiteralSelector,
		lhs:   lhs,
		mhs:   mhs,
		rhs:   rhs,
		list0: args,
	}
}

type Assert Node

func (n *Assert) Node() *Node      { return (*Node)(n) }
func (n *Assert) Condition() *Expr { return n.rhs.Expr() }

func NewAssert(condition *Expr) *Assert {
	return &Assert{
		kind: KAssert,
		lhs:  condition.Node(),
	}
}

type Assign Node

func (n *Assign) Node() *Node    { return (*Node)(n) }
func (n *Assign) Operator() t.ID { return n.id0 }
func (n *Assign) LHS() *Expr     { return n.lhs.Expr() }
func (n *Assign) RHS() *Expr     { return n.rhs.Expr() }

func NewAssign(operator t.ID, lhs *Expr, rhs *Expr) *Assign {
	return &Assign{
		kind: KAssign,
		id0:  operator,
		lhs:  lhs.Node(),
		rhs:  rhs.Node(),
	}
}

type Var Node

func (n *Var) Node() *Node      { return (*Node)(n) }
func (n *Var) Name() t.ID       { return n.id1 }
func (n *Var) XType() *TypeExpr { return n.lhs.TypeExpr() }
func (n *Var) Value() *Expr     { return n.rhs.Expr() }

func NewVar(name t.ID, xType *TypeExpr, value *Expr) *Var {
	return &Var{
		kind: KVar,
		id1:  name,
		lhs:  xType.Node(),
		rhs:  value.Node(),
	}
}

type Param Node

func (n *Param) Node() *Node      { return (*Node)(n) }
func (n *Param) Name() t.ID       { return n.id1 }
func (n *Param) XType() *TypeExpr { return n.lhs.TypeExpr() }

func NewParam(name t.ID, xType *TypeExpr) *Param {
	return &Param{
		kind: KParam,
		id1:  name,
		lhs:  xType.Node(),
	}
}

type For Node

func (n *For) Node() *Node      { return (*Node)(n) }
func (n *For) Condition() *Expr { return n.lhs.Expr() }
func (n *For) Body() []*Node    { return n.list2 }

func NewFor(condition *Expr, body []*Node) *For {
	return &For{
		kind:  KFor,
		lhs:   condition.Node(),
		list2: body,
	}
}

type If Node

func (n *If) Node() *Node          { return (*Node)(n) }
func (n *If) Condition() *Expr     { return n.lhs.Expr() }
func (n *If) ElseIf() *If          { return n.rhs.If() }
func (n *If) BodyIfTrue() []*Node  { return n.list1 }
func (n *If) BodyIfFalse() []*Node { return n.list2 }

func NewIf(condition *Expr, elseIf *If, bodyIfTrue []*Node, bodyIfFalse []*Node) *If {
	return &If{
		kind:  KIf,
		lhs:   condition.Node(),
		rhs:   elseIf.Node(),
		list1: bodyIfTrue,
		list2: bodyIfFalse,
	}
}

type Return Node

func (n *Return) Node() *Node  { return (*Node)(n) }
func (n *Return) Value() *Expr { return n.lhs.Expr() }

func NewReturn(value *Expr) *Return {
	return &Return{
		kind: KReturn,
		lhs:  value.Node(),
	}
}

type Break Node

func (n *Break) Node() *Node { return (*Node)(n) }

func NewBreak() *Break {
	return &Break{
		kind: KBreak,
	}
}

type Continue Node

func (n *Continue) Node() *Node { return (*Node)(n) }

func NewContinue() *Continue {
	return &Continue{
		kind: KContinue,
	}
}

type TypeExpr Node

func (n *TypeExpr) Node() *Node              { return (*Node)(n) }
func (n *TypeExpr) PackageOrDecorator() t.ID { return n.id0 }
func (n *TypeExpr) Name() t.ID               { return n.id1 }
func (n *TypeExpr) LHS() *Node               { return n.lhs }
func (n *TypeExpr) MHS() *Node               { return n.mhs }
func (n *TypeExpr) RHS() *Node               { return n.rhs }

// TODO: tighten the types here.

func NewTypeExpr(pkgOrDec t.ID, name t.ID, lhs *Node, mhs *Node, rhs *Node) *TypeExpr {
	return &TypeExpr{
		kind: KTypeExpr,
		id0:  pkgOrDec,
		id1:  name,
		lhs:  lhs,
		mhs:  mhs,
		rhs:  rhs,
	}
}

type Func Node

func (n *Func) Node() *Node        { return (*Node)(n) }
func (n *Func) Suspendible() bool  { return n.flags&FlagsSuspendible != 0 }
func (n *Func) Filename() string   { return n.filename }
func (n *Func) Line() uint32       { return n.line }
func (n *Func) QID() t.QID         { return t.QID{n.id0, n.id1} }
func (n *Func) Receiver() t.ID     { return n.id0 }
func (n *Func) Name() t.ID         { return n.id1 }
func (n *Func) InParams() []*Node  { return n.list0 }
func (n *Func) OutParams() []*Node { return n.list1 }
func (n *Func) Body() []*Node      { return n.list2 }

func NewFunc(flags Flags, filename string, line uint32, receiver t.ID, name t.ID, inParams []*Node, outParams []*Node, body []*Node) *Func {
	return &Func{
		kind:     KFunc,
		flags:    flags,
		filename: filename,
		line:     line,
		id0:      receiver,
		id1:      name,
		list0:    inParams,
		list1:    outParams,
		list2:    body,
	}
}

type Struct Node

func (n *Struct) Node() *Node       { return (*Node)(n) }
func (n *Struct) Suspendible() bool { return n.flags&FlagsSuspendible != 0 }
func (n *Struct) Filename() string  { return n.filename }
func (n *Struct) Line() uint32      { return n.line }
func (n *Struct) Name() t.ID        { return n.id1 }
func (n *Struct) Fields() []*Node   { return n.list0 }

func NewStruct(flags Flags, filename string, line uint32, name t.ID, fields []*Node) *Struct {
	return &Struct{
		kind:     KStruct,
		flags:    flags,
		filename: filename,
		line:     line,
		id1:      name,
		list0:    fields,
	}
}

type Use Node

func (n *Use) Node() *Node      { return (*Node)(n) }
func (n *Use) Filename() string { return n.filename }
func (n *Use) Line() uint32     { return n.line }
func (n *Use) Path() t.ID       { return n.id1 }

func NewUse(filename string, line uint32, path t.ID) *Use {
	return &Use{
		kind:     KUse,
		filename: filename,
		line:     line,
		id1:      path,
	}
}

type File Node

func (n *File) Node() *Node            { return (*Node)(n) }
func (n *File) Filename() string       { return n.filename }
func (n *File) TopLevelDecls() []*Node { return n.list0 }

func NewFile(filename string, topLevelDecls []*Node) *File {
	return &File{
		kind:     KFile,
		filename: filename,
		list0:    topLevelDecls,
	}
}
