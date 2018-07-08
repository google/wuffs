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
	method := n.LHS().AsExpr()
	recv := method.LHS().AsExpr()
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
			if err := g.writeExpr(b, o.AsArg().Value(), rp, depth); err != nil {
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
	case t.IDUndoByte:
		b.writes("(ioptr_src--, wuffs_base__return_empty_struct())")
		return nil

	case t.IDCanUndoByte:
		b.writes("(ioptr_src > iobounds0orig_src)")
		return nil

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

	case t.IDSkipFast:
		// Generate a two part expression using the comma operator: "(pointer
		// increment, return_empty_struct call)". The final part is a function
		// call (to a static inline function) instead of a struct literal, to
		// avoid a "expression result unused" compiler error.
		b.writes("(ioptr_src += ")
		if err := g.writeExpr(b, args[1].AsArg().Value(), rp, depth); err != nil {
			return err
		}
		b.writes(", wuffs_base__return_empty_struct())")
		return nil
	}

	if method >= peekMethodsBase {
		if m := method - peekMethodsBase; m < t.ID(len(peekMethods)) {
			if p := peekMethods[m]; p.n != 0 {
				b.printf("wuffs_base__load_u%d%ce(ioptr_src)", p.n, p.endianness)
				return nil
			}
		}
	}

	return g.writeBuiltinIO(b, recv, method, args, rp, depth)
}

