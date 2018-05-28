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

func (g *gen) writeBuiltinCall(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
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
	case t.IDTable:
		return g.writeBuiltinTable(b, recv, method.Ident(), n.Args(), rp, depth)
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
			p0 = "iobounds1_dst"
			p1 = "ioptr_dst"
		case "in.src":
			p0 = "iobounds1_src"
			p1 = "ioptr_src"
		case "w":
			p0 = "iobounds1_w"
			p1 = "ioptr_w"
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
		// TODO: don't hard-code v_w and u_w.
		b.printf("wuffs_base__io_%s__set(&v_w, &u_w, &ioptr_w, &iobounds1_w,", typ)
		return g.writeArgs(b, args, rp, depth)

	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinIOReader(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32) error {
	// TODO: don't hard-code the recv being a_src.
	switch method {
	case t.IDSetLimit:
		b.printf("wuffs_base__io_reader__set_limit(&%ssrc, ioptr_src,", aPrefix)
		// TODO: update the ioptr variables?
		return g.writeArgs(b, args, rp, depth)

	case t.IDSetMark:
		b.printf("wuffs_base__io_reader__set_mark(&%ssrc, ioptr_src)", aPrefix)
		return nil

	case t.IDSinceMark:
		b.printf("((wuffs_base__slice_u8){ "+
			".ptr = %ssrc.private_impl.bounds[0], "+
			".len = (size_t)(ioptr_src - %ssrc.private_impl.bounds[0]), })",
			aPrefix, aPrefix)
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
			"&ioptr_dst, %sdst.private_impl.bounds[0], iobounds1_dst",
			bco, aPrefix)
		for _, o := range args {
			b.writeb(',')
			if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil

	case t.IDCopyFromReader32:
		b.printf("wuffs_base__io_writer__copy_from_reader32(&ioptr_dst, iobounds1_dst,")
		// TODO: don't assume that the first argument is "in.src".
		b.printf("&ioptr_src, iobounds1_src,")
		return g.writeArgs(b, args[1:], rp, depth)

	case t.IDCopyFromSlice:
		b.printf("wuffs_base__io_writer__copy_from_slice(&ioptr_dst, iobounds1_dst,")
		return g.writeArgs(b, args, rp, depth)

	case t.IDCopyFromSlice32:
		b.printf("wuffs_base__io_writer__copy_from_slice32(&ioptr_dst, iobounds1_dst,")
		return g.writeArgs(b, args, rp, depth)

	case t.IDSetMark:
		// TODO: is a private_impl.bounds[0] the right representation? What
		// if the function is passed a (ptr io_writer) instead of a
		// (io_writer)? Do we still want to have that mark live outside of
		// the function scope?
		b.printf("wuffs_base__io_writer__set_mark(&%sdst, ioptr_dst)", aPrefix)
		return nil

	case t.IDSinceMark:
		b.printf("((wuffs_base__slice_u8){ "+
			".ptr = %sdst.private_impl.bounds[0], "+
			".len = (size_t)(ioptr_dst - %sdst.private_impl.bounds[0]), })",
			aPrefix, aPrefix)
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
		return g.writeArgs(b, args, rp, depth)

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
		return g.writeArgs(b, args, rp, depth)
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

func (g *gen) writeBuiltinTable(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32) error {
	field := ""

	switch method {
	case t.IDHeight:
		field = "height"
	case t.IDStride:
		field = "stride"
	case t.IDWidth:
		field = "width"

	case t.IDRow:
		// TODO: don't assume that the table is a table of base.u8.
		b.writes("wuffs_base__table_u8__row(")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.writeb(',')
		return g.writeArgs(b, args, rp, depth)
	}

	if field != "" {
		b.writes("((uint64_t)(")
		if err := g.writeExpr(b, recv, rp, depth); err != nil {
			return err
		}
		b.printf(".%s))", field)
		return nil
	}

	return errNoSuchBuiltin
}

func (g *gen) writeArgs(b *buffer, args []*a.Node, rp replacementPolicy, depth uint32) error {
	for i, o := range args {
		if i > 0 {
			b.writeb(',')
		}
		if err := g.writeExpr(b, o.Arg().Value(), rp, depth); err != nil {
			return err
		}
	}
	b.writes(")")
	return nil
}

func (g *gen) writeBuiltinCallSuspendibles(b *buffer, n *a.Expr, depth uint32) error {
	// TODO: also handle (or reject??) t.IDTry.
	if n.Operator() != t.IDOpenParen {
		return errNoSuchBuiltin
	}
	method := n.LHS().Expr()
	recv := method.LHS().Expr()
	recvTyp := recv.MType()
	if !recvTyp.IsIOType() {
		return errNoSuchBuiltin
	}

	if recvTyp.QID()[1] == t.IDIOReader {
		switch method.Ident() {
		case t.IDUnreadU8:
			b.printf("if (ioptr_src == iobounds0orig_src) { status = %sERROR_INVALID_I_O_OPERATION;",
				g.PKGPREFIX)
			b.writes("goto exit;")
			b.writes("}\n")
			b.printf("ioptr_src--;\n")
			return nil

		case t.IDReadU8:
			if g.currFunk.tempW > maxTemp {
				return fmt.Errorf("too many temporary variables required")
			}
			temp := g.currFunk.tempW
			g.currFunk.tempW++

			if !n.ProvenNotToSuspend() {
				b.printf("if (WUFFS_BASE__UNLIKELY(ioptr_src == iobounds1_src)) { goto short_read_src; }")
				g.currFunk.shortReads = append(g.currFunk.shortReads, "src")
			}

			// TODO: watch for passing an array type to writeCTypeName? In C, an
			// array type can decay into a pointer.
			if err := g.writeCTypeName(b, n.MType(), tPrefix, fmt.Sprint(temp)); err != nil {
				return err
			}
			b.printf(" = *ioptr_src++;\n")
			return nil

		case t.IDReadU16BE:
			return g.writeReadUXX(b, n, "src", 16, "be")
		case t.IDReadU16LE:
			return g.writeReadUXX(b, n, "src", 16, "le")
		case t.IDReadU32BE:
			return g.writeReadUXX(b, n, "src", 32, "be")
		case t.IDReadU32LE:
			return g.writeReadUXX(b, n, "src", 32, "le")

		case t.IDSkip32:
			g.currFunk.usesScratch = true
			// TODO: don't hard-code [0], and allow recursive coroutines.
			scratchName := fmt.Sprintf("self->private_impl.%s%s[0].scratch",
				cPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

			b.printf("%s = ", scratchName)
			x := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, x, replaceCallSuspendibles, depth); err != nil {
				return err
			}
			b.writes(";\n")

			// TODO: the CSP prior to this is probably unnecessary.
			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}

			b.printf("if (%s > iobounds1_src - ioptr_src) {\n", scratchName)
			b.printf("%s -= iobounds1_src - ioptr_src;\n", scratchName)
			b.printf("ioptr_src = iobounds1_src;\n")

			b.writes("goto short_read_src; }\n")
			g.currFunk.shortReads = append(g.currFunk.shortReads, "src")
			b.printf("ioptr_src += %s;\n", scratchName)
			return nil
		}

	} else {
		switch method.Ident() {
		case t.IDWriteU8:
			if !n.ProvenNotToSuspend() {
				b.printf("if (ioptr_dst == iobounds1_dst) { status = %sSUSPENSION_SHORT_WRITE;",
					g.PKGPREFIX)
				b.writes("goto suspend;")
				b.writes("}\n")
			}

			b.printf("*ioptr_dst++ = ")
			x := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, x, replaceCallSuspendibles, depth); err != nil {
				return err
			}
			b.writes(";\n")
			return nil
		}
	}
	return errNoSuchBuiltin
}
