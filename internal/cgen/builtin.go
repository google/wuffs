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
	"math/big"
	"strings"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var (
	errNoSuchBuiltin             = errors.New("cgen: internal error: no such built-in")
	errOptimizationNotApplicable = errors.New("cgen: internal error: optimization not applicable")
)

func (g *gen) writeBuiltinCall(b *buffer, n *a.Expr, depth uint32) error {
	if n.Operator() != t.IDOpenParen {
		return errNoSuchBuiltin
	}
	method := n.LHS().AsExpr()
	recv := method.LHS().AsExpr()
	recvTyp := recv.MType()

	switch recvTyp.Decorator() {
	case 0:
		// No-op.

	case t.IDNptr, t.IDPtr:
		u64ToFlicksIndex := -1

		// TODO: don't hard-code these, or a_dst.
		switch meth := method.Ident(); {
		case (meth == t.IDSet) && (recvTyp.Inner().QID() == t.QID{t.IDBase, t.IDImageConfig}):
			b.writes("wuffs_base__image_config__set(a_dst")
		case (meth == t.IDUpdate) && (recvTyp.Inner().QID() == t.QID{t.IDBase, t.IDFrameConfig}):
			b.writes("wuffs_base__frame_config__update(a_dst")
			u64ToFlicksIndex = 1
		default:
			return errNoSuchBuiltin
		}

		for i, o := range n.Args() {
			b.writeb(',')
			if i == u64ToFlicksIndex {
				if o.AsArg().Name().Str(g.tm) != "duration" {
					return errors.New("cgen: internal error: inconsistent frame_config.update argument")
				}
				b.writes("((wuffs_base__flicks)(")
			}
			if err := g.writeExpr(b, o.AsArg().Value(), depth); err != nil {
				return err
			}
			if i == u64ToFlicksIndex {
				b.writes("))")
			}
		}
		b.writeb(')')
		return nil

	case t.IDSlice:
		return g.writeBuiltinSlice(b, recv, method.Ident(), n.Args(), depth)
	case t.IDTable:
		return g.writeBuiltinTable(b, recv, method.Ident(), n.Args(), depth)
	default:
		return errNoSuchBuiltin
	}

	qid := recvTyp.QID()
	if qid[0] != t.IDBase {
		return errNoSuchBuiltin
	}

	if qid[1].IsNumType() {
		return g.writeBuiltinNumType(b, recv, method.Ident(), n.Args(), depth)
	} else {
		switch qid[1] {
		case t.IDIOReader:
			return g.writeBuiltinIOReader(b, recv, method.Ident(), n.Args(), depth)
		case t.IDIOWriter:
			return g.writeBuiltinIOWriter(b, recv, method.Ident(), n.Args(), depth)
		case t.IDUtility:
			switch method.Ident() {
			case t.IDEmptyIOReader, t.IDEmptyIOWriter:
				if !g.currFunk.usesEmptyIOBuffer {
					g.currFunk.usesEmptyIOBuffer = true
					g.currFunk.bPrologue.writes("wuffs_base__io_buffer empty_io_buffer = " +
						"wuffs_base__empty_io_buffer();\n\n")
				}
				b.writes("&empty_io_buffer")
				return nil
			}
		}
	}
	return errNoSuchBuiltin
}

func (g *gen) ioRecvName(recv *a.Expr) (string, error) {
	switch recv.Operator() {
	case 0:
		return vPrefix + recv.Ident().Str(g.tm), nil
	case t.IDDot:
		if lhs := recv.LHS().AsExpr(); lhs.Operator() == 0 && lhs.Ident() == t.IDArgs {
			return aPrefix + recv.Ident().Str(g.tm), nil
		}
	}
	return "", fmt.Errorf("ioRecvName: cannot cgen a %q expression", recv.Str(g.tm))
}

