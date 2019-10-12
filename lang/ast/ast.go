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

	"github.com/google/wuffs/lib/interval"

	t "github.com/google/wuffs/lang/token"
)

// Kind is what kind of node it is. For example, a top-level func or a numeric
// constant. Kind is different from Type; the latter is used for type-checking
// in the programming language sense.
type Kind uint32

// XType is the explicit type, directly from the source code.
//
// MType is the implicit type, deduced for expressions during type checking.
//
// MBounds are the implicit bounds, deduced during bounds checking.

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

	KArg:      "KArg",
	KAssert:   "KAssert",
	KAssign:   "KAssign",
	KConst:    "KConst",
	KExpr:     "KExpr",
	KField:    "KField",
	KFile:     "KFile",
	KFunc:     "KFunc",
	KIOBind:   "KIOBind",
	KIf:       "KIf",
	KIterate:  "KIterate",
	KJump:     "KJump",
	KRet:      "KRet",
	KStatus:   "KStatus",
	KStruct:   "KStruct",
	KTypeExpr: "KTypeExpr",
	KUse:      "KUse",
	KVar:      "KVar",
	KWhile:    "KWhile",
}

type Flags uint32

const (
	// The low 8 bits are reserved. Effect values (Effect is a uint8) share the
	// same values as Flags.

	FlagsPublic           = Flags(0x00000100)
	FlagsHasBreak         = Flags(0x00000200)
	FlagsHasContinue      = Flags(0x00000400)
	FlagsGlobalIdent      = Flags(0x00000800)
	FlagsClassy           = Flags(0x00001000)
	FlagsSubExprHasEffect = Flags(0x00002000)
	FlagsRetsError        = Flags(0x00004000)
	FlagsPrivateData      = Flags(0x00008000)
)

func (f Flags) AsEffect() Effect { return Effect(f) }

type Effect uint8

const (
	EffectPure            = Effect(0)
	EffectImpure          = Effect(effectBitImpure)
	EffectImpureCoroutine = Effect(effectBitImpure | effectBitCoroutine)

	effectBitImpure    = 0x01
	effectBitCoroutine = 0x02

	effectMask = Effect(0xFF)
)

func (e Effect) AsFlags() Flags { return Flags(e) }

func (e Effect) Pure() bool      { return e == 0 }
func (e Effect) Impure() bool    { return e&effectBitImpure != 0 }
func (e Effect) Coroutine() bool { return e&effectBitCoroutine != 0 }

func (e Effect) WeakerThan(o Effect) bool { return e < o }

func (e Effect) String() string {
	switch e & effectMask {
	case EffectPure:
		return ""
	case EffectImpure:
		return "!"
	case EffectImpureCoroutine:
		return "?"
	}
	return "‽INVALID_EFFECT‽"
}

