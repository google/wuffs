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

package ast

import (
	"fmt"
	"math/big"

	t "github.com/google/wuffs/lang/token"
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

	KArg
	KAssert
	KAssign
	KConst
	KExpr
	KField
	KFile
	KFunc
	KIOBind
	KIf
	KIterate
	KJump
	KPackageID
	KRet
	KStatus
	KStruct
	KTypeExpr
	KUse
	KVar
	KWhile
)

func (k Kind) String() string {
	if uint(k) < uint(len(kindStrings)) {
		return kindStrings[k]
	}
	return "KUnknown"
}

var kindStrings = [...]string{
	KInvalid: "KInvalid",

	KArg:       "KArg",
	KAssert:    "KAssert",
	KAssign:    "KAssign",
	KConst:     "KConst",
	KExpr:      "KExpr",
	KField:     "KField",
	KFile:      "KFile",
	KFunc:      "KFunc",
	KIOBind:    "KIOBind",
	KIf:        "KIf",
	KIterate:   "KIterate",
	KJump:      "KJump",
	KPackageID: "KPackageID",
	KRet:       "KRet",
	KStatus:    "KStatus",
	KStruct:    "KStruct",
	KTypeExpr:  "KTypeExpr",
	KUse:       "KUse",
	KVar:       "KVar",
	KWhile:     "KWhile",
}

type Flags uint32

const (
	FlagsImpure          = Flags(0x00000001)
	FlagsSuspendible     = Flags(0x00000002)
	FlagsCallImpure      = Flags(0x00000004)
	FlagsCallSuspendible = Flags(0x00000008)
	FlagsPublic          = Flags(0x00000010)
	FlagsTypeChecked     = Flags(0x00000020)
	FlagsHasBreak        = Flags(0x00000040)
	FlagsHasContinue     = Flags(0x00000080)
	FlagsGlobalIdent     = Flags(0x00000100)
)

const (
	flagEffect = FlagsImpure | FlagsSuspendible

	// flagsThatMatterForEq is the bitwise or of all flags that matter for the
	// Expr.Eq method.
	flagsThatMatterForEq = Flags(0x0000FFFF)
)

// These flags are set by the bounds checker to generate optimized code.
const (
	// FlagsProvenNotToSuspend notes that a method such as read_u8 or
	// write_u16le is proven not to suspend. For example, if it's guaranteed
	// that there enough buffer space for an I/O method to succeed.
	//
	// Some methods, such as unread_u8, are called with the question mark, as
	// "r.unread_u8?()", so nominally can suspend, but their semantics also
	// guarantee to either succeed or fail, never suspend and retry later.
	FlagsProvenNotToSuspend = Flags(0x00010000)
	// FlagsBoundsCheckOptimized similarly means that some code gen's run time
	// bounds checks or null pointer checks can be skipped, although such
	// methods are not called with the question mark. Specifically, those
	// methods are:
	//  - since_mark
	FlagsBoundsCheckOptimized = Flags(0x00020000)
)

type Effect uint32

func (e Effect) String() string {
	switch e {
	case Effect(0):
		return ""
	case Effect(FlagsImpure):
		return "!"
	case Effect(FlagsImpure | FlagsSuspendible):
		return "?"
	}
	return "INVALID_EFFECT"
}

type Node struct {
	kind  Kind
	flags Flags

	constValue *big.Int
	mType      *TypeExpr
	jumpTarget Loop

	filename string
	line     uint32

	// The idX fields' meaning depend on what kind of node it is.
	//
	// kind          id0           id1           id2           kind
	// ------------------------------------------------------------
	// Arg           .             .             name          Arg
	// Assert        keyword       .             lit(reason)   Assert
	// Assign        operator      .             .             Assign
	// Const         .             pkg           name          Const
	// Expr          operator      pkg           literal/ident Expr
	// Field         .             .             name          Field
	// File          .             .             .             File
	// Func          funcName      receiverPkg   receiverName  Func
	// IOBind        .             .             .             IOBind
	// If            .             .             .             If
	// Iterate       unroll        label         length        Iterate
	// Jump          keyword       label         .             Jump
	// PackageID     .             .             lit(pkgID)    PackageID
	// Ret           keyword       .             .             Ret
	// Status        keyword       pkg           lit(message)  Status
	// Struct        .             pkg           name          Struct
	// TypeExpr      decorator     pkg           name          TypeExpr
	// Use           .             .             lit(path)     Use
	// Var           operator      .             name          Var
	// While         .             label         .             While
	id0 t.ID
	id1 t.ID
	id2 t.ID

	lhs *Node // Left Hand Side.
	mhs *Node // Middle Hand Side.
	rhs *Node // Right Hand Side.

	list0 []*Node
	list1 []*Node
	list2 []*Node
}

