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
	if n.Operator() != t.IDOpenParen {
		return errNoSuchBuiltin
	}
	method := n.LHS().Expr()
	recv := method.LHS().Expr()
	recvTyp := recv.MType()

	switch recvTyp.Decorator() {
	case 0:
		// No-op.
	case t.IDPtr:
		// TODO: don't hard-code initialize.
		if method.Ident() != g.tm.ByName("initialize") {
			return errNoSuchBuiltin
		}
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
	case t.IDSlice:
		return g.writeBuiltinSlice(b, recv, method.Ident(), n.Args(), rp, depth)
	default:
		return errNoSuchBuiltin
	}

	qid := recvTyp.QID()
	if qid[0] != t.IDBase {
		return errNoSuchBuiltin
	}

	if qid[1].IsNumType() {
		return g.writeBuiltinNumType(b, recv, method.Ident(), n.Args(), rp, depth)
	} else {
		switch qid[1] {
		case t.IDIOReader:
			return g.writeBuiltinIOReader(b, recv, method.Ident(), n.Args(), rp, depth)
		case t.IDIOWriter:
			return g.writeBuiltinIOWriter(b, recv, method.Ident(), n.Args(), rp, depth, n.BoundsCheckOptimized())
		case t.IDStatus:
			return g.writeBuiltinStatus(b, recv, method.Ident(), n.Args(), rp, depth)
		}
	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinIO(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32) error {
	switch method {
	case t.IDAvailable:
		p0, p1 := "", ""
		// TODO: don't hard-code these.
		switch recv.Str(g.tm) {
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
		if p0 == "" {
			return fmt.Errorf(`TODO: cgen a "foo.available" expression`)
		}
		b.printf("((uint64_t)(%s - %s))", p0, p1)
		return nil

	case t.IDSet:
		typ := "reader"
		if len(args) == 1 {
			typ = "writer"
		}
		// TODO: don't hard-code v_w and u_w, and wptr vs rptr.
		b.printf("wuffs_base__io_%s__set(&v_w, &u_w, &b_wptr_w, &b_wend_w", typ)
		for _, o := range args {
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

func (g *gen) writeBuiltinIOReader(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32) error {
	// TODO: don't hard-code the recv being a_src.
	switch method {
	case t.IDSetLimit:
		b.printf("wuffs_base__io_reader__set_limit(&%ssrc, %srptr_src,", aPrefix, bPrefix)
		if err := g.writeExpr(b, args[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		// TODO: update the bPrefix variables?
		return nil

	case t.IDSetMark:
		b.printf("wuffs_base__io_reader__set_mark(&%ssrc, %srptr_src)", aPrefix, bPrefix)
		return nil

	case t.IDSinceMark:
		b.printf("((wuffs_base__slice_u8){ "+
			".ptr = %ssrc.private_impl.bounds[0], "+
			".len = (size_t)(%srptr_src - %ssrc.private_impl.bounds[0]), })",
			aPrefix, bPrefix, aPrefix)
		return nil
	}

	return g.writeBuiltinIO(b, recv, method, args, rp, depth)
}

// TODO: remove bcoHack.
func (g *gen) writeBuiltinIOWriter(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32, bcoHack bool) error {
	// TODO: don't hard-code the recv being a_dst.
	switch method {
	case t.IDCopyFromHistory32:
		bco := ""
		if bcoHack {
			bco = "__bco"
		}
		b.printf("wuffs_base__io_writer__copy_from_history32%s("+
			"&%swptr_dst, %sdst.private_impl.bounds[0] , %swend_dst",
			bco, bPrefix, aPrefix, bPrefix)
		for _, o := range args {
			b.writeb(',')
			if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil

	case t.IDCopyFromReader32:
		b.printf("wuffs_base__io_writer__copy_from_reader32(&%swptr_dst, %swend_dst",
			bPrefix, bPrefix)
		// TODO: don't assume that the first argument is "in.src".
		b.printf(", &%srptr_src, %srend_src,", bPrefix, bPrefix)
		a := args[1].Arg().Value()
		if err := g.writeExpr(b, a, rp, depth); err != nil {
			return err
		}
		b.writeb(')')
		return nil

	case t.IDCopyFromSlice:
		b.printf("wuffs_base__io_writer__copy_from_slice(&%swptr_dst, %swend_dst,", bPrefix, bPrefix)
		a := args[0].Arg().Value()
		if err := g.writeExpr(b, a, rp, depth); err != nil {
			return err
		}
		b.writeb(')')
		return nil

	case t.IDCopyFromSlice32:
		b.printf("wuffs_base__io_writer__copy_from_slice32("+
			"&%swptr_dst, %swend_dst", bPrefix, bPrefix)
		for _, o := range args {
			b.writeb(',')
			if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil

	case t.IDSetMark:
		// TODO: is a private_impl.bounds[0] the right representation? What
		// if the function is passed a (ptr io_writer) instead of a
		// (io_writer)? Do we still want to have that mark live outside of
		// the function scope?
		b.printf("wuffs_base__io_writer__set_mark(&%sdst, %swptr_dst)", aPrefix, bPrefix)
		return nil

	case t.IDSinceMark:
		b.printf("((wuffs_base__slice_u8){ "+
			".ptr = %sdst.private_impl.bounds[0], "+
			".len = (size_t)(%swptr_dst - %sdst.private_impl.bounds[0]), })",
			aPrefix, bPrefix, aPrefix)
		return nil
	}

	return g.writeBuiltinIO(b, recv, method, args, rp, depth)
}

func (g *gen) writeBuiltinNumType(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32) error {
	switch method {
	case t.IDLowBits:
		// "recv.low_bits(n:etc)" in C is "((recv) & ((1 << (n)) - 1))".
		b.writes("((")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writes(") & ((1 << (")
		if err := g.writeExpr(b, args[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")) - 1))")
		return nil

	case t.IDHighBits:
		// "recv.high_bits(n:etc)" in C is "((recv) >> (8*sizeof(recv) - (n)))".
		b.writes("((")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writes(") >> (")
		if sz, err := g.sizeof(recv.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes(" - (")
		if err := g.writeExpr(b, args[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")))")
		return nil

	case t.IDMax:
		b.writes("wuffs_base__u")
		if sz, err := g.sizeof(recv.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes("__max(")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writes(",")
		if err := g.writeExpr(b, args[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil

	case t.IDMin:
		b.writes("wuffs_base__u")
		if sz, err := g.sizeof(recv.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes("__min(")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writes(",")
		if err := g.writeExpr(b, args[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil
	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinSlice(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32) error {
	switch method {
	case t.IDCopyFromSlice:
		// TODO: don't assume that the slice is a slice of base.u8.
		b.writes("wuffs_base__slice_u8__copy_from_slice(")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writeb(',')
		if err := g.writeExpr(b, args[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")\n")
		return nil

	case t.IDLength:
		b.writes("((uint64_t)(")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writes(".len))")
		return nil

	case t.IDSuffix:
		// TODO: don't assume that the slice is a slice of base.u8.
		b.writes("wuffs_base__slice_u8__suffix(")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writeb(',')
		if err := g.writeExpr(b, args[0].Arg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil
	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinStatus(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32) error {
	b.writeb('(')
	if err := g.writeExpr(b, recv, rp, depth); err != nil {
		return err
	}
	switch method {
	case t.IDIsError:
		b.writes(" < 0")
	case t.IDIsOK:
		b.writes(" == 0")
	case t.IDIsSuspension:
		b.writes(" > 0")
	default:
		return fmt.Errorf("cgen: internal error for writeBuiltinStatus")
	}
	b.writeb(')')
	return nil
}
