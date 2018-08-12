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

package cgen

import (
	"fmt"

	"github.com/google/wuffs/lang/builtin"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

func (g *gen) writeExpr(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("expression recursion depth too large")
	}
	depth++

	if rp == replaceCallSuspendibles && n.Effect().RootCause() && n.Effect().Coroutine() {
		if g.currFunk.tempR >= g.currFunk.tempW {
			return fmt.Errorf("internal error: temporary variable count out of sync")
		}
		// TODO: check that this works with nested call-suspendibles:
		// "foo?().bar().qux?()(p?(), q?())".
		//
		// Also be aware of evaluation order in the presence of side effects:
		// in "foo(a?(), b!(), c?())", b should be called between a and c.
		b.printf("%s%d", tPrefix, g.currFunk.tempR)
		g.currFunk.tempR++
		return nil
	}

	if cv := n.ConstValue(); cv != nil {
		if typ := n.MType(); typ.IsNumTypeOrIdeal() {
			b.writes(cv.String())
		} else if typ.IsNullptr() {
			b.writes("NULL")
		} else if cv.Cmp(zero) == 0 {
			b.writes("false")
		} else if cv.Cmp(one) == 0 {
			b.writes("true")
		} else {
			return fmt.Errorf("%v has type bool but constant value %v is neither 0 or 1", n.Str(g.tm), cv)
		}
		return nil
	}

	switch op := n.Operator(); {
	case op.IsXUnaryOp():
		return g.writeExprUnaryOp(b, n, rp, depth)
	case op.IsXBinaryOp():
		return g.writeExprBinaryOp(b, n, rp, depth)
	case op.IsXAssociativeOp():
		return g.writeExprAssociativeOp(b, n, rp, depth)
	}
	return g.writeExprOther(b, n, rp, depth)
}

