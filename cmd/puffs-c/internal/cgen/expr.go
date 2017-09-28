// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

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
		if isThatMethod(g.tm, n, t.KeyIsSuspension, 0) {
			if pp == parenthesesMandatory {
				b.writeb('(')
			}
			x := n.LHS().Expr().LHS().Expr()
			if err := g.writeExpr(b, x, rp, parenthesesMandatory, depth); err != nil {
				return err
			}
			// TODO: write < or == instead of > for KeyIsError or KeyIsOK.
			b.writes(" > 0")
			if pp == parenthesesMandatory {
				b.writeb(')')
			}
			return nil
		}
		if isThatMethod(g.tm, n, t.KeySuffix, 1) {
			// TODO: don't assume that the slice is a slice of u8.
			b.writes("puffs_base_slice_u8_suffix(")
			x := n.LHS().Expr().LHS().Expr()
			if err := g.writeExpr(b, x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(", ")
			if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(")")
			return nil
		}
		if isInDst(g.tm, n, t.KeyMark, 0) {
			if pp == parenthesesMandatory {
				b.writeb('(')
			}
			b.printf("(puffs_base_buf1_mark){.ptr= %swptr_dst}", bPrefix)
			if pp == parenthesesMandatory {
				b.writeb(')')
			}
			return nil
		}
		if isInDst(g.tm, n, t.KeySlice, 2) {
			b.printf("puffs_base_make_slice_u8_from_ptrs(%sdst.buf ? %sdst.buf->ptr : NULL,", aPrefix, aPrefix)
			for _, o := range n.Args() {
				if err := g.writeExpr(b, o.Arg().Value(), rp, parenthesesOptional, depth); err != nil {
					return err
				}
				b.writes(".ptr,")
			}
			b.printf("%sdst.buf ? %swptr_dst : NULL)", aPrefix, bPrefix)
			return nil
		}
		if isThatMethod(g.tm, n, t.KeyCopyFrom, 1) {
			b.writes("puffs_base_slice_u8_copy_from(")
			receiver := n.LHS().Expr().LHS().Expr()
			if err := g.writeExpr(b, receiver, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writeb(',')
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, a, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
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
			x := n.LHS().Expr().LHS().Expr()
			if err := g.writeExpr(b, x, rp, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writes(".len)")
			if pp == parenthesesMandatory {
				b.writeb(')')
			}
			return nil
		}
		// TODO.

	case t.KeyOpenBracket:
		// n is an index.
		if err := g.writeExpr(b, n.LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
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
		if lhs.MType().Decorator().Key() == t.KeyOpenBracket {
			// Slice of an array.
			// TODO: don't assume that the slice is a slice of u8.
			length := lhs.MType().ArrayLength().ConstValue().String()
			b.writes("puffs_base_make_slice_u8_from_ints(")
			if err := g.writeExpr(b, lhs, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writeb(',')
			if mhs == nil {
				b.writes("0")
			} else if err := g.writeExpr(b, mhs, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writeb(',')
			if rhs == nil {
				b.writes(length)
			} else if err := g.writeExpr(b, rhs, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writeb(',')
			b.writes(length)
			b.writes(")")
		} else {
			// Slice of a slice.
			if mhs != nil || rhs != nil {
				return fmt.Errorf("TODO: writeExprOther for a non-trivial slice of a slice")
			}
			if err := g.writeExpr(b, lhs, rp, parenthesesMandatory, depth); err != nil {
				return err
			}
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
		b.writes("(puffs_base_reader1) {")
		b.writes(".buf = a_src.buf,")
		b.writes(".limit = (puffs_base_limit1) {")
		b.printf(".ptr_to_len = &%s%s,", lPrefix, g.currFunk.limitVarName)
		b.writes(".next = &a_src.limit,")
		b.writes("}}")
		g.currFunk.limitVarName = ""
		return nil
	}
	return fmt.Errorf("unrecognized token.Key (0x%X) for writeExprOther", n.ID0().Key())
}

func (g *gen) writeExprUnaryOp(b *buffer, n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	// TODO.
	return nil
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
			b.writes("puffs_base_slice_u8")
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
	t.KeyI8:       "int8_t",
	t.KeyI16:      "int16_t",
	t.KeyI32:      "int32_t",
	t.KeyI64:      "int64_t",
	t.KeyU8:       "uint8_t",
	t.KeyU16:      "uint16_t",
	t.KeyU32:      "uint32_t",
	t.KeyU64:      "uint64_t",
	t.KeyUsize:    "size_t",
	t.KeyBool:     "bool",
	t.KeyBuf1:     "puffs_base_buf1",
	t.KeyBuf1Mark: "puffs_base_buf1_mark",
	t.KeyReader1:  "puffs_base_reader1",
	t.KeyWriter1:  "puffs_base_writer1",
	t.KeyBuf2:     "puffs_base_buf2",
}

var cOpNames = [256]string{
	t.KeyEq:        " = ",
	t.KeyPlusEq:    " += ",
	t.KeyMinusEq:   " -= ",
	t.KeyStarEq:    " *= ",
	t.KeySlashEq:   " /= ",
	t.KeyShiftLEq:  " <<= ",
	t.KeyShiftREq:  " >>= ",
	t.KeyAmpEq:     " &= ",
	t.KeyAmpHatEq:  " no_such_amp_hat_C_operator ",
	t.KeyPipeEq:    " |= ",
	t.KeyHatEq:     " ^= ",
	t.KeyPercentEq: " %= ",

	t.KeyXUnaryPlus:  "+",
	t.KeyXUnaryMinus: "-",
	t.KeyXUnaryNot:   "!",

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

	t.KeyXAssociativePlus: " + ",
	t.KeyXAssociativeStar: " * ",
	t.KeyXAssociativeAmp:  " & ",
	t.KeyXAssociativePipe: " | ",
	t.KeyXAssociativeHat:  " ^ ",
	t.KeyXAssociativeAnd:  " && ",
	t.KeyXAssociativeOr:   " || ",
}