func (n *Node) Kind() Kind        { return n.kind }
func (n *Node) TypeChecked() bool { return n.flags&FlagsTypeChecked != 0 }
func (n *Node) SetTypeChecked()   { n.flags |= FlagsTypeChecked }

func (n *Node) Arg() *Arg             { return (*Arg)(n) }
func (n *Node) Assert() *Assert       { return (*Assert)(n) }
func (n *Node) Assign() *Assign       { return (*Assign)(n) }
func (n *Node) Const() *Const         { return (*Const)(n) }
func (n *Node) Expr() *Expr           { return (*Expr)(n) }
func (n *Node) Field() *Field         { return (*Field)(n) }
func (n *Node) File() *File           { return (*File)(n) }
func (n *Node) Func() *Func           { return (*Func)(n) }
func (n *Node) IOBind() *IOBind       { return (*IOBind)(n) }
func (n *Node) If() *If               { return (*If)(n) }
func (n *Node) Iterate() *Iterate     { return (*Iterate)(n) }
func (n *Node) Jump() *Jump           { return (*Jump)(n) }
func (n *Node) PackageID() *PackageID { return (*PackageID)(n) }
func (n *Node) Raw() *Raw             { return (*Raw)(n) }
func (n *Node) Ret() *Ret             { return (*Ret)(n) }
func (n *Node) Status() *Status       { return (*Status)(n) }
func (n *Node) Struct() *Struct       { return (*Struct)(n) }
func (n *Node) TypeExpr() *TypeExpr   { return (*TypeExpr)(n) }
func (n *Node) Use() *Use             { return (*Use)(n) }
func (n *Node) Var() *Var             { return (*Var)(n) }
func (n *Node) While() *While         { return (*While)(n) }

func (n *Node) Walk(f func(*Node) error) error {
	if n != nil {
		if err := f(n); err != nil {
			return err
		}
		for _, o := range n.Raw().SubNodes() {
			if err := o.Walk(f); err != nil {
				return err
			}
		}
		for _, l := range n.Raw().SubLists() {
			for _, o := range l {
				if err := o.Walk(f); err != nil {
					return err
				}
			}
		}
	}
	return nil
}

type Loop interface {
	Node() *Node
	HasBreak() bool
	HasContinue() bool
	Label() t.ID
	Asserts() []*Node
	Body() []*Node
	SetHasBreak()
	SetHasContinue()
}

type Raw Node

func (n *Raw) Node() *Node                    { return (*Node)(n) }
func (n *Raw) Flags() Flags                   { return n.flags }
func (n *Raw) FilenameLine() (string, uint32) { return n.filename, n.line }
func (n *Raw) SubNodes() [3]*Node             { return [3]*Node{n.lhs, n.mhs, n.rhs} }
func (n *Raw) SubLists() [3][]*Node           { return [3][]*Node{n.list0, n.list1, n.list2} }

func (n *Raw) SetFilenameLine(f string, l uint32) { n.filename, n.line = f, l }

func (n *Raw) SetPackage(tm *t.Map, pkg t.ID) error {
	return n.Node().Walk(func(o *Node) error {
		switch o.Kind() {
		default:
			return nil

		case KConst, KFunc, KStatus, KStruct:
			// No-op.

		case KExpr:
			switch o.id0 {
			default:
				return nil
			case t.IDError, t.IDStatus, t.IDSuspension:
				// No-op.
			}

		case KTypeExpr:
			if o.id0 != 0 {
				return nil
			}
		}

		if o.id1 == t.IDBase {
			return nil
		}
		if o.id1 != 0 {
			return fmt.Errorf(`invalid SetPackage(%q) call: old package: got %q, want ""`,
				pkg.Str(tm), o.id1.Str(tm))
		}
		o.id1 = pkg
		return nil
	})
}