func (g *gen) writeBuiltinIO(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	switch method {
	case t.IDAvailable:
		name, err := g.ioRecvName(recv)
		if err != nil {
			return err
		}
		b.printf("((uint64_t)(%s%s - %s%s))", io2Prefix, name, iopPrefix, name)
		return nil
	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinIOReader(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	name, err := g.ioRecvName(recv)
	if err != nil {
		return err
	}

	switch method {
	case t.IDUndoByte:
		b.printf("(%s%s--, wuffs_base__make_empty_struct())", iopPrefix, name)
		return nil

	case t.IDCanUndoByte:
		b.printf("(%s%s > %s%s)", iopPrefix, name, io1Prefix, name)
		return nil

	case t.IDCountSince:
		b.printf("wuffs_base__io__count_since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)))", iopPrefix, name, io0Prefix, name)
		return nil

	case t.IDMark:
		b.printf("((uint64_t)(%s%s - %s%s))", iopPrefix, name, io0Prefix, name)
		return nil

	case t.IDPosition:
		b.printf("wuffs_base__u64__sat_add(%s->meta.pos, ((uint64_t)(%s%s - %s%s)))",
			name, iopPrefix, name, io0Prefix, name)
		return nil

	case t.IDSince:
		b.printf("wuffs_base__io__since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)), %s%s)",
			iopPrefix, name, io0Prefix, name, io0Prefix, name)
		return nil

	case t.IDSkipFast:
		// Generate a two part expression using the comma operator: "(pointer
		// increment, return_empty_struct call)". The final part is a function
		// call (to a static inline function) instead of a struct literal, to
		// avoid a "expression result unused" compiler error.
		b.printf("(%s%s += ", iopPrefix, name)
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
			return err
		}
		b.writes(", wuffs_base__make_empty_struct())")
		return nil

	case t.IDTake:
		b.printf("wuffs_base__io_reader__take(&%s%s, %s%s,", iopPrefix, name, io2Prefix, name)
		return g.writeArgs(b, args, depth)
	}

	if method >= peekMethodsBase {
		if m := method - peekMethodsBase; m < t.ID(len(peekMethods)) {
			if p := peekMethods[m]; p.n != 0 {
				if p.size != p.n {
					b.printf("((uint%d_t)(", p.size)
				}
				b.printf("wuffs_base__load_u%d%ce(%s%s)", p.n, p.endianness, iopPrefix, name)
				if p.size != p.n {
					b.writes("))")
				}
				return nil
			}
		}
	}

	return g.writeBuiltinIO(b, recv, method, args, depth)
}

func (g *gen) writeBuiltinIOWriter(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	name, err := g.ioRecvName(recv)
	if err != nil {
		return err
	}

	switch method {
	case t.IDCopyNFromHistory, t.IDCopyNFromHistoryFast:
		suffix := ""
		if method == t.IDCopyNFromHistoryFast {
			suffix = "_fast"
		}
		b.printf("wuffs_base__io_writer__copy_n_from_history%s(&%s%s, %s%s, %s%s",
			suffix, iopPrefix, name, io0Prefix, name, io2Prefix, name)
		for _, o := range args {
			b.writeb(',')
			if err := g.writeExpr(b, o.AsArg().Value(), depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil

	case t.IDCopyNFromReader:
		readerName, err := g.ioRecvName(args[1].AsArg().Value())
		if err != nil {
			return err
		}

		b.printf("wuffs_base__io_writer__copy_n_from_reader(&%s%s, %s%s,",
			iopPrefix, name, io2Prefix, name)
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
			return err
		}
		b.printf(", &%s%s, %s%s)", iopPrefix, readerName, io2Prefix, readerName)
		return nil

	case t.IDCopyFromSlice:
		b.printf("wuffs_base__io_writer__copy_from_slice(&%s%s, %s%s,",
			iopPrefix, name, io2Prefix, name)
		return g.writeArgs(b, args, depth)

	case t.IDCopyNFromSlice:
		b.printf("wuffs_base__io_writer__copy_n_from_slice(&%s%s, %s%s,",
			iopPrefix, name, io2Prefix, name)
		return g.writeArgs(b, args, depth)

	case t.IDCountSince:
		b.printf("wuffs_base__io__count_since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)))", iopPrefix, name, io0Prefix, name)
		return nil

	case t.IDHistoryAvailable, t.IDMark:
		b.printf("((uint64_t)(%s%s - %s%s))", iopPrefix, name, io0Prefix, name)
		return nil

	case t.IDPosition:
		b.printf("wuffs_base__u64__sat_add(%s->meta.pos, ((uint64_t)(%s%s - %s%s)))",
			name, iopPrefix, name, io0Prefix, name)
		return nil

	case t.IDSince:
		b.printf("wuffs_base__io__since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)), %s%s)",
			iopPrefix, name, io0Prefix, name, io0Prefix, name)
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
				b.printf("(wuffs_base__store_u%d%ce(%s%s,", p.n, p.endianness, iopPrefix, name)
				if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
					return err
				}
				b.printf("), %s%s += %d, wuffs_base__make_empty_struct())",
					iopPrefix, name, p.n/8)
				return nil
			}
		}
	}

	return g.writeBuiltinIO(b, recv, method, args, depth)
}