// TODO: remove bcoHack.
func (g *gen) writeBuiltinIOWriter(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, rp replacementPolicy, depth uint32, bcoHack bool) error {
	// TODO: don't hard-code the recv being a_dst.
	switch method {
	case t.IDCopyNFromHistory:
		bco := ""
		if bcoHack {
			bco = "__bco"
		}
		b.printf("wuffs_base__io_writer__copy_n_from_history%s("+
			"&ioptr_dst, %sdst.private_impl.bounds[0], iobounds1_dst",
			bco, aPrefix)
		for _, o := range args {
			b.writeb(',')
			if err := g.writeExpr(b, o.AsArg().Value(), rp, depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil

	case t.IDCopyNFromReader:
		b.printf("wuffs_base__io_writer__copy_n_from_reader(&ioptr_dst, iobounds1_dst,")
		if err := g.writeExpr(b, args[0].AsArg().Value(), rp, depth); err != nil {
			return err
		}
		// TODO: don't assume that the last argument is "in.src".
		b.printf(", &ioptr_src, iobounds1_src)")
		return nil

	case t.IDCopyFromSlice:
		b.printf("wuffs_base__io_writer__copy_from_slice(&ioptr_dst, iobounds1_dst,")
		return g.writeArgs(b, args, rp, depth)

	case t.IDCopyNFromSlice:
		b.printf("wuffs_base__io_writer__copy_n_from_slice(&ioptr_dst, iobounds1_dst,")
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

	if method >= writeFastMethodsBase {
		if m := method - writeFastMethodsBase; m < t.ID(len(writeFastMethods)) {
			if p := writeFastMethods[m]; p.n != 0 {
				// Generate a three part expression using the comma operator:
				// "(store, pointer increment, return_empty_struct call)". The
				// final part is a function call (to a static inline function)
				// instead of a struct literal, to avoid a "expression result
				// unused" compiler error.
				b.printf("(wuffs_base__store_u%d%ce(ioptr_dst,", p.n, p.endianness)
				if err := g.writeExpr(b, args[0].AsArg().Value(), rp, depth); err != nil {
					return err
				}
				b.printf("), ioptr_dst += %d, wuffs_base__return_empty_struct())", p.n/8)
				return nil
			}
		}
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
		if err := g.writeExpr(b, args[0].AsArg().Value(), rp, depth); err != nil {
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
		if err := g.writeExpr(b, args[0].AsArg().Value(), rp, depth); err != nil {
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
		if err := g.writeExpr(b, args[0].AsArg().Value(), rp, depth); err != nil {
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
		if err := g.writeExpr(b, args[0].AsArg().Value(), rp, depth); err != nil {
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
		if err := g.writeExpr(b, o.AsArg().Value(), rp, depth); err != nil {
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
	method := n.LHS().AsExpr()
	recv := method.LHS().AsExpr()
	recvTyp := recv.MType()
	if !recvTyp.IsIOType() {
		return errNoSuchBuiltin
	}

	if recvTyp.QID()[1] == t.IDIOReader {
		switch method.Ident() {
		case t.IDReadU8:
			if g.currFunk.tempW > maxTemp {
				return fmt.Errorf("too many temporary variables required")
			}
			temp := g.currFunk.tempW
			g.currFunk.tempW++

			b.printf("if (WUFFS_BASE__UNLIKELY(ioptr_src == iobounds1_src)) { goto short_read_src; }")
			g.currFunk.shortReads = append(g.currFunk.shortReads, "src")

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
		case t.IDReadU24BE:
			return g.writeReadUXX(b, n, "src", 24, "be")
		case t.IDReadU24LE:
			return g.writeReadUXX(b, n, "src", 24, "le")
		case t.IDReadU32BE:
			return g.writeReadUXX(b, n, "src", 32, "be")
		case t.IDReadU32LE:
			return g.writeReadUXX(b, n, "src", 32, "le")
		case t.IDReadU40BE:
			return g.writeReadUXX(b, n, "src", 40, "be")
		case t.IDReadU40LE:
			return g.writeReadUXX(b, n, "src", 40, "le")
		case t.IDReadU48BE:
			return g.writeReadUXX(b, n, "src", 48, "be")
		case t.IDReadU48LE:
			return g.writeReadUXX(b, n, "src", 48, "le")
		case t.IDReadU56BE:
			return g.writeReadUXX(b, n, "src", 56, "be")
		case t.IDReadU56LE:
			return g.writeReadUXX(b, n, "src", 56, "le")
		case t.IDReadU64BE:
			return g.writeReadUXX(b, n, "src", 64, "be")
		case t.IDReadU64LE:
			return g.writeReadUXX(b, n, "src", 64, "le")

		case t.IDSkip:
			g.currFunk.usesScratch = true
			// TODO: don't hard-code [0], and allow recursive coroutines.
			scratchName := fmt.Sprintf("self->private_impl.%s%s[0].scratch",
				cPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

			b.printf("%s = ", scratchName)
			x := n.Args()[0].AsArg().Value()
			if err := g.writeExpr(b, x, replaceCallSuspendibles, depth); err != nil {
				return err
			}
			b.writes(";\n")

			// TODO: the CSP prior to this is probably unnecessary.
			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}

			b.printf("if (%s > ((uint64_t)(iobounds1_src - ioptr_src))) {\n", scratchName)
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
			b.writes("if (ioptr_dst == iobounds1_dst) {\n" +
				"status = WUFFS_BASE__SUSPENSION_SHORT_WRITE; goto suspend; }\n" +
				"*ioptr_dst++ = ")
			x := n.Args()[0].AsArg().Value()
			if err := g.writeExpr(b, x, replaceCallSuspendibles, depth); err != nil {
				return err
			}
			b.writes(";\n")
			return nil
		}
	}
	return errNoSuchBuiltin
}

const peekMethodsBase = t.IDPeekU8

var peekMethods = [...]struct {
	n          uint8
	endianness uint8
}{
	t.IDPeekU8 - peekMethodsBase:    {8, 'b'},
	t.IDPeekU16BE - peekMethodsBase: {16, 'b'},
	t.IDPeekU16LE - peekMethodsBase: {16, 'l'},
	t.IDPeekU24BE - peekMethodsBase: {24, 'b'},
	t.IDPeekU24LE - peekMethodsBase: {24, 'l'},
	t.IDPeekU32BE - peekMethodsBase: {32, 'b'},
	t.IDPeekU32LE - peekMethodsBase: {32, 'l'},
	t.IDPeekU40BE - peekMethodsBase: {40, 'b'},
	t.IDPeekU40LE - peekMethodsBase: {40, 'l'},
	t.IDPeekU48BE - peekMethodsBase: {48, 'b'},
	t.IDPeekU48LE - peekMethodsBase: {48, 'l'},
	t.IDPeekU56BE - peekMethodsBase: {56, 'b'},
	t.IDPeekU56LE - peekMethodsBase: {56, 'l'},
	t.IDPeekU64BE - peekMethodsBase: {64, 'b'},
	t.IDPeekU64LE - peekMethodsBase: {64, 'l'},
}

const writeFastMethodsBase = t.IDWriteFastU8

var writeFastMethods = [...]struct {
	n          uint8
	endianness uint8
}{
	t.IDWriteFastU8 - writeFastMethodsBase:    {8, 'b'},
	t.IDWriteFastU16BE - writeFastMethodsBase: {16, 'b'},
	t.IDWriteFastU16LE - writeFastMethodsBase: {16, 'l'},
	t.IDWriteFastU24BE - writeFastMethodsBase: {24, 'b'},
	t.IDWriteFastU24LE - writeFastMethodsBase: {24, 'l'},
	t.IDWriteFastU32BE - writeFastMethodsBase: {32, 'b'},
	t.IDWriteFastU32LE - writeFastMethodsBase: {32, 'l'},
	t.IDWriteFastU40BE - writeFastMethodsBase: {40, 'b'},
	t.IDWriteFastU40LE - writeFastMethodsBase: {40, 'l'},
	t.IDWriteFastU48BE - writeFastMethodsBase: {48, 'b'},
	t.IDWriteFastU48LE - writeFastMethodsBase: {48, 'l'},
	t.IDWriteFastU56BE - writeFastMethodsBase: {56, 'b'},
	t.IDWriteFastU56LE - writeFastMethodsBase: {56, 'l'},
	t.IDWriteFastU64BE - writeFastMethodsBase: {64, 'b'},
	t.IDWriteFastU64LE - writeFastMethodsBase: {64, 'l'},
}