// MaxExprDepth is an advisory limit for an Expr's recursion depth.
const MaxExprDepth = 255

// Expr is an expression, such as "i", "+j" or "k + l[m(n, o)].p":
//  - FlagsImpure          is if it or a sub-expr is FlagsCallImpure
//  - FlagsSuspendible     is if it or a sub-expr is FlagsCallSuspendible
//  - FlagsCallImpure      is "f(x)" vs "f!(x)"
//  - FlagsCallSuspendible is "f(x)" vs "f?(x)", it implies FlagsCallImpure
//  - ID0:   <0|operator|IDOpenParen|IDOpenBracket|IDColon|IDDot>
//  - ID1:   <0|pkg> (for statuses)
//  - ID2:   <0|literal|ident>
//  - LHS:   <nil|Expr>
//  - MHS:   <nil|Expr>
//  - RHS:   <nil|Expr|TypeExpr>
//  - List0: <Arg|Expr> function call args, assoc. op args or list members.
//
// A zero ID0 means an identifier or literal in ID2, like "foo" or "42".
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
// For try/function calls, like "try LHS(List0)", ID0 is IDTry.
//
// For indexes, like "LHS[RHS]", ID0 is IDOpenBracket.
//
// For slices, like "LHS[MHS:RHS]", ID0 is IDColon.
//
// For selectors, like "LHS.ID2", ID0 is IDDot.
//
// For lists, like "$(0, 1, 2)", ID0 is IDDollar.
//
// For statuses, like `error "foo"` and `suspension bar."baz"`, ID0 is the
// keyword, ID1 is the package and ID2 is the message.
type Expr Node

func (n *Expr) Node() *Node                { return (*Node)(n) }
func (n *Expr) Effect() Effect             { return Effect(n.flags & flagEffect) }
func (n *Expr) Pure() bool                 { return n.flags&FlagsImpure == 0 }
func (n *Expr) Impure() bool               { return n.flags&FlagsImpure != 0 }
func (n *Expr) Suspendible() bool          { return n.flags&FlagsSuspendible != 0 }
func (n *Expr) CallImpure() bool           { return n.flags&FlagsCallImpure != 0 }
func (n *Expr) CallSuspendible() bool      { return n.flags&FlagsCallSuspendible != 0 }
func (n *Expr) GlobalIdent() bool          { return n.flags&FlagsGlobalIdent != 0 }
func (n *Expr) ProvenNotToSuspend() bool   { return n.flags&FlagsProvenNotToSuspend != 0 }
func (n *Expr) BoundsCheckOptimized() bool { return n.flags&FlagsBoundsCheckOptimized != 0 }
func (n *Expr) ConstValue() *big.Int       { return n.constValue }
func (n *Expr) MType() *TypeExpr           { return n.mType }
func (n *Expr) Operator() t.ID             { return n.id0 }
func (n *Expr) StatusQID() t.QID           { return t.QID{n.id1, n.id2} }
func (n *Expr) Ident() t.ID                { return n.id2 }
func (n *Expr) LHS() *Node                 { return n.lhs }
func (n *Expr) MHS() *Node                 { return n.mhs }
func (n *Expr) RHS() *Node                 { return n.rhs }
func (n *Expr) Args() []*Node              { return n.list0 }

func (n *Expr) SetBoundsCheckOptimized() { n.flags |= FlagsBoundsCheckOptimized }
func (n *Expr) SetConstValue(x *big.Int) { n.constValue = x }
func (n *Expr) SetGlobalIdent()          { n.flags |= FlagsGlobalIdent }
func (n *Expr) SetMType(x *TypeExpr)     { n.mType = x }
func (n *Expr) SetProvenNotToSuspend()   { n.flags |= FlagsProvenNotToSuspend }