func (g *gen) writeExprOther(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	switch n.Operator() {
	case 0:
		if id1 := n.Ident(); id1 == t.IDThis {
			b.writes("self")
		} else {
			if n.GlobalIdent() {
				b.writes(g.pkgPrefix)
			} else {
				b.writes(vPrefix)
			}
			b.writes(id1.Str(g.tm))
		}
		return nil

	case t.IDOpenParen:
		// n is a function call.
		if err := g.writeBuiltinCall(b, n, rp, depth); err != errNoSuchBuiltin {
			return err
		}

		if n.LHS().AsExpr().Ident() == t.IDReset {
			method := n.LHS().AsExpr()
			recv := method.LHS().AsExpr()
			recvTyp, addr := recv.MType(), "&"
			if p := recvTyp.Decorator(); p == t.IDNptr || p == t.IDPtr {
				recvTyp, addr = recvTyp.Inner(), ""
			}
			if recvTyp.Decorator() != 0 {
				return fmt.Errorf("cannot generate reset method call %q for receiver type %q",
					n.Str(g.tm), recv.MType().Str(g.tm))
			}
			qid := recvTyp.QID()

			// Generate a three part expression using the comma operator:
			// "(memset call, check_wuffs_version call, return_empty_struct
			// call)". The final part is a function call (to a static inline
			// function) instead of a struct literal, to avoid a "expression
			// result unused" compiler error.

			b.printf("(memset(%s", addr)
			// TODO: ensure that the recv expression is idempotent.
			if err := g.writeExpr(b, recv, rp, depth); err != nil {
				return err
			}
			b.printf(", 0, sizeof ((%s%s){})),", g.packagePrefix(qid), qid[1].Str(g.tm))

			b.printf("wuffs_base__ignore_check_wuffs_version_status("+
				"%s%s__check_wuffs_version(%s", g.packagePrefix(qid), qid[1].Str(g.tm), addr)
			if err := g.writeExpr(b, recv, rp, depth); err != nil {
				return err
			}
			b.printf(", sizeof ((%s%s){}), WUFFS_VERSION))", g.packagePrefix(qid), qid[1].Str(g.tm))

			b.writes(", wuffs_base__return_empty_struct())")
			return nil
		}

		return g.writeExprUserDefinedCall(b, n, rp, depth)

	case t.IDOpenBracket:
		// n is an index.
		if err := g.writeExpr(b, n.LHS().AsExpr(), rp, depth); err != nil {
			return err
		}
		if lTyp := n.LHS().AsExpr().MType(); lTyp.IsSliceType() {
			// TODO: don't assume that the slice is a slice of base.u8.
			b.writes(".ptr")
		}
		b.writeb('[')
		if err := g.writeExpr(b, n.RHS().AsExpr(), rp, depth); err != nil {
			return err
		}
		b.writeb(']')
		return nil

	case t.IDColon:
		// n is a slice.
		lhs := n.LHS().AsExpr()
		mhs := n.MHS().AsExpr()
		rhs := n.RHS().AsExpr()
		switch {
		case mhs != nil && rhs == nil:
			b.writes("wuffs_base__slice_u8__subslice_i(")
		case mhs == nil && rhs != nil:
			b.writes("wuffs_base__slice_u8__subslice_j(")
		case mhs != nil && rhs != nil:
			b.writes("wuffs_base__slice_u8__subslice_ij(")
		}

		lhsIsArray := lhs.MType().IsArrayType()
		if lhsIsArray {
			// TODO: don't assume that the slice is a slice of base.u8.
			b.writes("((wuffs_base__slice_u8){.ptr=")
		}
		if err := g.writeExpr(b, lhs, rp, depth); err != nil {
			return err
		}
		if lhsIsArray {
			b.printf(",.len=%v})", lhs.MType().ArrayLength().ConstValue())
		}

		if mhs != nil {
			b.writeb(',')
			if err := g.writeExpr(b, mhs, rp, depth); err != nil {
				return err
			}
		}
		if rhs != nil {
			b.writeb(',')
			if err := g.writeExpr(b, rhs, rp, depth); err != nil {
				return err
			}
		}
		if mhs != nil || rhs != nil {
			b.writeb(')')
		}
		return nil

	case t.IDDot:
		lhs := n.LHS().AsExpr()
		if lhs.Ident() == t.IDArgs {
			b.writes(aPrefix)
			b.writes(n.Ident().Str(g.tm))
			return nil
		}

		if err := g.writeExpr(b, lhs, rp, depth); err != nil {
			return err
		}
		if p := lhs.MType().Decorator(); p == t.IDNptr || p == t.IDPtr {
			b.writes("->")
		} else {
			b.writes(".")
		}
		b.writes("private_impl." + fPrefix)
		b.writes(n.Ident().Str(g.tm))
		return nil

	case t.IDStatus:
		if z := g.statusMap[n.StatusQID()]; z.cName != "" {
			b.writes(z.cName)
		} else {
			msg, _ := t.Unescape(n.Ident().Str(g.tm))
			if !builtin.StatusMap[msg] {
				return fmt.Errorf("no status code for %q", msg)
			}
			b.writes("wuffs_base__")
			if statusMsgIsError(msg) {
				b.writes("error__")
			} else {
				b.writes("suspension__")
			}
			b.writes(cName(msg, ""))
		}
		return nil
	}
	return fmt.Errorf("unrecognized token (0x%X) for writeExprOther", n.Operator())
}

func (g *gen) writeExprUnaryOp(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	op := n.Operator()
	opName := cOpName(op)
	if opName == "" {
		return fmt.Errorf("unrecognized operator %q", op.AmbiguousForm().Str(g.tm))
	}

	b.writes(opName)
	return g.writeExpr(b, n.RHS().AsExpr(), rp, depth)
}

func (g *gen) writeExprBinaryOp(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	opName := ""

	op := n.Operator()
	switch op {
	case t.IDXBinaryTildeSatPlus, t.IDXBinaryTildeSatMinus:
		uBits := uintBits(n.MType().QID())
		if uBits == 0 {
			return fmt.Errorf("unsupported tilde-operator type %q", n.MType().Str(g.tm))
		}
		uOp := "add"
		if op != t.IDXBinaryTildeSatPlus {
			uOp = "sub"
		}
		b.printf("wuffs_base__u%d__sat_%s", uBits, uOp)
		opName = ","

	case t.IDXBinaryAs:
		return g.writeExprAs(b, n.LHS().AsExpr(), n.RHS().AsTypeExpr(), rp, depth)

	default:
		opName = cOpName(op)
		if opName == "" {
			return fmt.Errorf("unrecognized operator %q", op.AmbiguousForm().Str(g.tm))
		}
	}

	b.writeb('(')
	if err := g.writeExpr(b, n.LHS().AsExpr(), rp, depth); err != nil {
		return err
	}
	b.writes(opName)
	if err := g.writeExpr(b, n.RHS().AsExpr(), rp, depth); err != nil {
		return err
	}
	b.writeb(')')
	return nil
}

