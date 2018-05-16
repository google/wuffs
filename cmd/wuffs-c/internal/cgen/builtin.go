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
	"errors"
	"fmt"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var errNoSuchBuiltin = errors.New("cgen: internal error: no such built-in")

func (g *gen) writeBuiltin(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	if isThatMethod(g.tm, n, t.IDLowBits, 1) {
		// "x.low_bits(n:etc)" in C is "((x) & ((1 << (n)) - 1))".
		x := n.LHS().Expr().LHS().Expr()
		b.writes("((")
		if err := g.writeExpr(b, x, rp, depth); err != nil {
			return err
		}
		b.writes(") & ((1 << (")
		if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")) - 1))")
		return nil
	}
	if isThatMethod(g.tm, n, t.IDHighBits, 1) {
		// "x.high_bits(n:etc)" in C is "((x) >> (8*sizeof(x) - (n)))".
		x := n.LHS().Expr().LHS().Expr()
		b.writes("((")
		if err := g.writeExpr(b, x, rp, depth); err != nil {
			return err
		}
		b.writes(") >> (")
		if sz, err := g.sizeof(x.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes(" - (")
		if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")))")
		return nil
	}
	if isThatMethod(g.tm, n, t.IDMin, 1) {
		x := n.LHS().Expr().LHS().Expr()
		b.writes("wuffs_base__u")
		if sz, err := g.sizeof(x.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes("__min(")
		if err := g.writeExpr(b, x, rp, depth); err != nil {
			return err
		}
		b.writes(",")
		if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil
	}
	if isThatMethod(g.tm, n, t.IDMax, 1) {
		x := n.LHS().Expr().LHS().Expr()
		b.writes("wuffs_base__u")
		if sz, err := g.sizeof(x.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes("__max(")
		if err := g.writeExpr(b, x, rp, depth); err != nil {
			return err
		}
		b.writes(",")
		if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil
	}
	if isThatMethod(g.tm, n, t.IDIsError, 0) || isThatMethod(g.tm, n, t.IDIsOK, 0) ||
		isThatMethod(g.tm, n, t.IDIsSuspension, 0) {
		b.writeb('(')
		x := n.LHS().Expr().LHS().Expr()
		if err := g.writeExpr(b, x, rp, depth); err != nil {
			return err
		}
		switch key := n.LHS().Expr().Ident(); key {
		case t.IDIsError:
			b.writes(" < 0")
		case t.IDIsOK:
			b.writes(" == 0")
		case t.IDIsSuspension:
			b.writes(" > 0")
		default:
			return fmt.Errorf("unrecognized token (0x%X) for writeExprOther's IsXxx", key)
		}
		b.writeb(')')
		return nil
	}
	if isThatMethod(g.tm, n, t.IDSuffix, 1) {
		// TODO: don't assume that the slice is a slice of base.u8.
		b.writes("wuffs_base__slice_u8__suffix(")
		x := n.LHS().Expr().LHS().Expr()
		if err := g.writeExpr(b, x, rp, depth); err != nil {
			return err
		}
		b.writeb(',')
		if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil
	}
	if isInSrc(g.tm, n, t.IDSetLimit, 1) {
		b.printf("wuffs_base__io_reader__set_limit(&%ssrc, %srptr_src,", aPrefix, bPrefix)
		if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		// TODO: update the bPrefix variables?
		return nil
	}
	if isInSrc(g.tm, n, t.IDSetMark, 0) {
		b.printf("wuffs_base__io_reader__set_mark(&%ssrc, %srptr_src)", aPrefix, bPrefix)
		return nil
	}
	if isInSrc(g.tm, n, t.IDSinceMark, 0) {
		b.printf("((wuffs_base__slice_u8){ "+
			".ptr = %ssrc.private_impl.bounds[0], "+
			".len = (size_t)(%srptr_src - %ssrc.private_impl.bounds[0]), })",
			aPrefix, bPrefix, aPrefix)
		return nil
	}
	if isInDst(g.tm, n, t.IDSetMark, 0) {
		// TODO: is a private_impl.bounds[0] the right representation? What
		// if the function is passed a (ptr io_writer) instead of a
		// (io_writer)? Do we still want to have that mark live outside of
		// the function scope?
		b.printf("wuffs_base__io_writer__set_mark(&%sdst, %swptr_dst)", aPrefix, bPrefix)
		return nil
	}
	if isInDst(g.tm, n, t.IDSinceMark, 0) {
		b.printf("((wuffs_base__slice_u8){ "+
			".ptr = %sdst.private_impl.bounds[0], "+
			".len = (size_t)(%swptr_dst - %sdst.private_impl.bounds[0]), })",
			aPrefix, bPrefix, aPrefix)
		return nil
	}
	if isInDst(g.tm, n, t.IDCopyFromReader32, 2) {
		b.printf("wuffs_base__io_writer__copy_from_reader32(&%swptr_dst, %swend_dst",
			bPrefix, bPrefix)
		// TODO: don't assume that the first argument is "in.src".
		b.printf(", &%srptr_src, %srend_src,", bPrefix, bPrefix)
		a := n.Args()[1].Arg().Value()
		if err := g.writeExpr(b, a, rp, depth); err != nil {
			return err
		}
		b.writeb(')')
		return nil
	}
	if isInDst(g.tm, n, t.IDCopyFromHistory32, 2) {
		bco := ""
		if n.BoundsCheckOptimized() {
			bco = "__bco"
		}
		b.printf("wuffs_base__io_writer__copy_from_history32%s("+
			"&%swptr_dst, %sdst.private_impl.bounds[0] , %swend_dst",
			bco, bPrefix, aPrefix, bPrefix)
		for _, o := range n.Args() {
			b.writeb(',')
			if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil
	}
	if isInDst(g.tm, n, t.IDCopyFromSlice32, 2) {
		b.printf("wuffs_base__io_writer__copy_from_slice32("+
			"&%swptr_dst, %swend_dst", bPrefix, bPrefix)
		for _, o := range n.Args() {
			b.writeb(',')
			if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil
	}
	if isInDst(g.tm, n, t.IDCopyFromSlice, 1) {
		b.printf("wuffs_base__io_writer__copy_from_slice(&%swptr_dst, %swend_dst,", bPrefix, bPrefix)
		a := n.Args()[0].Arg().Value()
		if err := g.writeExpr(b, a, rp, depth); err != nil {
			return err
		}
		b.writeb(')')
		return nil
	}
	if isThatMethod(g.tm, n, t.IDCopyFromSlice, 1) {
		b.writes("wuffs_base__slice_u8__copy_from_slice(")
		receiver := n.LHS().Expr().LHS().Expr()
		if err := g.writeExpr(b, receiver, rp, depth); err != nil {
			return err
		}
		b.writeb(',')
		a := n.Args()[0].Arg().Value()
		if err := g.writeExpr(b, a, rp, depth); err != nil {
			return err
		}
		b.writes(")\n")
		return nil
	}
	if isThatMethod(g.tm, n, t.IDLength, 0) {
		b.writes("((uint64_t)(")
		if err := g.writeExpr(b, n.LHS().Expr().LHS().Expr(), rp, depth); err != nil {
			return err
		}
		b.writes(".len))")
		return nil
	}
	if isThatMethod(g.tm, n, t.IDAvailable, 0) {
		p0, p1 := "", ""
		if o := n.LHS().Expr().LHS().Expr(); o != nil {
			// TODO: don't hard-code these.
			switch o.Str(g.tm) {
			case "in.dst":
				p0 = bPrefix + "wend_dst"
				p1 = bPrefix + "wptr_dst"
			case "in.src":
				p0 = bPrefix + "rend_src"
				p1 = bPrefix + "rptr_src"
			case "w":
				p0 = bPrefix + "wend_w"
				p1 = bPrefix + "wptr_w"
			}
		}
		if p0 == "" {
			return fmt.Errorf(`TODO: cgen a "foo.available" expression`)
		}
		b.printf("((uint64_t)(%s - %s))", p0, p1)
		return nil
	}
	if isThatMethod(g.tm, n, g.tm.ByName("initialize"), 5) {
		// TODO: don't hard-code a_dst.
		b.printf("wuffs_base__image_config__initialize(a_dst")
		for _, o := range n.Args() {
			b.writeb(',')
			if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.printf(")")
		return nil
	}
	if isThatMethod(g.tm, n, t.IDSet, 1) || isThatMethod(g.tm, n, t.IDSet, 2) {
		typ := "reader"
		if len(n.Args()) == 1 {
			typ = "writer"
		}
		// TODO: don't hard-code v_w and u_w, and wptr vs rptr.
		b.printf("wuffs_base__io_%s__set(&v_w, &u_w, &b_wptr_w, &b_wend_w", typ)
		for _, o := range n.Args() {
			b.writeb(',')
			if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.writes(")")
		return nil
	}
	return errNoSuchBuiltin
}