func NewExpr(flags Flags, operator t.ID, statusPkg t.ID, ident t.ID, lhs *Node, mhs *Node, rhs *Node, args []*Node) *Expr {
	if lhs != nil {
		flags |= lhs.flags & (FlagsImpure | FlagsSuspendible)
	}
	if mhs != nil {
		flags |= mhs.flags & (FlagsImpure | FlagsSuspendible)
	}
	if rhs != nil {
		flags |= rhs.flags & (FlagsImpure | FlagsSuspendible)
	}
	for _, o := range args {
		flags |= o.flags & (FlagsImpure | FlagsSuspendible)
	}

	return &Expr{
		kind:  KExpr,
		flags: flags,
		id0:   operator,
		id1:   statusPkg,
		id2:   ident,
		lhs:   lhs,
		mhs:   mhs,
		rhs:   rhs,
		list0: args,
	}
}

// Assert is "assert RHS via ID2(args)", "pre etc", "inv etc" or "post etc":
//  - ID0:   <IDAssert|IDPre|IDInv|IDPost>
//  - ID2:   <string literal> reason
//  - RHS:   <Expr>
//  - List0: <Arg> reason arguments
type Assert Node

func (n *Assert) Node() *Node      { return (*Node)(n) }
func (n *Assert) Keyword() t.ID    { return n.id0 }
func (n *Assert) Reason() t.ID     { return n.id2 }
func (n *Assert) Condition() *Expr { return n.rhs.Expr() }
func (n *Assert) Args() []*Node    { return n.list0 }

func NewAssert(keyword t.ID, condition *Expr, reason t.ID, args []*Node) *Assert {
	return &Assert{
		kind:  KAssert,
		id0:   keyword,
		id2:   reason,
		rhs:   condition.Node(),
		list0: args,
	}
}

// Arg is "name:value".
//  - ID2:   <ident> name
//  - RHS:   <Expr> value
type Arg Node

func (n *Arg) Node() *Node  { return (*Node)(n) }
func (n *Arg) Name() t.ID   { return n.id2 }
func (n *Arg) Value() *Expr { return n.rhs.Expr() }

