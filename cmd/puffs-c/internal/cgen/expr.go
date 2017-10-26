// Copyright 2017 The Puffs Authors.
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

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

func (g *gen) writeExpr(b *buffer, n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("expression recursion depth too large")
	}
	depth++

	if rp == replaceCallSuspendibles && n.CallSuspendible() {
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
		if !n.MType().IsBool() {
			b.writes(cv.String())
		} else if cv.Cmp(zero) == 0 {
			b.writes("false")
		} else if cv.Cmp(one) == 0 {
			b.writes("true")
		} else {
			return fmt.Errorf("%v has type bool but constant value %v is neither 0 or 1", n.String(g.tm), cv)
		}
		return nil
	}

	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		if err := g.writeExprOther(b, n, rp, pp, depth); err != nil {
			return err
		}
	case t.FlagsUnaryOp:
		if err := g.writeExprUnaryOp(b, n, rp, pp, depth); err != nil {
			return err
		}
	case t.FlagsBinaryOp:
		if err := g.writeExprBinaryOp(b, n, rp, pp, depth); err != nil {
			return err
		}
	case t.FlagsAssociativeOp:
		if err := g.writeExprAssociativeOp(b, n, rp, pp, depth); err != nil {
			return err
		}
	default:
		return fmt.Errorf("unrecognized token.Key (0x%X) for writeExpr", n.ID0().Key())
	}

	return nil
}