func (g *gen) writeExprAs(b *buffer, lhs *a.Expr, rhs *a.TypeExpr, rp replacementPolicy, depth uint32) error {
	b.writes("((")
	// TODO: watch for passing an array type to writeCTypeName? In C, an array
	// type can decay into a pointer.
	if err := g.writeCTypeName(b, rhs, "", ""); err != nil {
		return err
	}
	b.writes(")(")
	if err := g.writeExpr(b, lhs, rp, depth); err != nil {
		return err
	}
	b.writes("))")
	return nil
}

func (g *gen) writeExprAssociativeOp(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	op := n.Operator()
	opName := cOpName(op)
	if opName == "" {
		return fmt.Errorf("unrecognized operator %q", op.AmbiguousForm().Str(g.tm))
	}

	b.writeb('(')
	for i, o := range n.Args() {
		if i != 0 {
			b.writes(opName)
		}
		if err := g.writeExpr(b, o.AsExpr(), rp, depth); err != nil {
			return err
		}
	}
	b.writeb(')')
	return nil
}

func (g *gen) writeExprUserDefinedCall(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	method := n.LHS().AsExpr()
	recv := method.LHS().AsExpr()
	recvTyp, addr := recv.MType(), "&"
	if p := recvTyp.Decorator(); p == t.IDNptr || p == t.IDPtr {
		recvTyp, addr = recvTyp.Inner(), ""
	} else if recvTyp.IsStatus() {
		addr = ""
	}
	if recvTyp.Decorator() != 0 {
		return fmt.Errorf("cannot generate user-defined method call %q for receiver type %q",
			n.Str(g.tm), recv.MType().Str(g.tm))
	}
	qid := recvTyp.QID()
	b.printf("%s%s__%s(%s", g.packagePrefix(qid), qid[1].Str(g.tm), method.Ident().Str(g.tm), addr)
	if err := g.writeExpr(b, recv, rp, depth); err != nil {
		return err
	}
	if len(n.Args()) > 0 {
		b.writeb(',')
	}
	return g.writeArgs(b, n.Args(), rp, depth)
}

func (g *gen) writeCTypeName(b *buffer, n *a.TypeExpr, varNamePrefix string, varName string) error {
	// It may help to refer to http://unixwiz.net/techtips/reading-cdecl.html

	// TODO: fix this, allow slices of all types, not just of base.u8's. Also
	// allow arrays of slices, slices of pointers, etc.
	if n.IsSliceType() {
		o := n.Inner()
		if o.Decorator() == 0 && o.QID() == (t.QID{t.IDBase, t.IDU8}) && !o.IsRefined() {
			b.writes("wuffs_base__slice_u8")
			b.writeb(' ')
			b.writes(varNamePrefix)
			b.writes(varName)
			return nil
		}
		return fmt.Errorf("cannot convert Wuffs type %q to C", n.Str(g.tm))
	}
	if n.IsTableType() {
		o := n.Inner()
		if o.Decorator() == 0 && o.QID() == (t.QID{t.IDBase, t.IDU8}) && !o.IsRefined() {
			b.writes("wuffs_base__table_u8")
			b.writeb(' ')
			b.writes(varNamePrefix)
			b.writes(varName)
			return nil
		}
		return fmt.Errorf("cannot convert Wuffs type %q to C", n.Str(g.tm))
	}

	// maxNumPointers is an arbitrary implementation restriction.
	const maxNumPointers = 16

	x := n
	for ; x != nil && x.IsArrayType(); x = x.Inner() {
	}

	numPointers, innermost := 0, x
	for ; innermost != nil && innermost.Inner() != nil; innermost = innermost.Inner() {
		if p := innermost.Decorator(); p == t.IDNptr || p == t.IDPtr {
			if numPointers == maxNumPointers {
				return fmt.Errorf("cannot convert Wuffs type %q to C: too many ptr's", n.Str(g.tm))
			}
			numPointers++
			continue
		}
		// TODO: fix this.
		return fmt.Errorf("cannot convert Wuffs type %q to C", n.Str(g.tm))
	}

	fallback := true
	if qid := innermost.QID(); qid[0] == t.IDBase {
		if key := qid[1]; key < t.ID(len(cTypeNames)) {
			if s := cTypeNames[key]; s != "" {
				b.writes(s)
				fallback = false
			}
		}
	}
	if fallback {
		qid := innermost.QID()
		b.printf("%s%s", g.packagePrefix(qid), qid[1].Str(g.tm))
	}

	for i := 0; i < numPointers; i++ {
		b.writeb('*')
	}

	b.writeb(' ')
	b.writes(varNamePrefix)
	b.writes(varName)

	x = n
	for ; x != nil && x.IsArrayType(); x = x.Inner() {
		b.writeb('[')
		b.writes(x.ArrayLength().ConstValue().String())
		b.writeb(']')
	}

	return nil
}