func NewArg(name t.ID, value *Expr) *Arg {
	return &Arg{
		kind: KArg,
		id2:  name,
		rhs:  value.Node(),
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

// Var is "var ID2 LHS" or "var ID2 LHS = RHS" or an iterate variable
// declaration "ID1 LHS =: RHS":
//  - ID0:   <0|IDEq|IDEqColon>
//  - ID2:   name
//  - LHS:   <TypeExpr>
//  - RHS:   <nil|Expr>
type Var Node

func (n *Var) Node() *Node           { return (*Node)(n) }
func (n *Var) IterateVariable() bool { return n.id0 == t.IDEqColon }
func (n *Var) Name() t.ID            { return n.id2 }
func (n *Var) XType() *TypeExpr      { return n.lhs.TypeExpr() }
func (n *Var) Value() *Expr          { return n.rhs.Expr() }

func NewVar(op t.ID, name t.ID, xType *TypeExpr, value *Expr) *Var {
	return &Var{
		kind: KVar,
		id0:  op,
		id2:  name,
		lhs:  xType.Node(),
		rhs:  value.Node(),
	}
}

// Field is a "name type = default_value" struct field:
//  - ID2:   name
//  - LHS:   <TypeExpr>
//  - RHS:   <nil|Expr>
type Field Node

func (n *Field) Node() *Node         { return (*Node)(n) }
func (n *Field) Name() t.ID          { return n.id2 }
func (n *Field) XType() *TypeExpr    { return n.lhs.TypeExpr() }
func (n *Field) DefaultValue() *Expr { return n.rhs.Expr() }

func NewField(name t.ID, xType *TypeExpr, defaultValue *Expr) *Field {
	return &Field{
		kind: KField,
		id2:  name,
		lhs:  xType.Node(),
		rhs:  defaultValue.Node(),
	}
}

// IOBind is "io_bind (in_fields) { List2 }":
//  - List0: <Expr> in.something fields
//  - List2: <Statement> body
type IOBind Node

func (n *IOBind) Node() *Node       { return (*Node)(n) }
func (n *IOBind) InFields() []*Node { return n.list0 }
func (n *IOBind) Body() []*Node     { return n.list2 }

func NewIOBind(in_fields []*Node, body []*Node) *IOBind {
	return &IOBind{
		kind:  KIOBind,
		list0: in_fields,
		list2: body,
	}
}

// Iterate is
// "iterate:ID1 (vars)(length:ID2, unroll:ID0), List1 { List2 } else RHS":
//  - FlagsHasBreak    is the iterate has an explicit break
//  - FlagsHasContinue is the iterate has an explicit continue
//  - ID0:   unroll
//  - ID1:   <0|label>
//  - ID2:   length
//  - RHS:   <nil|Iterate>
//  - List0: <Var> vars
//  - List1: <Assert> asserts
//  - List2: <Statement> body
type Iterate Node

func (n *Iterate) Node() *Node           { return (*Node)(n) }
func (n *Iterate) HasBreak() bool        { return n.flags&FlagsHasBreak != 0 }
func (n *Iterate) HasContinue() bool     { return n.flags&FlagsHasContinue != 0 }
func (n *Iterate) Unroll() t.ID          { return n.id0 }
func (n *Iterate) Label() t.ID           { return n.id1 }
func (n *Iterate) Length() t.ID          { return n.id2 }
func (n *Iterate) ElseIterate() *Iterate { return n.rhs.Iterate() }
func (n *Iterate) Variables() []*Node    { return n.list0 }
func (n *Iterate) Asserts() []*Node      { return n.list1 }
func (n *Iterate) Body() []*Node         { return n.list2 }

func (n *Iterate) SetHasBreak()    { n.flags |= FlagsHasBreak }
func (n *Iterate) SetHasContinue() { n.flags |= FlagsHasContinue }

func NewIterate(label t.ID, vars []*Node, length t.ID, unroll t.ID, asserts []*Node, body []*Node, elseIterate *Iterate) *Iterate {
	return &Iterate{
		kind:  KIterate,
		id0:   unroll,
		id1:   label,
		id2:   length,
		rhs:   elseIterate.Node(),
		list0: vars,
		list1: asserts,
		list2: body,
	}
}

// While is "while:ID1 MHS, List1 { List2 }":
//  - FlagsHasBreak    is the while has an explicit break
//  - FlagsHasContinue is the while has an explicit continue
//  - ID1:   <0|label>
//  - MHS:   <Expr>
//  - List1: <Assert> asserts
//  - List2: <Statement> body
//
// TODO: should we be able to unroll while loops too?
type While Node

func (n *While) Node() *Node       { return (*Node)(n) }
func (n *While) HasBreak() bool    { return n.flags&FlagsHasBreak != 0 }
func (n *While) HasContinue() bool { return n.flags&FlagsHasContinue != 0 }
func (n *While) Label() t.ID       { return n.id1 }
func (n *While) Condition() *Expr  { return n.mhs.Expr() }
func (n *While) Asserts() []*Node  { return n.list1 }
func (n *While) Body() []*Node     { return n.list2 }

func (n *While) SetHasBreak()    { n.flags |= FlagsHasBreak }
func (n *While) SetHasContinue() { n.flags |= FlagsHasContinue }

func NewWhile(label t.ID, condition *Expr, asserts []*Node, body []*Node) *While {
	return &While{
		kind:  KWhile,
		id1:   label,
		mhs:   condition.Node(),
		list1: asserts,
		list2: body,
	}
}

// If is "if MHS { List2 } else RHS" or "if MHS { List2 } else { List1 }":
//  - MHS:   <Expr>
//  - RHS:   <nil|If>
//  - List1: <Statement> if-false body
//  - List2: <Statement> if-true body
type If Node

func (n *If) Node() *Node          { return (*Node)(n) }
func (n *If) Condition() *Expr     { return n.mhs.Expr() }
func (n *If) ElseIf() *If          { return n.rhs.If() }
func (n *If) BodyIfTrue() []*Node  { return n.list2 }
func (n *If) BodyIfFalse() []*Node { return n.list1 }

func NewIf(condition *Expr, elseIf *If, bodyIfTrue []*Node, bodyIfFalse []*Node) *If {
	return &If{
		kind:  KIf,
		mhs:   condition.Node(),
		rhs:   elseIf.Node(),
		list1: bodyIfFalse,
		list2: bodyIfTrue,
	}
}

// Ret is "return LHS" or "yield LHS":
//  - ID0:   <IDReturn|IDYield>
//  - LHS:   <nil|Expr>
type Ret Node

func (n *Ret) Node() *Node   { return (*Node)(n) }
func (n *Ret) Keyword() t.ID { return n.id0 }
func (n *Ret) Value() *Expr  { return n.lhs.Expr() }

func NewRet(keyword t.ID, value *Expr) *Ret {
	return &Ret{
		kind: KRet,
		id0:  keyword,
		lhs:  value.Node(),
	}
}

// Jump is "break" or "continue", with an optional label, "break:label":
//  - ID0:   <IDBreak|IDContinue>
//  - ID1:   <0|label>
type Jump Node

func (n *Jump) Node() *Node      { return (*Node)(n) }
func (n *Jump) JumpTarget() Loop { return n.jumpTarget }
func (n *Jump) Keyword() t.ID    { return n.id0 }
func (n *Jump) Label() t.ID      { return n.id1 }

func (n *Jump) SetJumpTarget(o Loop) { n.jumpTarget = o }

func NewJump(keyword t.ID, label t.ID) *Jump {
	return &Jump{
		kind: KJump,
		id0:  keyword,
		id1:  label,
	}
}

// MaxTypeExprDepth is an advisory limit for a TypeExpr's recursion depth.
const MaxTypeExprDepth = 63

// TypeExpr is a type expression, such as "base.u32", "base.u32[..8]", "foo",
// "pkg.bar", "ptr T", "array[8] T" or "slice T":
//  - ID0:   <0|IDPtr|IDArray|IDSlice|IDFunc>
//  - ID1:   <0|pkg>
//  - ID2:   <0|type name>
//  - LHS:   <nil|Expr>
//  - MHS:   <nil|Expr>
//  - RHS:   <nil|TypeExpr>
//
// An IDPtr ID0 means "ptr RHS". RHS is the inner type.
//
// An IDArray ID0 means "array[LHS] RHS". RHS is the inner type.
//
// An IDSlice ID0 means "slice RHS". RHS is the inner type.
//
// An IDFunc ID0 means "func ID2" or "func (LHS).ID2", a function or method
// type. LHS is the receiver type, which may be nil. If non-nil, it will be a
// pointee type: "T" instead of "ptr T", "ptr ptr T", etc.
//
// TODO: method effects: "foo" vs "foo!" vs "foo?".
//
// A zero ID0 means a (possibly package-qualified) type like "pkg.foo" or
// "foo". ID1 is the "pkg" or zero, ID2 is the "foo".
//
// Numeric types can be refined as "foo[LHS..MHS]". LHS and MHS are Expr's,
// possibly nil. For example, the LHS for "base.u32[..4095]" is nil.
//
// TODO: struct types, list types, nptr vs ptr, table types.
type TypeExpr Node

func (n *TypeExpr) Node() *Node         { return (*Node)(n) }
func (n *TypeExpr) Decorator() t.ID     { return n.id0 }
func (n *TypeExpr) QID() t.QID          { return t.QID{n.id1, n.id2} }
func (n *TypeExpr) FuncName() t.ID      { return n.id2 }
func (n *TypeExpr) ArrayLength() *Expr  { return n.lhs.Expr() }
func (n *TypeExpr) Receiver() *TypeExpr { return n.lhs.TypeExpr() }
func (n *TypeExpr) Bounds() [2]*Expr    { return [2]*Expr{n.lhs.Expr(), n.mhs.Expr()} }
func (n *TypeExpr) Min() *Expr          { return n.lhs.Expr() }
func (n *TypeExpr) Max() *Expr          { return n.mhs.Expr() }
func (n *TypeExpr) Inner() *TypeExpr    { return n.rhs.TypeExpr() }

func (n *TypeExpr) Innermost() *TypeExpr {
	for ; n != nil && n.Inner() != nil; n = n.Inner() {
	}
	return n
}

func (n *TypeExpr) Pointee() *TypeExpr {
	for ; n != nil && n.id0 == t.IDPtr; n = n.Inner() {
	}
	return n
}

func (n *TypeExpr) IsBool() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2 == t.IDBool
}

func (n *TypeExpr) IsIdeal() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2 == t.IDDoubleZ
}