func (g *gen) writeBuiltinNumType(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	switch method {
	case t.IDLowBits:
		// "recv.low_bits(n:etc)" in C is one of:
		//  - "((recv) & constant)"
		//  - "((recv) & WUFFS_BASE__LOW_BITS_MASK__UXX(n))"
		b.writes("((")
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writes(") & ")

		if cv := args[0].AsArg().Value().ConstValue(); cv != nil && cv.Sign() >= 0 && cv.Cmp(sixtyFour) <= 0 {
			mask := big.NewInt(0)
			mask.Lsh(one, uint(cv.Uint64()))
			mask.Sub(mask, one)
			b.printf("0x%s", strings.ToUpper(mask.Text(16)))
		} else {
			if sz, err := g.sizeof(recv.MType()); err != nil {
				return err
			} else {
				b.printf("WUFFS_BASE__LOW_BITS_MASK__U%d(", 8*sz)
			}
			if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
				return err
			}
			b.writes(")")
		}

		b.writes(")")
		return nil

	case t.IDHighBits:
		// "recv.high_bits(n:etc)" in C is "((recv) >> (8*sizeof(recv) - (n)))".
		b.writes("((")
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writes(") >> (")
		if sz, err := g.sizeof(recv.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes(" - (")
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
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
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writes(",")
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
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
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writes(",")
		if err := g.writeExpr(b, args[0].AsArg().Value(), depth); err != nil {
			return err
		}
		b.writes(")")
		return nil
	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinSlice(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	switch method {
	case t.IDCopyFromSlice:
		if err := g.writeBuiltinSliceCopyFromSlice8(b, recv, method, args, depth); err != errOptimizationNotApplicable {
			return err
		}

		// TODO: don't assume that the slice is a slice of base.u8.
		b.writes("wuffs_base__slice_u8__copy_from_slice(")
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writeb(',')
		return g.writeArgs(b, args, depth)

	case t.IDLength:
		b.writes("((uint64_t)(")
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writes(".len))")
		return nil

	case t.IDSuffix:
		// TODO: don't assume that the slice is a slice of base.u8.
		b.writes("wuffs_base__slice_u8__suffix(")
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writeb(',')
		return g.writeArgs(b, args, depth)
	}
	return errNoSuchBuiltin
}

// writeBuiltinSliceCopyFromSlice8 writes an optimized version of:
//
// foo[fIndex .. fIndex + 8].copy_from_slice!(s:bar[bIndex .. bIndex + 8])
func (g *gen) writeBuiltinSliceCopyFromSlice8(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	if method != t.IDCopyFromSlice || len(args) != 1 {
		return errOptimizationNotApplicable
	}
	foo, fIndex := matchFooIndexIndexPlus8(recv)
	bar, bIndex := matchFooIndexIndexPlus8(args[0].AsArg().Value())
	if foo == nil || bar == nil {
		return errOptimizationNotApplicable
	}
	b.writes("memcpy((")
	if err := g.writeExpr(b, foo, depth); err != nil {
		return err
	}
	if foo.MType().IsSliceType() {
		b.writes(".ptr")
	}
	if fIndex != nil {
		b.writes(")+(")
		if err := g.writeExpr(b, fIndex, depth); err != nil {
			return err
		}
	}
	b.writes("),(")
	if err := g.writeExpr(b, bar, depth); err != nil {
		return err
	}
	if bar.MType().IsSliceType() {
		b.writes(".ptr")
	}
	if bIndex != nil {
		b.writes(")+(")
		if err := g.writeExpr(b, bIndex, depth); err != nil {
			return err
		}
	}
	// TODO: don't assume that the slice is a slice of base.u8.
	b.writes("), 8)")
	return nil
}

// matchFooIndexIndexPlus8 matches n with "foo[index .. index + 8]" or "foo[..
// 8]". It returns a nil foo if there isn't a match.
func matchFooIndexIndexPlus8(n *a.Expr) (foo *a.Expr, index *a.Expr) {
	if n.Operator() != t.IDDotDot {
		return nil, nil
	}
	foo = n.LHS().AsExpr()
	index = n.MHS().AsExpr()
	rhs := n.RHS().AsExpr()
	if rhs == nil {
		return nil, nil
	}

	if index == nil {
		// No-op.
	} else if rhs.Operator() != t.IDXBinaryPlus || !rhs.LHS().AsExpr().Eq(index) {
		return nil, nil
	} else {
		rhs = rhs.RHS().AsExpr()
	}

	if cv := rhs.ConstValue(); cv == nil || cv.Cmp(eight) != 0 {
		return nil, nil
	}
	return foo, index
}

func (g *gen) writeBuiltinTable(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
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
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.writeb(',')
		return g.writeArgs(b, args, depth)
	}

	if field != "" {
		b.writes("((uint64_t)(")
		if err := g.writeExpr(b, recv, depth); err != nil {
			return err
		}
		b.printf(".%s))", field)
		return nil
	}

	return errNoSuchBuiltin
}

func (g *gen) writeArgs(b *buffer, args []*a.Node, depth uint32) error {
	for i, o := range args {
		if i > 0 {
			b.writeb(',')
		}
		if err := g.writeExpr(b, o.AsArg().Value(), depth); err != nil {
			return err
		}
	}
	b.writes(")")
	return nil
}

func (g *gen) writeBuiltinQuestionCall(b *buffer, n *a.Expr, depth uint32) error {
	// TODO: also handle (or reject??) being on the RHS of an =? operator.
	if (n.Operator() != t.IDOpenParen) || (!n.Effect().Coroutine()) {
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
		case t.IDReadU8, t.IDReadU8AsU32, t.IDReadU8AsU64:
			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}
			if g.currFunk.tempW > maxTemp {
				return fmt.Errorf("too many temporary variables required")
			}
			temp := g.currFunk.tempW
			g.currFunk.tempW++

			b.printf("if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {" +
				"status = wuffs_base__suspension__short_read; goto suspend; }")

			// TODO: watch for passing an array type to writeCTypeName? In C, an
			// array type can decay into a pointer.
			if err := g.writeCTypeName(b, n.MType(), tPrefix, fmt.Sprint(temp)); err != nil {
				return err
			}
			b.printf(" = *iop_a_src++;\n")
			return nil

		case t.IDSkip:
			x := n.Args()[0].AsArg().Value()
			if cv := x.ConstValue(); cv != nil && cv.Cmp(one) == 0 {
				if err := g.writeCoroSuspPoint(b, false); err != nil {
					return err
				}
				b.printf("if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {" +
					"status = wuffs_base__suspension__short_read; goto suspend; }")
				b.printf("iop_a_src++;\n")
				return nil
			}

			g.currFunk.usesScratch = true
			// TODO: don't hard-code [0], and allow recursive coroutines.
			scratchName := fmt.Sprintf("self->private_data.%s%s[0].scratch",
				sPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

			b.printf("%s = ", scratchName)
			if err := g.writeExpr(b, x, depth); err != nil {
				return err
			}
			b.writes(";\n")

			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}

			b.printf("if (%s > ((uint64_t)(io2_a_src - iop_a_src))) {\n", scratchName)
			b.printf("%s -= ((uint64_t)(io2_a_src - iop_a_src));\n", scratchName)
			b.printf("iop_a_src = io2_a_src;\n")

			b.writes("status = wuffs_base__suspension__short_read; goto suspend; }\n")
			b.printf("iop_a_src += %s;\n", scratchName)
			return nil
		}

		if method.Ident() >= readMethodsBase {
			if m := method.Ident() - readMethodsBase; m < t.ID(len(readMethods)) {
				if p := readMethods[m]; p.n != 0 {
					if err := g.writeCoroSuspPoint(b, false); err != nil {
						return err
					}
					return g.writeReadUxxAsUyy(b, n, "a_src", p.n, p.size, p.endianness)
				}
			}
		}

	} else {
		switch method.Ident() {
		case t.IDWriteU8:
			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}
			b.writes("if (iop_a_dst == io2_a_dst) {\n" +
				"status = wuffs_base__suspension__short_write; goto suspend; }\n" +
				"*iop_a_dst++ = ")
			x := n.Args()[0].AsArg().Value()
			if err := g.writeExpr(b, x, depth); err != nil {
				return err
			}
			b.writes(";\n")
			return nil
		}
	}
	return errNoSuchBuiltin
}