func (g *gen) packagePrefix(qid t.QID) string {
	if qid[0] != 0 {
		otherPkg := g.tm.ByID(qid[0])
		// TODO: map the "deflate" in "deflate.decoder" to the "deflate" in
		// `use "std/deflate"`, and use the latter "deflate".
		//
		// This is pretty academic at the moment, since they're the same
		// "deflate", but in the future, we might be able to rename used
		// packages, e.g. `use "foo/bar" as "baz"`, so "baz.qux" would map
		// to generating "wuffs_bar__qux".
		//
		// TODO: sanitize or validate otherPkg, e.g. that it's ASCII only?
		//
		// See gen.writeInitializerImpl for a similar use of otherPkg.
		return "wuffs_" + otherPkg + "__"
	}
	return g.pkgPrefix
}

var cTypeNames = [...]string{
	t.IDI8:       "int8_t",
	t.IDI16:      "int16_t",
	t.IDI32:      "int32_t",
	t.IDI64:      "int64_t",
	t.IDU8:       "uint8_t",
	t.IDU16:      "uint16_t",
	t.IDU32:      "uint32_t",
	t.IDU64:      "uint64_t",
	t.IDBool:     "bool",
	t.IDIOReader: "wuffs_base__io_reader",
	t.IDIOWriter: "wuffs_base__io_writer",
}

const noSuchCOperator = " no_such_C_operator "

func cOpName(x t.ID) string {
	if x < t.ID(len(cOpNames)) {
		if s := cOpNames[x]; s != "" {
			return s
		}
	}
	return noSuchCOperator
}

var cOpNames = [...]string{
	t.IDEq:               " = ",
	t.IDPlusEq:           " += ",
	t.IDMinusEq:          " -= ",
	t.IDStarEq:           " *= ",
	t.IDSlashEq:          " /= ",
	t.IDShiftLEq:         " <<= ",
	t.IDShiftREq:         " >>= ",
	t.IDAmpEq:            " &= ",
	t.IDPipeEq:           " |= ",
	t.IDHatEq:            " ^= ",
	t.IDPercentEq:        " %= ",
	t.IDTildeModShiftLEq: " <<= ",
	t.IDTildeModPlusEq:   " += ",
	t.IDTildeModMinusEq:  " -= ",
	t.IDTildeSatPlusEq:   noSuchCOperator,
	t.IDTildeSatMinusEq:  noSuchCOperator,

	t.IDXUnaryPlus:  " + ",
	t.IDXUnaryMinus: " - ",
	t.IDXUnaryNot:   " ! ",
	t.IDXUnaryRef:   " & ",
	t.IDXUnaryDeref: " * ",

	t.IDXBinaryPlus:           " + ",
	t.IDXBinaryMinus:          " - ",
	t.IDXBinaryStar:           " * ",
	t.IDXBinarySlash:          " / ",
	t.IDXBinaryShiftL:         " << ",
	t.IDXBinaryShiftR:         " >> ",
	t.IDXBinaryAmp:            " & ",
	t.IDXBinaryPipe:           " | ",
	t.IDXBinaryHat:            " ^ ",
	t.IDXBinaryPercent:        " % ",
	t.IDXBinaryTildeModShiftL: " << ",
	t.IDXBinaryTildeModPlus:   " + ",
	t.IDXBinaryTildeModMinus:  " - ",
	t.IDXBinaryTildeSatPlus:   noSuchCOperator,
	t.IDXBinaryTildeSatMinus:  noSuchCOperator,
	t.IDXBinaryNotEq:          " != ",
	t.IDXBinaryLessThan:       " < ",
	t.IDXBinaryLessEq:         " <= ",
	t.IDXBinaryEqEq:           " == ",
	t.IDXBinaryGreaterEq:      " >= ",
	t.IDXBinaryGreaterThan:    " > ",
	t.IDXBinaryAnd:            " && ",
	t.IDXBinaryOr:             " || ",
	t.IDXBinaryAs:             noSuchCOperator,

	t.IDXAssociativePlus: " + ",
	t.IDXAssociativeStar: " * ",
	t.IDXAssociativeAmp:  " & ",
	t.IDXAssociativePipe: " | ",
	t.IDXAssociativeHat:  " ^ ",
	t.IDXAssociativeAnd:  " && ",
	t.IDXAssociativeOr:   " || ",
}