func (n *TypeExpr) IsNumType() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2.IsNumType()
}

func (n *TypeExpr) IsNumTypeOrIdeal() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2.IsNumTypeOrIdeal()
}

func (n *TypeExpr) IsRefined() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2.IsNumType() && (n.lhs != nil || n.mhs != nil)
}

func (n *TypeExpr) IsArrayType() bool {
	return n.id0 == t.IDArray
}

func (n *TypeExpr) IsSliceType() bool {
	return n.id0 == t.IDSlice
}

func (n *TypeExpr) IsUnsignedInteger() bool {
	return n.id0 == 0 && n.id1 == t.IDBase &&
		(n.id2 == t.IDU8 || n.id2 == t.IDU16 || n.id2 == t.IDU32 || n.id2 == t.IDU64)
}

func (n *TypeExpr) HasPointers() bool {
	for ; n != nil; n = n.Inner() {
		switch n.id0 {
		case 0:
			if n.id1 == t.IDBase {
				switch n.id2 {
				case t.IDIOReader, t.IDIOWriter:
					return true
				}
			}
		case t.IDPtr, t.IDNptr, t.IDSlice:
			return true
		}
	}
	return false
}

func (n *TypeExpr) Unrefined() *TypeExpr {
	if !n.IsRefined() {
		return n
	}
	o := *n
	o.lhs = nil
	o.mhs = nil
	return &o
}