func (g *gen) writeExprOther(b *buffer, n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	switch n.ID0().Key() {
	case 0:
		if id1 := n.ID1(); id1.Key() == t.KeyThis {
			b.writes("self")
		} else {
			if n.GlobalIdent() {
				b.writes(g.pkgPrefix)
			} else {
				b.writes(vPrefix)
			}
			b.writes(id1.String(g.tm))
		}
		return nil

	case t.KeyOpenParen:
		// n is a function call.
		// TODO: delete this hack that only matches "foo.bar_bits(etc)".
		if isThatMethod(g.tm, n, t.KeyLowBits, 1) {
			// "x.low_bits(n:etc)" in C is "((x) & ((1 << (n)) - 1))".
			x := n.LHS().Expr().LHS().Expr()
			b.writes("((")
			if err := g.writeExpr(b, x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(") & ((1 << (")
			if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(")) - 1))")
			return nil
		}
		if isThatMethod(g.tm, n, t.KeyHighBits, 1) {
			// "x.high_bits(n:etc)" in C is "((x) >> (8*sizeof(x) - (n)))".
			x := n.LHS().Expr().LHS().Expr()
			b.writes("((")
			if err := g.writeExpr(b, x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(") >> (")
			if sz, err := g.sizeof(x.MType()); err != nil {
				return err
			} else {
				b.printf("%d", 8*sz)
			}
			b.writes(" - (")
			if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(")))")
			return nil
		}
		if isThatMethod(g.tm, n, t.KeyIsError, 0) || isThatMethod(g.tm, n, t.KeyIsOK, 0) ||
			isThatMethod(g.tm, n, t.KeyIsSuspension, 0) {
			if pp == parenthesesMandatory {
				b.writeb('(')
			}
			x := n.LHS().Expr().LHS().Expr()
			if err := g.writeExpr(b, x, rp, parenthesesMandatory, depth); err != nil {
				return err
			}
			switch key := n.LHS().Expr().ID1().Key(); key {
			case t.KeyIsError:
				b.writes(" < 0")
			case t.KeyIsOK:
				b.writes(" == 0")
			case t.KeyIsSuspension:
				b.writes(" > 0")
			default:
				return fmt.Errorf("unrecognized token.Key (0x%X) for writeExprOther's IsXxx", key)
			}
			if pp == parenthesesMandatory {
				b.writeb(')')
			}
			return nil
		}
		if isThatMethod(g.tm, n, t.KeySuffix, 1) {
			// TODO: don't assume that the slice is a slice of u8.
			b.writes("puffs_base__slice_u8_suffix(")
			x := n.LHS().Expr().LHS().Expr()
			if err := g.writeExpr(b, x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writeb(',')
			if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(")")
			return nil
		}
		if isInDst(g.tm, n, t.KeySinceMark, 0) {
			b.printf("((puffs_base__slice_u8){ "+
				".ptr = %sdst.private_impl.mark, "+
				".len = %sdst.private_impl.mark ? %swptr_dst - %sdst.private_impl.mark : 0, })",
				aPrefix, aPrefix, bPrefix, aPrefix)
			return nil
		}
		if isInDst(g.tm, n, t.KeyMark, 0) {
			// TODO: is a private_impl.mark the right representation? What if
			// the function is passed a (ptr writer1) instead of a (writer1)?
			// Do we still want to have that mark live outside of the function
			// scope?
			b.printf("puffs_base__writer1__mark(&%sdst, %swptr_dst)", aPrefix, bPrefix)
			return nil
		}
		if isInDst(g.tm, n, t.KeyCopyFromReader32, 2) {
			b.printf("puffs_base__writer1__copy_from_reader32(&%swptr_dst, %swend_dst", bPrefix, bPrefix)
			// TODO: don't assume that the first argument is "in.src".
			b.printf(", &%srptr_src, %srend_src,", bPrefix, bPrefix)
			a := n.Args()[1].Arg().Value()
			if err := g.writeExpr(b, a, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writeb(')')
			return nil
		}
		if isInDst(g.tm, n, t.KeyCopyFromHistory32, 2) {
			b.printf("puffs_base__writer1__copy_from_history32(&%swptr_dst, %sdst.private_impl.mark , %swend_dst",
				bPrefix, aPrefix, bPrefix)
			for _, o := range n.Args() {
				b.writeb(',')
				if err := g.writeExpr(b, o.Arg().Value(), rp, parenthesesOptional, depth); err != nil {
					return err
				}
			}
			b.writeb(')')
			return nil
		}
		if isInDst(g.tm, n, t.KeyCopyFromSlice32, 2) {
			b.printf("puffs_base__writer1__copy_from_slice32(&%swptr_dst, %swend_dst", bPrefix, bPrefix)
			for _, o := range n.Args() {
				b.writeb(',')
				if err := g.writeExpr(b, o.Arg().Value(), rp, parenthesesOptional, depth); err != nil {
					return err
				}
			}
			b.writeb(')')
			return nil
		}
		if isInDst(g.tm, n, t.KeyCopyFromSlice, 1) {
			b.printf("puffs_base__writer1__copy_from_slice(&%swptr_dst, %swend_dst,", bPrefix, bPrefix)
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, a, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writeb(')')
			return nil
		}
		if isThatMethod(g.tm, n, t.KeyCopyFromSlice, 1) {
			b.writes("puffs_base__slice_u8__copy_from_slice(")
			receiver := n.LHS().Expr().LHS().Expr()
			if err := g.writeExpr(b, receiver, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writeb(',')
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, a, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(")\n")
			return nil
		}
		if isThatMethod(g.tm, n, t.KeyLength, 0) {
			if pp == parenthesesMandatory {
				b.writeb('(')
			}
			b.writes("(uint64_t)(")
			if err := g.writeExpr(b, n.LHS().Expr().LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writes(".len)")
			if pp == parenthesesMandatory {
				b.writeb(')')
			}
			return nil
		}
		if isThatMethod(g.tm, n, t.KeyAvailable, 0) {
			if pp == parenthesesMandatory {
				b.writeb('(')
			}
			// TODO: don't hard-code dst.
			const wName = "dst"
			b.printf("(uint64_t)(%swend_%s - %swptr_%s)", bPrefix, wName, bPrefix, wName)
			if pp == parenthesesMandatory {
				b.writeb(')')
			}
			return nil
		}
		if isThatMethod(g.tm, n, g.tm.ByName("update").Key(), 1) {
			// TODO: don't hard-code this.adler.
			b.printf("%sadler32__update(&self->private_impl.f_adler, ", g.pkgPrefix)
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, a, rp, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writes(")\n")
			return nil
		}
		if isThatMethod(g.tm, n, g.tm.ByName("set_literal_width").Key(), 1) {
			// TODO: don't hard-code lzw.
			b.printf("%slzw_decoder__set_literal_width(&self->private_impl.f_lzw, ", g.pkgPrefix)
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, a, rp, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writes(")\n")
			return nil
		}
		// TODO.

	case t.KeyOpenBracket:
		// n is an index.
		if err := g.writeExpr(b, n.LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
		}
		if lTyp := n.LHS().Expr().MType(); lTyp.Decorator().Key() == t.KeyColon {
			// TODO: don't assume that the slice is a slice of u8.
			b.writes(".ptr")
		}
		b.writeb('[')
		if err := g.writeExpr(b, n.RHS().Expr(), rp, parenthesesOptional, depth); err != nil {
			return err
		}
		b.writeb(']')
		return nil

	case t.KeyColon:
		// n is a slice.
		lhs := n.LHS().Expr()
		mhs := n.MHS().Expr()
		rhs := n.RHS().Expr()
		switch {
		case mhs != nil && rhs == nil:
			b.writes("puffs_base__slice_u8__subslice_i(")
		case mhs == nil && rhs != nil:
			b.writes("puffs_base__slice_u8__subslice_j(")
		case mhs != nil && rhs != nil:
			b.writes("puffs_base__slice_u8__subslice_ij(")
		}

		lhsIsArray := lhs.MType().Decorator().Key() == t.KeyOpenBracket
		if lhsIsArray {
			// TODO: don't assume that the slice is a slice of u8.
			b.writes("((puffs_base__slice_u8){.ptr=")
		}
		if err := g.writeExpr(b, lhs, rp, parenthesesOptional, depth); err != nil {
			return err
		}
		if lhsIsArray {
			b.printf(",.len=%v})", lhs.MType().ArrayLength().ConstValue())
		}

		if mhs != nil {
			b.writeb(',')
			if err := g.writeExpr(b, mhs, rp, parenthesesOptional, depth); err != nil {
				return err
			}
		}
		if rhs != nil {
			b.writeb(',')
			if err := g.writeExpr(b, rhs, rp, parenthesesOptional, depth); err != nil {
				return err
			}
		}
		if mhs != nil || rhs != nil {
			b.writeb(')')
		}
		return nil

	case t.KeyDot:
		lhs := n.LHS().Expr()
		if lhs.ID1().Key() == t.KeyIn {
			b.writes(aPrefix)
			b.writes(n.ID1().String(g.tm))
			return nil
		}

		if err := g.writeExpr(b, lhs, rp, parenthesesMandatory, depth); err != nil {
			return err
		}
		if key := lhs.MType().Decorator().Key(); key == t.KeyPtr || key == t.KeyNptr {
			b.writes("->")
		} else {
			b.writes(".")
		}
		b.writes("private_impl." + fPrefix)
		b.writes(n.ID1().String(g.tm))
		return nil

	case t.KeyLimit:
		// TODO: don't hard code so much detail.
		b.writes("(puffs_base__reader1) {")
		b.writes(".buf = a_src.buf,")
		b.writes(".limit = (puffs_base__limit1) {")
		b.printf(".ptr_to_len = &%s%s,", lPrefix, g.currFunk.limitVarName)
		b.writes(".next = &a_src.limit,")
		b.writes("}}")
		g.currFunk.limitVarName = ""
		return nil
	}
	return fmt.Errorf("unrecognized token.Key (0x%X) for writeExprOther", n.ID0().Key())
}

func (g *gen) writeExprUnaryOp(b *buffer, n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	b.writes(cOpNames[0xFF&n.ID0().Key()])
	return g.writeExpr(b, n.RHS().Expr(), rp, parenthesesMandatory, depth)
}

func (g *gen) writeExprBinaryOp(b *buffer, n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	op := n.ID0()
	if op.Key() == t.KeyXBinaryAs {
		return g.writeExprAs(b, n.LHS().Expr(), n.RHS().TypeExpr(), rp, depth)
	}
	if pp == parenthesesMandatory {
		b.writeb('(')
	}
	if err := g.writeExpr(b, n.LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
		return err
	}
	// TODO: does KeyXBinaryAmpHat need special consideration?
	b.writes(cOpNames[0xFF&op.Key()])
	if err := g.writeExpr(b, n.RHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
		return err
	}
	if pp == parenthesesMandatory {
		b.writeb(')')
	}
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
	if err := g.writeExpr(b, lhs, rp, parenthesesMandatory, depth); err != nil {
		return err
	}
	b.writes("))")
	return nil
}

func (g *gen) writeExprAssociativeOp(b *buffer, n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	if pp == parenthesesMandatory {
		b.writeb('(')
	}
	opName := cOpNames[0xFF&n.ID0().Key()]
	for i, o := range n.Args() {
		if i != 0 {
			b.writes(opName)
		}
		if err := g.writeExpr(b, o.Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
		}
	}
	if pp == parenthesesMandatory {
		b.writeb(')')
	}
	return nil
}

func (g *gen) writeCTypeName(b *buffer, n *a.TypeExpr, varNamePrefix string, varName string) error {
	// It may help to refer to http://unixwiz.net/techtips/reading-cdecl.html

	// TODO: fix this, allow slices of all types, not just of u8's. Also allow
	// arrays of slices, slices of pointers, etc.
	if n.Decorator().Key() == t.KeyColon {
		o := n.Inner()
		if o.Decorator() == 0 && o.Name().Key() == t.KeyU8 && !o.IsRefined() {
			b.writes("puffs_base__slice_u8")
			b.writeb(' ')
			b.writes(varNamePrefix)
			b.writes(varName)
			return nil
		}
		return fmt.Errorf("cannot convert Puffs type %q to C", n.String(g.tm))
	}

	// maxNumPointers is an arbitrary implementation restriction.
	const maxNumPointers = 16

	x := n
	for ; x != nil && x.Decorator().Key() == t.KeyOpenBracket; x = x.Inner() {
	}

	numPointers, innermost := 0, x
	for ; innermost != nil && innermost.Inner() != nil; innermost = innermost.Inner() {
		// TODO: "nptr T", not just "ptr T".
		if p := innermost.Decorator().Key(); p == t.KeyPtr {
			if numPointers == maxNumPointers {
				return fmt.Errorf("cannot convert Puffs type %q to C: too many ptr's", n.String(g.tm))
			}
			numPointers++
			continue
		}
		// TODO: fix this.
		return fmt.Errorf("cannot convert Puffs type %q to C", n.String(g.tm))
	}

	fallback := true
	if key := innermost.Name().Key(); key < t.Key(len(cTypeNames)) {
		if s := cTypeNames[key]; s != "" {
			b.writes(s)
			fallback = false
		}
	}
	if fallback {
		b.printf("%s%s", g.pkgPrefix, innermost.Name().String(g.tm))
	}

	for i := 0; i < numPointers; i++ {
		b.writeb('*')
	}

	b.writeb(' ')
	b.writes(varNamePrefix)
	b.writes(varName)

	x = n
	for ; x != nil && x.Decorator().Key() == t.KeyOpenBracket; x = x.Inner() {
		b.writeb('[')
		b.writes(x.ArrayLength().ConstValue().String())
		b.writeb(']')
	}

	return nil
}

var cTypeNames = [...]string{
	t.KeyI8:      "int8_t",
	t.KeyI16:     "int16_t",
	t.KeyI32:     "int32_t",
	t.KeyI64:     "int64_t",
	t.KeyU8:      "uint8_t",
	t.KeyU16:     "uint16_t",
	t.KeyU32:     "uint32_t",
	t.KeyU64:     "uint64_t",
	t.KeyUsize:   "size_t",
	t.KeyBool:    "bool",
	t.KeyBuf1:    "puffs_base__buf1",
	t.KeyReader1: "puffs_base__reader1",
	t.KeyWriter1: "puffs_base__writer1",
	t.KeyBuf2:    "puffs_base__buf2",
}

var cOpNames = [256]string{
	t.KeyEq:          " = ",
	t.KeyPlusEq:      " += ",
	t.KeyMinusEq:     " -= ",
	t.KeyStarEq:      " *= ",
	t.KeySlashEq:     " /= ",
	t.KeyShiftLEq:    " <<= ",
	t.KeyShiftREq:    " >>= ",
	t.KeyAmpEq:       " &= ",
	t.KeyAmpHatEq:    " no_such_amp_hat_C_operator ",
	t.KeyPipeEq:      " |= ",
	t.KeyHatEq:       " ^= ",
	t.KeyPercentEq:   " %= ",
	t.KeyTildePlusEq: " += ",

	t.KeyXUnaryPlus:  " + ",
	t.KeyXUnaryMinus: " - ",
	t.KeyXUnaryNot:   " ! ",
	t.KeyXUnaryRef:   " & ",
	t.KeyXUnaryDeref: " * ",

	t.KeyXBinaryPlus:        " + ",
	t.KeyXBinaryMinus:       " - ",
	t.KeyXBinaryStar:        " * ",
	t.KeyXBinarySlash:       " / ",
	t.KeyXBinaryShiftL:      " << ",
	t.KeyXBinaryShiftR:      " >> ",
	t.KeyXBinaryAmp:         " & ",
	t.KeyXBinaryAmpHat:      " no_such_amp_hat_C_operator ",
	t.KeyXBinaryPipe:        " | ",
	t.KeyXBinaryHat:         " ^ ",
	t.KeyXBinaryPercent:     " % ",
	t.KeyXBinaryNotEq:       " != ",
	t.KeyXBinaryLessThan:    " < ",
	t.KeyXBinaryLessEq:      " <= ",
	t.KeyXBinaryEqEq:        " == ",
	t.KeyXBinaryGreaterEq:   " >= ",
	t.KeyXBinaryGreaterThan: " > ",
	t.KeyXBinaryAnd:         " && ",
	t.KeyXBinaryOr:          " || ",
	t.KeyXBinaryAs:          " no_such_as_C_operator ",
	t.KeyXBinaryTildePlus:   " + ",

	t.KeyXAssociativePlus: " + ",
	t.KeyXAssociativeStar: " * ",
	t.KeyXAssociativeAmp:  " & ",
	t.KeyXAssociativePipe: " | ",
	t.KeyXAssociativeHat:  " ^ ",
	t.KeyXAssociativeAnd:  " && ",
	t.KeyXAssociativeOr:   " || ",
}
