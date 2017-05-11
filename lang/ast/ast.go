// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package ast

import (
	"math/big"

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

	KAssert
	KAssign
	KBreak
	KContinue
	KExpr
	KFile
	KFor
	KFunc
	KIf
	KParam
	KReturn
	KStruct
	KTypeExpr
	KUse
	KVar
)

func (k Kind) String() string {
	if uint(k) < uint(len(kindStrings)) {
		return kindStrings[k]
	}
	return "KUnknown"
}

var kindStrings = [...]string{
	KInvalid: "KInvalid",

	KAssert:   "KAssert",
	KAssign:   "KAssign",
	KBreak:    "KBreak",
	KContinue: "KContinue",
	KExpr:     "KExpr",
	KFile:     "KFile",
	KFor:      "KFor",
	KFunc:     "KFunc",
	KIf:       "KIf",
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

	constValue *big.Int
	mType      *TypeExpr

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

// Expr is an expression, such as "i", "+j" or "k + l[m(n, o)].p":
//  - FlagsSuspendible is "f(x)" vs "f?(x)"
//  - ID0:   <0|operator|IDOpenParen|IDOpenBracket|IDColon|IDDot>
//  - ID1:   <0|identifier name|literal>
//  - LHS:   <nil|Expr>
//  - MHS:   <nil|Expr>
//  - RHS:   <nil|Expr|TypeExpr>
//  - List0: <Expr> function call arguments or associative op arguments
//
// A zero ID0 means an identifier or literal in ID1, like "foo" or "42".
//
// For unary operators, ID0 is the operator and RHS is the operand.
//
// For binary operators, ID0 is the operator and LHS and RHS are the operands.
//
// For associative operators, ID0 is the operator and List0 holds the operands.
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
type Expr Node

func (n *Expr) Node() *Node          { return (*Node)(n) }
func (n *Expr) ConstValue() *big.Int { return n.constValue }
func (n *Expr) MType() *TypeExpr     { return n.mType }
func (n *Expr) ID0() t.ID            { return n.id0 }
func (n *Expr) ID1() t.ID            { return n.id1 }
func (n *Expr) LHS() *Node           { return n.lhs }
func (n *Expr) MHS() *Node           { return n.mhs }
func (n *Expr) RHS() *Node           { return n.rhs }
func (n *Expr) Args() []*Node        { return n.list0 }

func (n *Expr) SetConstValue(x *big.Int) { n.constValue = x }
func (n *Expr) SetMType(x *TypeExpr)     { n.mType = x }

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

// Assert is "assert RHS":
//  - RHS:   <Expr>
type Assert Node

func (n *Assert) Node() *Node      { return (*Node)(n) }
func (n *Assert) Condition() *Expr { return n.rhs.Expr() }

func NewAssert(condition *Expr) *Assert {
	return &Assert{
		kind: KAssert,
		rhs:  condition.Node(),
	}
}

// Assign is "LHS = RHS" or "LHS op= RHS":
//  - ID0:   operator
//  - LHS:   <Expr>
//  - RHS:   <Expr>
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

// Var is "var ID1 LHS" or "var ID1 LHS = RHS":
//  - ID1:   name
//  - LHS:   <TypeExpr>
//  - RHS:   <nil|Expr>
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

// Param is a "name type" parameter:
//  - ID1:   name
//  - LHS:   <TypeExpr>
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

// For is "for { List2 }" or "for LHS { List2 }":
//  - LHS:   <nil|Expr>
//  - List2: <*> loop body
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

// If is "if LHS { List1 } else RHS" or "if LHS { List1 } else { List2 }":
//  - LHS:   <Expr>
//  - RHS:   <nil|If>
//  - List1: <*> if-true body
//  - List2: <*> if-false body
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

// Return is "return LHS":
//  - LHS:   <nil|Expr>
type Return Node

func (n *Return) Node() *Node  { return (*Node)(n) }
func (n *Return) Value() *Expr { return n.lhs.Expr() }

func NewReturn(value *Expr) *Return {
	return &Return{
		kind: KReturn,
		lhs:  value.Node(),
	}
}

// Break is "break":
// TODO: label?
type Break Node

func (n *Break) Node() *Node { return (*Node)(n) }

func NewBreak() *Break {
	return &Break{
		kind: KBreak,
	}
}

// Continue is "continue":
// TODO: label?
type Continue Node

func (n *Continue) Node() *Node { return (*Node)(n) }

func NewContinue() *Continue {
	return &Continue{
		kind: KContinue,
	}
}

// TypeExpr is a type expression, such as "u32", "pkg.foo", "ptr T" or "[8] T":
//  - ID0:   <0|package name|IDPtr|IDOpenBracket>
//  - ID1:   <0|type name>
//  - LHS:   <nil|Expr>
//  - MHS:   <nil|Expr>
//  - RHS:   <nil|TypeExpr>
//
// An IDPtr ID0 means "ptr RHS". RHS is the inner type.
//
// An IDOpenBracket ID0 means "[LHS] RHS". RHS is the inner type.
//
// Other ID0 values mean a (possibly package-qualified) type like "pkg.foo" or
// "foo". ID0 is the "pkg" or zero, ID1 is the "foo". Such a type can be
// refined as "foo[LHS:MHS]". LHS and MHS are Expr's, possibly nil. For
// example, the MHS for "u32[:4096]" is nil.
type TypeExpr Node

func (n *TypeExpr) Node() *Node              { return (*Node)(n) }
func (n *TypeExpr) PackageOrDecorator() t.ID { return n.id0 }
func (n *TypeExpr) Name() t.ID               { return n.id1 }
func (n *TypeExpr) ArrayLength() *Expr       { return n.lhs.Expr() }
func (n *TypeExpr) Bounds() [2]*Expr         { return [2]*Expr{n.lhs.Expr(), n.mhs.Expr()} }
func (n *TypeExpr) InclMin() *Expr           { return n.lhs.Expr() }
func (n *TypeExpr) ExclMax() *Expr           { return n.mhs.Expr() }
func (n *TypeExpr) Inner() *TypeExpr         { return n.rhs.TypeExpr() }

func NewTypeExpr(pkgOrDec t.ID, name t.ID, arrayLengthInclMin *Expr, exclMax *Expr, inner *TypeExpr) *TypeExpr {
	return &TypeExpr{
		kind: KTypeExpr,
		id0:  pkgOrDec,
		id1:  name,
		lhs:  arrayLengthInclMin.Node(),
		mhs:  exclMax.Node(),
		rhs:  inner.Node(),
	}
}

// Func is "func ID0.ID1(List0) (List1) { List2 }":
//  - FlagsSuspendible is "ID1" vs "ID1?"
//  - ID0:   <0|receiver>
//  - ID1:   name
//  - List0: <Param> in-parameters
//  - List1: <Param> out-parameters
//  - List2: <*> function body
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

// Struct is "struct ID1(List0)":
//  - FlagsSuspendible is "ID1" vs "ID1?"
//  - ID1:   name
//  - List0: <Param> fields
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

// Use is "use ID1":
//  - ID1:   <string literal> package path
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

// File is a file of source code:
//  - List0: <Func|Struct|Use> top-level declarations
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