func NewTypeExpr(decorator t.ID, pkg t.ID, name t.ID, alenRecvMin *Node, max *Expr, inner *TypeExpr) *TypeExpr {
	return &TypeExpr{
		kind: KTypeExpr,
		id0:  decorator,
		id1:  pkg,
		id2:  name,
		lhs:  alenRecvMin,
		mhs:  max.Node(),
		rhs:  inner.Node(),
	}
}

// MaxBodyDepth is an advisory limit for a function body's recursion depth.
const MaxBodyDepth = 255

// Func is "func ID2.ID0(LHS)(RHS) { List2 }":
//  - FlagsImpure      is "ID1" vs "ID1!"
//  - FlagsSuspendible is "ID1" vs "ID1?", it implies FlagsImpure
//  - FlagsPublic      is "pub" vs "pri"
//  - ID0:   funcName
//  - ID1:   <0|receiverPkg> (set by calling SetPackage)
//  - ID2:   <0|receiverName>
//  - LHS:   <Struct> in-parameters
//  - RHS:   <Struct> out-parameters
//  - List1: <Assert> asserts
//  - List2: <Statement> body
//
// Statement means one of:
//  - Assert
//  - Assign
//  - Expr
//  - IOBind
//  - If
//  - Iterate
//  - Jump
//  - Ret
//  - Var
//  - While
type Func Node

func (n *Func) Node() *Node       { return (*Node)(n) }
func (n *Func) Effect() Effect    { return Effect(n.flags & flagEffect) }
func (n *Func) Pure() bool        { return n.flags&FlagsImpure == 0 }
func (n *Func) Impure() bool      { return n.flags&FlagsImpure != 0 }
func (n *Func) Suspendible() bool { return n.flags&FlagsSuspendible != 0 }
func (n *Func) Public() bool      { return n.flags&FlagsPublic != 0 }
func (n *Func) Filename() string  { return n.filename }
func (n *Func) Line() uint32      { return n.line }
func (n *Func) QQID() t.QQID      { return t.QQID{n.id1, n.id2, n.id0} }
func (n *Func) Receiver() t.QID   { return t.QID{n.id1, n.id2} }
func (n *Func) FuncName() t.ID    { return n.id0 }
func (n *Func) In() *Struct       { return n.lhs.Struct() }
func (n *Func) Out() *Struct      { return n.rhs.Struct() }
func (n *Func) Asserts() []*Node  { return n.list1 }
func (n *Func) Body() []*Node     { return n.list2 }

func NewFunc(flags Flags, filename string, line uint32, receiverName t.ID, funcName t.ID, in *Struct, out *Struct, asserts []*Node, body []*Node) *Func {
	return &Func{
		kind:     KFunc,
		flags:    flags,
		filename: filename,
		line:     line,
		id0:      funcName,
		id2:      receiverName,
		lhs:      in.Node(),
		rhs:      out.Node(),
		list1:    asserts,
		list2:    body,
	}
}