type Node struct {
	kind  Kind
	flags Flags

	constValue *big.Int
	mBounds    interval.IntRange
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

func (n *Node) Kind() Kind                     { return n.kind }
func (n *Node) MBounds() interval.IntRange     { return n.mBounds }
func (n *Node) MType() *TypeExpr               { return n.mType }
func (n *Node) SetMBounds(x interval.IntRange) { n.mBounds = x }
func (n *Node) SetMType(x *TypeExpr)           { n.mType = x }

func (n *Node) AsArg() *Arg           { return (*Arg)(n) }
func (n *Node) AsAssert() *Assert     { return (*Assert)(n) }
func (n *Node) AsAssign() *Assign     { return (*Assign)(n) }
func (n *Node) AsConst() *Const       { return (*Const)(n) }
func (n *Node) AsExpr() *Expr         { return (*Expr)(n) }
func (n *Node) AsField() *Field       { return (*Field)(n) }
func (n *Node) AsFile() *File         { return (*File)(n) }
func (n *Node) AsFunc() *Func         { return (*Func)(n) }
func (n *Node) AsIOBind() *IOBind     { return (*IOBind)(n) }
func (n *Node) AsIf() *If             { return (*If)(n) }
func (n *Node) AsIterate() *Iterate   { return (*Iterate)(n) }
func (n *Node) AsJump() *Jump         { return (*Jump)(n) }
func (n *Node) AsRaw() *Raw           { return (*Raw)(n) }
func (n *Node) AsRet() *Ret           { return (*Ret)(n) }
func (n *Node) AsStatus() *Status     { return (*Status)(n) }
func (n *Node) AsStruct() *Struct     { return (*Struct)(n) }
func (n *Node) AsTypeExpr() *TypeExpr { return (*TypeExpr)(n) }
func (n *Node) AsUse() *Use           { return (*Use)(n) }
func (n *Node) AsVar() *Var           { return (*Var)(n) }
func (n *Node) AsWhile() *While       { return (*While)(n) }

func (n *Node) Walk(f func(*Node) error) error {
	if n != nil {
		if err := f(n); err != nil {
			return err
		}
		for _, o := range n.AsRaw().SubNodes() {
			if err := o.Walk(f); err != nil {
				return err
			}
		}
		for _, l := range n.AsRaw().SubLists() {
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
	AsNode() *Node
	HasBreak() bool
	HasContinue() bool
	Label() t.ID
	Asserts() []*Node
	Body() []*Node
	SetHasBreak()
	SetHasContinue()
}

type Raw Node

func (n *Raw) AsNode() *Node                  { return (*Node)(n) }
func (n *Raw) Flags() Flags                   { return n.flags }
func (n *Raw) FilenameLine() (string, uint32) { return n.filename, n.line }
func (n *Raw) SubNodes() [3]*Node             { return [3]*Node{n.lhs, n.mhs, n.rhs} }
func (n *Raw) SubLists() [3][]*Node           { return [3][]*Node{n.list0, n.list1, n.list2} }

func (n *Raw) SetFilenameLine(f string, l uint32) { n.filename, n.line = f, l }

func (n *Raw) SetPackage(tm *t.Map, pkg t.ID) error {
	return n.AsNode().Walk(func(o *Node) error {
		switch o.Kind() {
		default:
			return nil

		case KConst, KFunc, KStatus, KStruct:
			// No-op.

		case KExpr:
			if o.id0 != t.IDStatus {
				return nil
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
//  - ID0:   <0|operator|IDOpenParen|IDOpenBracket|IDDotDot|IDDot>
//  - ID1:   <0|pkg> (for statuses)
//  - ID2:   <0|literal|ident>
//  - LHS:   <nil|Expr>
//  - MHS:   <nil|Expr>
//  - RHS:   <nil|Expr|TypeExpr>
//  - List0: <Arg|Expr> function call args, assoc. op args or list members.
//
// A zero ID0 means an identifier or literal in ID2, like `foo`, `42` or a
// status literal like `"#foo"` or `pkg."$bar"`. For status literals, ID1 is
// the package.
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
// For slices, like "LHS[MHS .. RHS]", ID0 is IDDotDot.
//
// For selectors, like "LHS.ID2", ID0 is IDDot.
//
// For lists, like "[0, 1, 2]", ID0 is IDComma.
type Expr Node

func (n *Expr) AsNode() *Node              { return (*Node)(n) }
func (n *Expr) Effect() Effect             { return Effect(n.flags) }
func (n *Expr) GlobalIdent() bool          { return n.flags&FlagsGlobalIdent != 0 }
func (n *Expr) SubExprHasEffect() bool     { return n.flags&FlagsSubExprHasEffect != 0 }
func (n *Expr) ConstValue() *big.Int       { return n.constValue }
func (n *Expr) MBounds() interval.IntRange { return n.mBounds }
func (n *Expr) MType() *TypeExpr           { return n.mType }
func (n *Expr) Operator() t.ID             { return n.id0 }
func (n *Expr) StatusQID() t.QID           { return t.QID{n.id1, n.id2} }
func (n *Expr) Ident() t.ID                { return n.id2 }
func (n *Expr) LHS() *Node                 { return n.lhs }
func (n *Expr) MHS() *Node                 { return n.mhs }
func (n *Expr) RHS() *Node                 { return n.rhs }
func (n *Expr) Args() []*Node              { return n.list0 }

func (n *Expr) SetConstValue(x *big.Int)       { n.constValue = x }
func (n *Expr) SetGlobalIdent()                { n.flags |= FlagsGlobalIdent }
func (n *Expr) SetMBounds(x interval.IntRange) { n.mBounds = x }
func (n *Expr) SetMType(x *TypeExpr)           { n.mType = x }

func NewExpr(flags Flags, operator t.ID, statusPkg t.ID, ident t.ID, lhs *Node, mhs *Node, rhs *Node, args []*Node) *Expr {
	subExprEffect := Flags(0)
	if lhs != nil {
		subExprEffect |= lhs.flags & Flags(effectMask)
	}
	if mhs != nil {
		subExprEffect |= mhs.flags & Flags(effectMask)
	}
	if rhs != nil {
		subExprEffect |= rhs.flags & Flags(effectMask)
	}
	for _, o := range args {
		subExprEffect |= o.flags & Flags(effectMask)
	}
	if subExprEffect != 0 {
		flags |= subExprEffect | FlagsSubExprHasEffect
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

func (n *Assert) AsNode() *Node    { return (*Node)(n) }
func (n *Assert) Keyword() t.ID    { return n.id0 }
func (n *Assert) Reason() t.ID     { return n.id2 }
func (n *Assert) Condition() *Expr { return n.rhs.AsExpr() }
func (n *Assert) Args() []*Node    { return n.list0 }

func NewAssert(keyword t.ID, condition *Expr, reason t.ID, args []*Node) *Assert {
	return &Assert{
		kind:  KAssert,
		id0:   keyword,
		id2:   reason,
		rhs:   condition.AsNode(),
		list0: args,
	}
}

// Arg is "name:value".
//  - ID2:   <ident> name
//  - RHS:   <Expr> value
type Arg Node

func (n *Arg) AsNode() *Node { return (*Node)(n) }
func (n *Arg) Name() t.ID    { return n.id2 }
func (n *Arg) Value() *Expr  { return n.rhs.AsExpr() }

func NewArg(name t.ID, value *Expr) *Arg {
	return &Arg{
		kind: KArg,
		id2:  name,
		rhs:  value.AsNode(),
	}
}

// Assign is "LHS = RHS" or "LHS op= RHS" or "RHS":
//  - ID0:   operator
//  - LHS:   <nil|Expr>
//  - RHS:   <Expr>
type Assign Node

func (n *Assign) AsNode() *Node  { return (*Node)(n) }
func (n *Assign) Operator() t.ID { return n.id0 }
func (n *Assign) LHS() *Expr     { return n.lhs.AsExpr() }
func (n *Assign) RHS() *Expr     { return n.rhs.AsExpr() }

func NewAssign(operator t.ID, lhs *Expr, rhs *Expr) *Assign {
	return &Assign{
		kind: KAssign,
		id0:  operator,
		lhs:  lhs.AsNode(),
		rhs:  rhs.AsNode(),
	}
}

// Var is "var ID2 LHS":
//  - ID2:   name
//  - LHS:   <TypeExpr>
type Var Node

func (n *Var) AsNode() *Node    { return (*Node)(n) }
func (n *Var) Name() t.ID       { return n.id2 }
func (n *Var) XType() *TypeExpr { return n.lhs.AsTypeExpr() }

func NewVar(name t.ID, xType *TypeExpr) *Var {
	return &Var{
		kind: KVar,
		id2:  name,
		lhs:  xType.AsNode(),
	}
}

// Field is a "name : type" struct field:
//  - FlagsPrivateData is the initializer need not explicitly memset to zero.
//  - ID2:   name
//  - LHS:   <TypeExpr>
type Field Node

func (n *Field) AsNode() *Node     { return (*Node)(n) }
func (n *Field) PrivateData() bool { return n.flags&FlagsPrivateData != 0 }
func (n *Field) Name() t.ID        { return n.id2 }
func (n *Field) XType() *TypeExpr  { return n.lhs.AsTypeExpr() }

func NewField(flags Flags, name t.ID, xType *TypeExpr) *Field {
	return &Field{
		kind:  KField,
		flags: flags,
		id2:   name,
		lhs:   xType.AsNode(),
	}
}

// IOBind is "io_bind (io:LHS, data:MHS) { List2 }" or "io_limit (io:LHS,
// limit:MHS) { List2 }":
//  - ID0:   <IDIOBind|IDIOLimit>
//  - LHS:   <Expr>
//  - MHS:   <Expr>
//  - List2: <Statement> body
type IOBind Node

func (n *IOBind) AsNode() *Node { return (*Node)(n) }
func (n *IOBind) Keyword() t.ID { return n.id0 }
func (n *IOBind) IO() *Expr     { return n.lhs.AsExpr() }
func (n *IOBind) Arg1() *Expr   { return n.mhs.AsExpr() }
func (n *IOBind) Body() []*Node { return n.list2 }

func NewIOBind(keyword t.ID, io *Expr, arg1 *Expr, body []*Node) *IOBind {
	return &IOBind{
		kind:  KIOBind,
		id0:   keyword,
		lhs:   io.AsNode(),
		mhs:   arg1.AsNode(),
		list2: body,
	}
}

// Iterate is
// "iterate.ID1 (assigns)(length:ID2, unroll:ID0), List1 { List2 } else RHS":
//  - FlagsHasBreak    is the iterate has an explicit break
//  - FlagsHasContinue is the iterate has an explicit continue
//  - ID0:   unroll
//  - ID1:   <0|label>
//  - ID2:   length
//  - RHS:   <nil|Iterate>
//  - List0: <Assign> assigns
//  - List1: <Assert> asserts
//  - List2: <Statement> body
type Iterate Node

func (n *Iterate) AsNode() *Node         { return (*Node)(n) }
func (n *Iterate) HasBreak() bool        { return n.flags&FlagsHasBreak != 0 }
func (n *Iterate) HasContinue() bool     { return n.flags&FlagsHasContinue != 0 }
func (n *Iterate) Unroll() t.ID          { return n.id0 }
func (n *Iterate) Label() t.ID           { return n.id1 }
func (n *Iterate) Length() t.ID          { return n.id2 }
func (n *Iterate) ElseIterate() *Iterate { return n.rhs.AsIterate() }
func (n *Iterate) Assigns() []*Node      { return n.list0 }
func (n *Iterate) Asserts() []*Node      { return n.list1 }
func (n *Iterate) Body() []*Node         { return n.list2 }

func (n *Iterate) SetHasBreak()    { n.flags |= FlagsHasBreak }
func (n *Iterate) SetHasContinue() { n.flags |= FlagsHasContinue }

func NewIterate(label t.ID, assigns []*Node, length t.ID, unroll t.ID, asserts []*Node, body []*Node, elseIterate *Iterate) *Iterate {
	return &Iterate{
		kind:  KIterate,
		id0:   unroll,
		id1:   label,
		id2:   length,
		rhs:   elseIterate.AsNode(),
		list0: assigns,
		list1: asserts,
		list2: body,
	}
}

// While is "while.ID1 MHS, List1 { List2 }":
//  - FlagsHasBreak    is the while has an explicit break
//  - FlagsHasContinue is the while has an explicit continue
//  - ID1:   <0|label>
//  - MHS:   <Expr>
//  - List1: <Assert> asserts
//  - List2: <Statement> body
//
// TODO: should we be able to unroll while loops too?
type While Node

func (n *While) AsNode() *Node     { return (*Node)(n) }
func (n *While) HasBreak() bool    { return n.flags&FlagsHasBreak != 0 }
func (n *While) HasContinue() bool { return n.flags&FlagsHasContinue != 0 }
func (n *While) Label() t.ID       { return n.id1 }
func (n *While) Condition() *Expr  { return n.mhs.AsExpr() }
func (n *While) Asserts() []*Node  { return n.list1 }
func (n *While) Body() []*Node     { return n.list2 }

func (n *While) SetHasBreak()    { n.flags |= FlagsHasBreak }
func (n *While) SetHasContinue() { n.flags |= FlagsHasContinue }

func NewWhile(label t.ID, condition *Expr, asserts []*Node, body []*Node) *While {
	return &While{
		kind:  KWhile,
		id1:   label,
		mhs:   condition.AsNode(),
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

func (n *If) AsNode() *Node        { return (*Node)(n) }
func (n *If) Condition() *Expr     { return n.mhs.AsExpr() }
func (n *If) ElseIf() *If          { return n.rhs.AsIf() }
func (n *If) BodyIfTrue() []*Node  { return n.list2 }
func (n *If) BodyIfFalse() []*Node { return n.list1 }

func NewIf(condition *Expr, bodyIfTrue []*Node, bodyIfFalse []*Node, elseIf *If) *If {
	return &If{
		kind:  KIf,
		mhs:   condition.AsNode(),
		rhs:   elseIf.AsNode(),
		list1: bodyIfFalse,
		list2: bodyIfTrue,
	}
}

// Ret is "return LHS" or "yield LHS":
//  - FlagsReturnsError LHS is an error status
//  - ID0:   <IDReturn|IDYield>
//  - LHS:   <Expr>
type Ret Node

func (n *Ret) AsNode() *Node   { return (*Node)(n) }
func (n *Ret) RetsError() bool { return n.flags&FlagsRetsError != 0 }
func (n *Ret) Keyword() t.ID   { return n.id0 }
func (n *Ret) Value() *Expr    { return n.lhs.AsExpr() }

func (n *Ret) SetRetsError() { n.flags |= FlagsRetsError }

func NewRet(keyword t.ID, value *Expr) *Ret {
	return &Ret{
		kind: KRet,
		id0:  keyword,
		lhs:  value.AsNode(),
	}
}

// Jump is "break" or "continue", with an optional label, "break.label":
//  - ID0:   <IDBreak|IDContinue>
//  - ID1:   <0|label>
type Jump Node

func (n *Jump) AsNode() *Node    { return (*Node)(n) }
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

// TypeExpr is a type expression, such as "base.u32", "base.u32[..= 8]", "foo",
// "pkg.bar", "ptr T", "array[8] T", "slice T" or "table T":
//  - ID0:   <0|IDArray|IDFunc|IDNptr|IDPtr|IDSlice|IDTable>
//  - ID1:   <0|pkg>
//  - ID2:   <0|type name>
//  - LHS:   <nil|Expr>
//  - MHS:   <nil|Expr>
//  - RHS:   <nil|TypeExpr>
//
// An IDNptr or IDPtr ID0 means "nptr RHS" or "ptr RHS". RHS is the inner type.
//
// An IDArray ID0 means "array[LHS] RHS". RHS is the inner type.
//
// An IDSlice ID0 means "slice RHS". RHS is the inner type.
//
// An IDTable ID0 means "table RHS". RHS is the inner type.
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
// Numeric types can be refined as "foo[LHS ..= MHS]". LHS and MHS are Expr's,
// possibly nil. For example, the LHS for "base.u32[..= 4095]" is nil.
//
// TODO: struct types, list types, nptr vs ptr.
type TypeExpr Node

func (n *TypeExpr) AsNode() *Node       { return (*Node)(n) }
func (n *TypeExpr) Decorator() t.ID     { return n.id0 }
func (n *TypeExpr) QID() t.QID          { return t.QID{n.id1, n.id2} }
func (n *TypeExpr) FuncName() t.ID      { return n.id2 }
func (n *TypeExpr) ArrayLength() *Expr  { return n.lhs.AsExpr() }
func (n *TypeExpr) Receiver() *TypeExpr { return n.lhs.AsTypeExpr() }
func (n *TypeExpr) Bounds() [2]*Expr    { return [2]*Expr{n.lhs.AsExpr(), n.mhs.AsExpr()} }
func (n *TypeExpr) Min() *Expr          { return n.lhs.AsExpr() }
func (n *TypeExpr) Max() *Expr          { return n.mhs.AsExpr() }
func (n *TypeExpr) Inner() *TypeExpr    { return n.rhs.AsTypeExpr() }

func (n *TypeExpr) Innermost() *TypeExpr {
	for ; n != nil && n.Inner() != nil; n = n.Inner() {
	}
	return n
}

func (n *TypeExpr) Pointee() *TypeExpr {
	for ; n != nil && (n.id0 == t.IDNptr || n.id0 == t.IDPtr); n = n.Inner() {
	}
	return n
}

func (n *TypeExpr) IsBool() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2 == t.IDBool
}

func (n *TypeExpr) IsIdeal() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2 == t.IDQIdeal
}

func (n *TypeExpr) IsIOType() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && (n.id2 == t.IDIOReader || n.id2 == t.IDIOWriter)
}

func (n *TypeExpr) IsNullptr() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2 == t.IDQNullptr
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

func (n *TypeExpr) IsStatus() bool {
	return n.id0 == 0 && n.id1 == t.IDBase && n.id2 == t.IDStatus
}

func (n *TypeExpr) IsArrayType() bool {
	return n.id0 == t.IDArray
}

func (n *TypeExpr) IsPointerType() bool {
	return n.id0 == t.IDNptr || n.id0 == t.IDPtr
}

func (n *TypeExpr) IsSliceType() bool {
	return n.id0 == t.IDSlice
}

func (n *TypeExpr) IsTableType() bool {
	return n.id0 == t.IDTable
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
		case t.IDNptr, t.IDPtr, t.IDSlice, t.IDTable:
			return true
		}
	}
	return false
}

func (n *TypeExpr) Unrefined() *TypeExpr {
	if !n.IsRefined() {
		return n
	}
	return &TypeExpr{
		kind: KTypeExpr,
		id1:  n.id1,
		id2:  n.id2,
	}
}

func NewTypeExpr(decorator t.ID, pkg t.ID, name t.ID, alenRecvMin *Node, max *Expr, inner *TypeExpr) *TypeExpr {
	return &TypeExpr{
		kind: KTypeExpr,
		id0:  decorator,
		id1:  pkg,
		id2:  name,
		lhs:  alenRecvMin,
		mhs:  max.AsNode(),
		rhs:  inner.AsNode(),
	}
}

// MaxBodyDepth is an advisory limit for a function body's recursion depth.
const MaxBodyDepth = 255

// Func is "func ID2.ID0(LHS)(RHS) { List2 }":
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
//  - IOBind
//  - If
//  - Iterate
//  - Jump
//  - Ret
//  - Var
//  - While
type Func Node

func (n *Func) AsNode() *Node    { return (*Node)(n) }
func (n *Func) Effect() Effect   { return Effect(n.flags) }
func (n *Func) Public() bool     { return n.flags&FlagsPublic != 0 }
func (n *Func) Filename() string { return n.filename }
func (n *Func) Line() uint32     { return n.line }
func (n *Func) QQID() t.QQID     { return t.QQID{n.id1, n.id2, n.id0} }
func (n *Func) Receiver() t.QID  { return t.QID{n.id1, n.id2} }
func (n *Func) FuncName() t.ID   { return n.id0 }
func (n *Func) In() *Struct      { return n.lhs.AsStruct() }
func (n *Func) Out() *TypeExpr   { return n.rhs.AsTypeExpr() }
func (n *Func) Asserts() []*Node { return n.list1 }
func (n *Func) Body() []*Node    { return n.list2 }

func NewFunc(flags Flags, filename string, line uint32, receiverName t.ID, funcName t.ID, in *Struct, out *TypeExpr, asserts []*Node, body []*Node) *Func {
	return &Func{
		kind:     KFunc,
		flags:    flags,
		filename: filename,
		line:     line,
		id0:      funcName,
		id2:      receiverName,
		lhs:      in.AsNode(),
		rhs:      out.AsNode(),
		list1:    asserts,
		list2:    body,
	}
}

// Status is "error (RHS) ID2" or "suspension (RHS) ID2":
//  - FlagsPublic      is "pub" vs "pri"
//  - ID1:   <0|pkg> (set by calling SetPackage)
//  - ID2:   message
type Status Node

func (n *Status) AsNode() *Node    { return (*Node)(n) }
func (n *Status) Public() bool     { return n.flags&FlagsPublic != 0 }
func (n *Status) Filename() string { return n.filename }
func (n *Status) Line() uint32     { return n.line }
func (n *Status) QID() t.QID       { return t.QID{n.id1, n.id2} }

func NewStatus(flags Flags, filename string, line uint32, message t.ID) *Status {
	return &Status{
		kind:     KStatus,
		flags:    flags,
		filename: filename,
		line:     line,
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

func (n *Const) AsNode() *Node    { return (*Node)(n) }
func (n *Const) Public() bool     { return n.flags&FlagsPublic != 0 }
func (n *Const) Filename() string { return n.filename }
func (n *Const) Line() uint32     { return n.line }
func (n *Const) QID() t.QID       { return t.QID{n.id1, n.id2} }
func (n *Const) XType() *TypeExpr { return n.lhs.AsTypeExpr() }
func (n *Const) Value() *Expr     { return n.rhs.AsExpr() }

func NewConst(flags Flags, filename string, line uint32, name t.ID, xType *TypeExpr, value *Expr) *Const {
	return &Const{
		kind:     KConst,
		flags:    flags,
		filename: filename,
		line:     line,
		id2:      name,
		lhs:      xType.AsNode(),
		rhs:      value.AsNode(),
	}
}

// Struct is "struct ID2(List0)" or "struct ID2?(List0)":
//  - FlagsPublic      is "pub" vs "pri"
//  - FlagsClassy      is "ID2" vs "ID2?"
//  - ID1:   <0|pkg> (set by calling SetPackage)
//  - ID2:   name
//  - List0: <Field> fields
//
// The question mark indicates a classy struct - one that supports methods,
// especially coroutines.
type Struct Node

func (n *Struct) AsNode() *Node    { return (*Node)(n) }
func (n *Struct) Classy() bool     { return n.flags&FlagsClassy != 0 }
func (n *Struct) Public() bool     { return n.flags&FlagsPublic != 0 }
func (n *Struct) Filename() string { return n.filename }
func (n *Struct) Line() uint32     { return n.line }
func (n *Struct) QID() t.QID       { return t.QID{n.id1, n.id2} }
func (n *Struct) Fields() []*Node  { return n.list0 }

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

// Use is "use ID2":
//  - ID2:   <string literal> package path
type Use Node

func (n *Use) AsNode() *Node    { return (*Node)(n) }
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
//  - List0: <Const|Func|Status|Struct|Use> top-level declarations
type File Node

func (n *File) AsNode() *Node          { return (*Node)(n) }
func (n *File) Filename() string       { return n.filename }
func (n *File) TopLevelDecls() []*Node { return n.list0 }

func NewFile(filename string, topLevelDecls []*Node) *File {
	return &File{
		kind:     KFile,
		filename: filename,
		list0:    topLevelDecls,
	}
}