func (g *gen) writeReadUxxAsUyy(b *buffer, n *a.Expr, preName string, xx uint8, yy uint8, endianness uint8) error {
	if (xx&7 != 0) || (xx < 16) || (xx > 64) {
		return fmt.Errorf("internal error: bad writeReadUXX size %d", xx)
	}
	if endianness != 'b' && endianness != 'l' {
		return fmt.Errorf("internal error: bad writeReadUXX endianness %q", endianness)
	}

	if g.currFunk.tempW > maxTemp {
		return fmt.Errorf("too many temporary variables required")
	}
	temp := g.currFunk.tempW
	g.currFunk.tempW++

	if err := g.writeCTypeName(b, n.MType(), tPrefix, fmt.Sprint(temp)); err != nil {
		return err
	}
	b.writes(";")

	g.currFunk.usesScratch = true
	// TODO: don't hard-code [0], and allow recursive coroutines.
	scratchName := fmt.Sprintf("self->private_data.%s%s[0].scratch",
		sPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

	b.printf("if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= %d)) {", xx/8)
	b.printf("%s%d =", tPrefix, temp)
	if xx != yy {
		b.printf("((uint%d_t)(", yy)
	}
	b.printf("wuffs_base__load_u%d%ce(iop_a_src)", xx, endianness)
	if xx != yy {
		b.writes("))")
	}
	b.printf("; iop_a_src += %d;\n", xx/8)
	b.printf("} else {")

	b.printf("%s = 0;\n", scratchName)
	if err := g.writeCoroSuspPoint(b, false); err != nil {
		return err
	}
	b.printf("while (true) {")

	b.printf("if (WUFFS_BASE__UNLIKELY(iop_%s == io2_%s)) {"+
		"status = wuffs_base__suspension__short_read; goto suspend; }",
		preName, preName)

	b.printf("uint64_t *scratch = &%s;", scratchName)
	b.printf("uint32_t num_bits_%d = ((uint32_t)(*scratch", temp)
	switch endianness {
	case 'b':
		b.writes("& 0xFF)); *scratch >>= 8; *scratch <<= 8;")
		b.printf("*scratch |= ((uint64_t)(*%s%s++)) << (56 - num_bits_%d);",
			iopPrefix, preName, temp)
	case 'l':
		b.writes(">> 56)); *scratch <<= 8; *scratch >>= 8;")
		b.printf("*scratch |= ((uint64_t)(*%s%s++)) << num_bits_%d;",
			iopPrefix, preName, temp)
	}

	b.printf("if (num_bits_%d == %d) { %s%d = ((uint%d_t)(", temp, xx-8, tPrefix, temp, yy)
	switch endianness {
	case 'b':
		b.printf("*scratch >> %d));", 64-xx)
	case 'l':
		b.printf("*scratch));")
	}
	b.printf("break;")
	b.printf("}")

	b.printf("num_bits_%d += 8;", temp)
	switch endianness {
	case 'b':
		b.printf("*scratch |= ((uint64_t)(num_bits_%d));", temp)
	case 'l':
		b.printf("*scratch |= ((uint64_t)(num_bits_%d)) << 56;", temp)
	}

	b.writes("}}\n")
	return nil
}

const readMethodsBase = t.IDReadU8

var readMethods = [...]struct {
	size       uint8
	n          uint8
	endianness uint8
}{
	t.IDReadU8 - readMethodsBase: {8, 8, 'b'},

	t.IDReadU16BE - readMethodsBase: {16, 16, 'b'},
	t.IDReadU16LE - readMethodsBase: {16, 16, 'l'},

	t.IDReadU8AsU32 - readMethodsBase:    {32, 8, 'b'},
	t.IDReadU16BEAsU32 - readMethodsBase: {32, 16, 'b'},
	t.IDReadU16LEAsU32 - readMethodsBase: {32, 16, 'l'},
	t.IDReadU24BEAsU32 - readMethodsBase: {32, 24, 'b'},
	t.IDReadU24LEAsU32 - readMethodsBase: {32, 24, 'l'},
	t.IDReadU32BE - readMethodsBase:      {32, 32, 'b'},
	t.IDReadU32LE - readMethodsBase:      {32, 32, 'l'},

	t.IDReadU8AsU64 - readMethodsBase:    {64, 8, 'b'},
	t.IDReadU16BEAsU64 - readMethodsBase: {64, 16, 'b'},
	t.IDReadU16LEAsU64 - readMethodsBase: {64, 16, 'l'},
	t.IDReadU24BEAsU64 - readMethodsBase: {64, 24, 'b'},
	t.IDReadU24LEAsU64 - readMethodsBase: {64, 24, 'l'},
	t.IDReadU32BEAsU64 - readMethodsBase: {64, 32, 'b'},
	t.IDReadU32LEAsU64 - readMethodsBase: {64, 32, 'l'},
	t.IDReadU40BEAsU64 - readMethodsBase: {64, 40, 'b'},
	t.IDReadU40LEAsU64 - readMethodsBase: {64, 40, 'l'},
	t.IDReadU48BEAsU64 - readMethodsBase: {64, 48, 'b'},
	t.IDReadU48LEAsU64 - readMethodsBase: {64, 48, 'l'},
	t.IDReadU56BEAsU64 - readMethodsBase: {64, 56, 'b'},
	t.IDReadU56LEAsU64 - readMethodsBase: {64, 56, 'l'},
	t.IDReadU64BE - readMethodsBase:      {64, 64, 'b'},
	t.IDReadU64LE - readMethodsBase:      {64, 64, 'l'},
}

const peekMethodsBase = t.IDPeekU8

var peekMethods = [...]struct {
	size       uint8
	n          uint8
	endianness uint8
}{
	t.IDPeekU8 - peekMethodsBase: {8, 8, 'b'},

	t.IDPeekU16BE - peekMethodsBase: {16, 16, 'b'},
	t.IDPeekU16LE - peekMethodsBase: {16, 16, 'l'},

	t.IDPeekU8AsU32 - peekMethodsBase:    {32, 8, 'b'},
	t.IDPeekU16BEAsU32 - peekMethodsBase: {32, 16, 'b'},
	t.IDPeekU16LEAsU32 - peekMethodsBase: {32, 16, 'l'},
	t.IDPeekU24BEAsU32 - peekMethodsBase: {32, 24, 'b'},
	t.IDPeekU24LEAsU32 - peekMethodsBase: {32, 24, 'l'},
	t.IDPeekU32BE - peekMethodsBase:      {32, 32, 'b'},
	t.IDPeekU32LE - peekMethodsBase:      {32, 32, 'l'},

	t.IDPeekU8AsU64 - peekMethodsBase:    {64, 8, 'b'},
	t.IDPeekU16BEAsU64 - peekMethodsBase: {64, 16, 'b'},
	t.IDPeekU16LEAsU64 - peekMethodsBase: {64, 16, 'l'},
	t.IDPeekU24BEAsU64 - peekMethodsBase: {64, 24, 'b'},
	t.IDPeekU24LEAsU64 - peekMethodsBase: {64, 24, 'l'},
	t.IDPeekU32BEAsU64 - peekMethodsBase: {64, 32, 'b'},
	t.IDPeekU32LEAsU64 - peekMethodsBase: {64, 32, 'l'},
	t.IDPeekU40BEAsU64 - peekMethodsBase: {64, 40, 'b'},
	t.IDPeekU40LEAsU64 - peekMethodsBase: {64, 40, 'l'},
	t.IDPeekU48BEAsU64 - peekMethodsBase: {64, 48, 'b'},
	t.IDPeekU48LEAsU64 - peekMethodsBase: {64, 48, 'l'},
	t.IDPeekU56BEAsU64 - peekMethodsBase: {64, 56, 'b'},
	t.IDPeekU56LEAsU64 - peekMethodsBase: {64, 56, 'l'},
	t.IDPeekU64BE - peekMethodsBase:      {64, 64, 'b'},
	t.IDPeekU64LE - peekMethodsBase:      {64, 64, 'l'},
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