// Status is "error ID2" or "suspension ID2":
//  - FlagsPublic      is "pub" vs "pri"
//  - ID0:   <IDError|IDSuspension>
//  - ID1:   <0|pkg> (set by calling SetPackage)
//  - ID2:   message
type Status Node

func (n *Status) Node() *Node      { return (*Node)(n) }
func (n *Status) Public() bool     { return n.flags&FlagsPublic != 0 }
func (n *Status) Filename() string { return n.filename }
func (n *Status) Line() uint32     { return n.line }
func (n *Status) Keyword() t.ID    { return n.id0 }
func (n *Status) QID() t.QID       { return t.QID{n.id1, n.id2} }

func NewStatus(flags Flags, filename string, line uint32, keyword t.ID, message t.ID) *Status {
	return &Status{
		kind:     KStatus,
		flags:    flags,
		filename: filename,
		line:     line,
		id0:      keyword,
		id2:      message,
	}
}

// Const is "const ID2 LHS = RHS":
//  - FlagsPublic      is "pub" vs "pri"
//  - ID1:   <0|pkg> (set by calling SetPackage)
//  - ID2:   name
//  - LHS:   <TypeExpr>
//  - RHS:   <Expr>
type Const Node

func (n *Const) Node() *Node      { return (*Node)(n) }
func (n *Const) Public() bool     { return n.flags&FlagsPublic != 0 }
func (n *Const) Filename() string { return n.filename }
func (n *Const) Line() uint32     { return n.line }
func (n *Const) QID() t.QID       { return t.QID{n.id1, n.id2} }
func (n *Const) XType() *TypeExpr { return n.lhs.TypeExpr() }
func (n *Const) Value() *Expr     { return n.rhs.Expr() }

func NewConst(flags Flags, filename string, line uint32, name t.ID, xType *TypeExpr, value *Expr) *Const {
	return &Const{
		kind:     KConst,
		flags:    flags,
		filename: filename,
		line:     line,
		id2:      name,
		lhs:      xType.Node(),
		rhs:      value.Node(),
	}
}

// Struct is "struct ID2(List0)":
//  - FlagsSuspendible is "ID1" vs "ID1?"
//  - FlagsPublic      is "pub" vs "pri"
//  - ID1:   <0|pkg> (set by calling SetPackage)
//  - ID2:   name
//  - List0: <Field> fields
type Struct Node

func (n *Struct) Node() *Node       { return (*Node)(n) }
func (n *Struct) Suspendible() bool { return n.flags&FlagsSuspendible != 0 }
func (n *Struct) Public() bool      { return n.flags&FlagsPublic != 0 }
func (n *Struct) Filename() string  { return n.filename }
func (n *Struct) Line() uint32      { return n.line }
func (n *Struct) QID() t.QID        { return t.QID{n.id1, n.id2} }
func (n *Struct) Fields() []*Node   { return n.list0 }

func NewStruct(flags Flags, filename string, line uint32, name t.ID, fields []*Node) *Struct {
	return &Struct{
		kind:     KStruct,
		flags:    flags,
		filename: filename,
		line:     line,
		id2:      name,
		list0:    fields,
	}
}

// PackageID is "packageid ID2":
//  - ID2:   <string literal> package ID
type PackageID Node

func (n *PackageID) Node() *Node      { return (*Node)(n) }
func (n *PackageID) Filename() string { return n.filename }
func (n *PackageID) Line() uint32     { return n.line }
func (n *PackageID) ID() t.ID         { return n.id2 }

func NewPackageID(filename string, line uint32, id t.ID) *PackageID {
	return &PackageID{
		kind:     KPackageID,
		filename: filename,
		line:     line,
		id2:      id,
	}
}

// Use is "use ID2":
//  - ID2:   <string literal> package path
type Use Node

func (n *Use) Node() *Node      { return (*Node)(n) }
func (n *Use) Filename() string { return n.filename }
func (n *Use) Line() uint32     { return n.line }
func (n *Use) Path() t.ID       { return n.id2 }

func NewUse(filename string, line uint32, path t.ID) *Use {
	return &Use{
		kind:     KUse,
		filename: filename,
		line:     line,
		id2:      path,
	}
}

// File is a file of source code:
//  - List0: <Const|Func|PackageID|Status|Struct|Use> top-level declarations
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
