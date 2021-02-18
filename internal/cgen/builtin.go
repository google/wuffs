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

func (g *gen) writeBuiltinCall(b *buffer, n *a.Expr, sideEffectsOnly bool, depth uint32) error {
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

		if method.Ident() != t.IDSet {
			return errNoSuchBuiltin
		}
		recvName, err := g.recvName(recv)
		if err != nil {
			return err
		}

		switch recvTyp.Inner().QID() {
		case t.QID{t.IDBase, t.IDImageConfig}:
			b.printf("wuffs_base__image_config__set(\n%s", recvName)
		case t.QID{t.IDBase, t.IDFrameConfig}:
			b.printf("wuffs_base__frame_config__set(\n%s", recvName)
			u64ToFlicksIndex = 1
		default:
			return errNoSuchBuiltin
		}

		for i, o := range n.Args() {
			b.writes(",\n")
			if i == u64ToFlicksIndex {
				if o.AsArg().Name().Str(g.tm) != "duration" {
					return errors.New("cgen: internal error: inconsistent frame_config.set argument")
				}
				b.writes("((wuffs_base__flicks)(")
			}
			if err := g.writeExpr(b, o.AsArg().Value(), false, depth); err != nil {
				return err
			}
			if i == u64ToFlicksIndex {
				b.writes("))")
			}
		}
		b.writeb(')')
		return nil

	case t.IDSlice:
		return g.writeBuiltinSlice(b, recv, method.Ident(), n.Args(), sideEffectsOnly, depth)
	case t.IDTable:
		return g.writeBuiltinTable(b, recv, method.Ident(), n.Args(), sideEffectsOnly, depth)
	default:
		return errNoSuchBuiltin
	}

	qid := recvTyp.QID()
	if qid[0] != t.IDBase {
		return errNoSuchBuiltin
	}

	if qid[1].IsNumType() {
		return g.writeBuiltinNumType(b, recv, method.Ident(), n.Args(), depth)
	} else if qid[1].IsBuiltInCPUArch() {
		return g.writeBuiltinCPUArch(b, recv, method.Ident(), n.Args(), sideEffectsOnly, depth)
	} else {
		switch qid[1] {
		case t.IDIOReader:
			return g.writeBuiltinIOReader(b, recv, method.Ident(), n.Args(), sideEffectsOnly, depth)
		case t.IDIOWriter:
			return g.writeBuiltinIOWriter(b, recv, method.Ident(), n.Args(), sideEffectsOnly, depth)
		case t.IDPixelSwizzler:
			switch method.Ident() {
			case t.IDLimitedSwizzleU32InterleavedFromReader, t.IDSwizzleInterleavedFromReader:
				b.writes("wuffs_base__pixel_swizzler__")
				if method.Ident() == t.IDLimitedSwizzleU32InterleavedFromReader {
					b.writes("limited_swizzle_u32_interleaved_from_reader")
				} else {
					b.writes("swizzle_interleaved_from_reader")
				}
				b.writes("(\n&")
				if err := g.writeExpr(b, recv, false, depth); err != nil {
					return err
				}
				args := n.Args()
				for _, o := range args[:len(args)-1] {
					b.writes(",\n")
					if err := g.writeExpr(b, o.AsArg().Value(), false, depth); err != nil {
						return err
					}
				}
				readerArgName, err := g.recvName(args[len(args)-1].AsArg().Value())
				if err != nil {
					return err
				}
				b.printf(",\n&%s%s,\n%s%s)", iopPrefix, readerArgName, io2Prefix, readerArgName)
				return nil
			}
		case t.IDTokenWriter:
			return g.writeBuiltinTokenWriter(b, recv, method.Ident(), n.Args(), depth)
		case t.IDUtility:
			switch method.Ident() {
			case t.IDCPUArchIs32Bit:
				b.writes("(sizeof(void*) == 4)")
				return nil
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

func (g *gen) recvName(recv *a.Expr) (string, error) {
	switch recv.Operator() {
	case 0:
		return vPrefix + recv.Ident().Str(g.tm), nil
	case t.IDDot:
		if lhs := recv.LHS().AsExpr(); lhs.Operator() == 0 && lhs.Ident() == t.IDArgs {
			return aPrefix + recv.Ident().Str(g.tm), nil
		}
	}
	return "", fmt.Errorf("recvName: cannot cgen a %q expression", recv.Str(g.tm))
}

func (g *gen) writeBuiltinIO(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	switch method {
	case t.IDLength:
		recvName, err := g.recvName(recv)
		if err != nil {
			return err
		}
		b.printf("((uint64_t)(%s%s - %s%s))", io2Prefix, recvName, iopPrefix, recvName)
		return nil
	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinIOReader(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, sideEffectsOnly bool, depth uint32) error {
	recvName, err := g.recvName(recv)
	if err != nil {
		return err
	}

	switch method {
	case t.IDValidUTF8Length:
		b.printf("((uint64_t)(wuffs_base__utf_8__longest_valid_prefix(%s%s,\n"+
			"((size_t)(wuffs_base__u64__min(((uint64_t)(%s%s - %s%s)), ",
			iopPrefix, recvName, io2Prefix, recvName, iopPrefix, recvName)
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes("))))))")
		return nil

	case t.IDUndoByte:
		if !sideEffectsOnly {
			// Generate a two part expression using the comma operator: "(etc,
			// return_empty_struct call)". The final part is a function call
			// (to a static inline function) instead of a struct literal, to
			// avoid a "expression result unused" compiler error.
			b.writes("(")
		}
		b.printf("%s%s--", iopPrefix, recvName)
		if !sideEffectsOnly {
			b.writes(", wuffs_base__make_empty_struct())")
		}
		return nil

	case t.IDCanUndoByte:
		b.printf("(%s%s > %s%s)", iopPrefix, recvName, io1Prefix, recvName)
		return nil

	case t.IDLimitedCopyU32ToSlice:
		b.printf("wuffs_base__io_reader__limited_copy_u32_to_slice(\n&%s%s, %s%s,",
			iopPrefix, recvName, io2Prefix, recvName)
		return g.writeArgs(b, args, depth)

	case t.IDCountSince:
		b.printf("wuffs_base__io__count_since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)))", iopPrefix, recvName, io0Prefix, recvName)
		return nil

	case t.IDIsClosed:
		b.printf("(%s && %s->meta.closed)", recvName, recvName)
		return nil

	case t.IDMark:
		b.printf("((uint64_t)(%s%s - %s%s))", iopPrefix, recvName, io0Prefix, recvName)
		return nil

	case t.IDMatch7:
		b.printf("wuffs_base__io_reader__match7(%s%s, %s%s, %s,",
			iopPrefix, recvName, io2Prefix, recvName, recvName)
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writeb(')')
		return nil

	case t.IDPeekU64LEAt:
		b.printf("wuffs_base__peek_u64le__no_bounds_check(%s%s + ", iopPrefix, recvName)
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writeb(')')
		return nil

	case t.IDPosition:
		b.printf("wuffs_base__u64__sat_add(%s->meta.pos, ((uint64_t)(%s%s - %s%s)))",
			recvName, iopPrefix, recvName, io0Prefix, recvName)
		return nil

	case t.IDSince:
		b.printf("wuffs_base__io__since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)), %s%s)",
			iopPrefix, recvName, io0Prefix, recvName, io0Prefix, recvName)
		return nil

	case t.IDSkipU32Fast:
		if !sideEffectsOnly {
			// Generate a two part expression using the comma operator: "(etc,
			// return_empty_struct call)". The final part is a function call
			// (to a static inline function) instead of a struct literal, to
			// avoid a "expression result unused" compiler error.
			b.writes("(")
		}
		b.printf("%s%s += ", iopPrefix, recvName)
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		if !sideEffectsOnly {
			b.writes(", wuffs_base__make_empty_struct())")
		}
		return nil
	}

	if method >= peekMethodsBase {
		if m := method - peekMethodsBase; m < t.ID(len(peekMethods)) {
			if p := peekMethods[m]; p.n != 0 {
				if p.size != p.n {
					b.printf("((uint%d_t)(", p.size)
				}
				b.printf("wuffs_base__peek_u%d%ce__no_bounds_check(%s%s)",
					p.n, p.endianness, iopPrefix, recvName)
				if p.size != p.n {
					b.writes("))")
				}
				return nil
			}
		}
	}

	return g.writeBuiltinIO(b, recv, method, args, depth)
}

func (g *gen) writeBuiltinIOWriter(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, sideEffectsOnly bool, depth uint32) error {
	recvName, err := g.recvName(recv)
	if err != nil {
		return err
	}

	switch method {
	case t.IDLimitedCopyU32FromHistory, t.IDLimitedCopyU32FromHistoryFast:
		suffix := ""
		if method == t.IDLimitedCopyU32FromHistoryFast {
			suffix = "_fast"
		}
		b.printf("wuffs_base__io_writer__limited_copy_u32_from_history%s(\n&%s%s, %s%s, %s%s",
			suffix, iopPrefix, recvName, io0Prefix, recvName, io2Prefix, recvName)
		for _, o := range args {
			b.writes(", ")
			if err := g.writeExpr(b, o.AsArg().Value(), false, depth); err != nil {
				return err
			}
		}
		b.writeb(')')
		return nil

	case t.IDLimitedCopyU32FromReader:
		readerName, err := g.recvName(args[1].AsArg().Value())
		if err != nil {
			return err
		}

		b.printf("wuffs_base__io_writer__limited_copy_u32_from_reader(\n&%s%s, %s%s,",
			iopPrefix, recvName, io2Prefix, recvName)
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.printf(", &%s%s, %s%s)", iopPrefix, readerName, io2Prefix, readerName)
		return nil

	case t.IDCopyFromSlice:
		b.printf("wuffs_base__io_writer__copy_from_slice(&%s%s, %s%s,",
			iopPrefix, recvName, io2Prefix, recvName)
		return g.writeArgs(b, args, depth)

	case t.IDLimitedCopyU32FromSlice:
		b.printf("wuffs_base__io_writer__limited_copy_u32_from_slice(\n&%s%s, %s%s,",
			iopPrefix, recvName, io2Prefix, recvName)
		return g.writeArgs(b, args, depth)

	case t.IDCountSince:
		b.printf("wuffs_base__io__count_since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)))", iopPrefix, recvName, io0Prefix, recvName)
		return nil

	case t.IDHistoryLength, t.IDMark:
		b.printf("((uint64_t)(%s%s - %s%s))", iopPrefix, recvName, io0Prefix, recvName)
		return nil

	case t.IDPosition:
		b.printf("wuffs_base__u64__sat_add(%s->meta.pos, ((uint64_t)(%s%s - %s%s)))",
			recvName, iopPrefix, recvName, io0Prefix, recvName)
		return nil

	case t.IDSince:
		b.printf("wuffs_base__io__since(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.printf(", ((uint64_t)(%s%s - %s%s)), %s%s)",
			iopPrefix, recvName, io0Prefix, recvName, io0Prefix, recvName)
		return nil
	}

	if method >= writeFastMethodsBase {
		if m := method - writeFastMethodsBase; m < t.ID(len(writeFastMethods)) {
			if p := writeFastMethods[m]; p.n != 0 {
				// Generate a two/three part expression using the comma
				// operator: "(store, pointer increment, return_empty_struct
				// call)". The final part is a function call (to a static
				// inline function) instead of a struct literal, to avoid a
				// "expression result unused" compiler error.
				b.printf("(wuffs_base__poke_u%d%ce__no_bounds_check(%s%s, ",
					p.n, p.endianness, iopPrefix, recvName)
				if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
					return err
				}
				b.printf("), %s%s += %d", iopPrefix, recvName, p.n/8)
				if !sideEffectsOnly {
					b.writes(", wuffs_base__make_empty_struct()")
				}
				b.writes(")")
				return nil
			}
		}
	}

	return g.writeBuiltinIO(b, recv, method, args, depth)
}

func (g *gen) writeBuiltinTokenWriter(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	switch method {
	case t.IDWriteSimpleTokenFast, t.IDWriteExtendedTokenFast:
		recvName, err := g.recvName(recv)
		if err != nil {
			return err
		}
		b.printf("*iop_%s++ = wuffs_base__make_token(\n", recvName)

		if method == t.IDWriteSimpleTokenFast {
			b.writes("(((uint64_t)(")
			if cv := args[0].AsArg().Value().ConstValue(); (cv == nil) || (cv.Sign() != 0) {
				if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
					return err
				}
				b.writes(")) << WUFFS_BASE__TOKEN__VALUE_MAJOR__SHIFT) |\n(((uint64_t)(")
			}

			if err := g.writeExpr(b, args[1].AsArg().Value(), false, depth); err != nil {
				return err
			}
			b.writes(")) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |\n(((uint64_t)(")
		} else {
			b.writes("(~")
			if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
				return err
			}
			b.writes(" << WUFFS_BASE__TOKEN__VALUE_EXTENSION__SHIFT) |\n(((uint64_t)(")
		}

		if cv := args[len(args)-2].AsArg().Value().ConstValue(); (cv == nil) || (cv.Sign() != 0) {
			if err := g.writeExpr(b, args[len(args)-2].AsArg().Value(), false, depth); err != nil {
				return err
			}
			b.writes(")) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |\n(((uint64_t)(")
		}

		if err := g.writeExpr(b, args[len(args)-1].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes(")) << WUFFS_BASE__TOKEN__LENGTH__SHIFT))")
		return nil
	}

	return g.writeBuiltinIO(b, recv, method, args, depth)
}

func (g *gen) writeBuiltinCPUArch(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, sideEffectsOnly bool, depth uint32) error {
	switch recv.MType().QID()[1] {
	case t.IDX86SSE42Utility:
		return g.writeBuiltinCPUArchX86(b, recv, method, args, sideEffectsOnly, depth)
	}
	armCRC32U32 := recv.MType().Eq(typeExprARMCRC32U32)
	armNeon := recv.MType().Eq(typeExprARMNeon64) || recv.MType().Eq(typeExprARMNeon128)

	switch method {
	case t.IDTruncateU32, t.IDTruncateU64, t.IDStoreSlice128:
		switch method {
		case t.IDTruncateU32:
			b.writes("((uint32_t)(_mm_cvtsi128_si32(")
		case t.IDTruncateU64:
			b.writes("((uint64_t)(_mm_cvtsi128_si64(")
		case t.IDStoreSlice128:
			if !sideEffectsOnly {
				// Generate a two part expression using the comma operator: "(etc,
				// return_empty_struct call)". The final part is a function call
				// (to a static inline function) instead of a struct literal, to
				// avoid a "expression result unused" compiler error.
				b.writes("(")
			}
			b.writes("_mm_storeu_si128((__m128i*)(void*)(")
			if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
				return err
			}
			b.writes(".ptr), ")
		}

		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}

		switch method {
		case t.IDStoreSlice128:
			b.writes(")")
			if !sideEffectsOnly {
				b.writes(", wuffs_base__make_empty_struct())")
			}
		default:
			b.writes(")))")
		}
		return nil

	case t.IDCreateSlice64, t.IDCreateSlice128:
		if armNeon {
			switch method {
			case t.IDCreateSlice64:
				b.writes("vld1_u8(")
			case t.IDCreateSlice128:
				b.writes("vld1q_u8(")
			}
		}
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes(".ptr)")
		return nil
	}

	methodStr := method.Str(g.tm)
	vreinterpretU8Uxx, vreinterpretUxxU8, vreinterpretClose := "", "", ""
	if armNeon {
		vreinterpretClose = ")"
		switch {
		case methodStr == "create_vdupq_n_u16":
			vreinterpretU8Uxx = "vreinterpretq_u8_u16("
			vreinterpretUxxU8 = "("
		case methodStr == "create_vdupq_n_u32":
			vreinterpretU8Uxx = "vreinterpretq_u8_u32("
			vreinterpretUxxU8 = "("
		case methodStr == "create_vdupq_n_u64":
			vreinterpretU8Uxx = "vreinterpretq_u8_u64("
			vreinterpretUxxU8 = "("
		case methodStr == "vmovn_u16":
			vreinterpretU8Uxx = "("
			vreinterpretUxxU8 = "vreinterpretq_u16_u8("
		case methodStr == "vmovn_u32":
			vreinterpretU8Uxx = "vreinterpret_u8_u16("
			vreinterpretUxxU8 = "vreinterpretq_u32_u8("
		case methodStr == "vmovn_u64":
			vreinterpretU8Uxx = "vreinterpret_u8_u32("
			vreinterpretUxxU8 = "vreinterpretq_u64_u8("
		case strings.HasSuffix(methodStr, "q_u16"):
			vreinterpretU8Uxx = "vreinterpretq_u8_u16("
			vreinterpretUxxU8 = "vreinterpretq_u16_u8("
		case strings.HasSuffix(methodStr, "q_u32"):
			vreinterpretU8Uxx = "vreinterpretq_u8_u32("
			vreinterpretUxxU8 = "vreinterpretq_u32_u8("
		case strings.HasSuffix(methodStr, "q_u64"):
			vreinterpretU8Uxx = "vreinterpretq_u8_u64("
			vreinterpretUxxU8 = "vreinterpretq_u64_u8("
		case strings.HasSuffix(methodStr, "_u16"):
			vreinterpretU8Uxx = "vreinterpret_u8_u16("
			vreinterpretUxxU8 = "vreinterpret_u16_u8("
		case strings.HasSuffix(methodStr, "_u32"):
			vreinterpretU8Uxx = "vreinterpret_u8_u32("
			vreinterpretUxxU8 = "vreinterpret_u32_u8("
		case strings.HasSuffix(methodStr, "_u64"):
			vreinterpretU8Uxx = "vreinterpret_u8_u64("
			vreinterpretUxxU8 = "vreinterpret_u64_u8("
		default:
			vreinterpretClose = ""
		}
	}

	const (
		create         = "create"
		createLiteralU = "create_literal_u"
	)
	if methodStr == "value" {
		return g.writeExpr(b, recv, false, depth)

	} else if methodStr == create {
		return g.writeExpr(b, args[0].AsArg().Value(), false, depth)

	} else if strings.HasPrefix(methodStr, createLiteralU) {
		methodStr = methodStr[len(createLiteralU):]
		switch methodStr {
		case "16x4":
			b.writes("vreinterpret_u8_u16")
		case "16x8":
			b.writes("vreinterpretq_u8_u16")
		case "32x2":
			b.writes("vreinterpret_u8_u32")
		case "32x4":
			b.writes("vreinterpretq_u8_u32")
		case "64x1":
			b.writes("vreinterpret_u8_u64")
		case "64x2":
			b.writes("vreinterpretq_u8_u64")
		}
		b.writes("((uint")
		b.writes(methodStr)
		b.writes("_t){")
		for i, o := range args {
			if i > 0 {
				b.writes(", ")
			}
			if err := g.writeExpr(b, o.AsArg().Value(), false, depth); err != nil {
				return err
			}
		}
		b.writes("})")
		return nil

	} else if strings.HasPrefix(methodStr, create) {
		methodStr = methodStr[len(create):]
		if armNeon && (methodStr != "") && (methodStr[0] == '_') {
			methodStr = methodStr[1:]
		}

		b.writes(vreinterpretU8Uxx)
		b.printf("%s(", methodStr)
		for i, o := range args {
			if i > 0 {
				b.writes(", ")
			}
			if err := g.writeExpr(b, o.AsArg().Value(), false, depth); err != nil {
				return err
			}
		}
		b.writes(")")
		b.writes(vreinterpretClose)
		return nil

	}

	switch methodStr {
	case "vaddw_u8":
		b.writes("vreinterpretq_u8_u16(")
		b.writes(methodStr)
		b.writes("(vreinterpretq_u16_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes("), ")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes("))")
		return nil

	case "vget_low_u16", "vget_high_u16":
		b.writes("vreinterpret_u8_u16(")
		b.writes(methodStr)
		b.writes("(vreinterpretq_u16_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(")))")
		return nil

	case "vget_low_u32", "vget_high_u32":
		b.writes("vreinterpret_u8_u32(")
		b.writes(methodStr)
		b.writes("(vreinterpretq_u32_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(")))")
		return nil

	case "vget_low_u64", "vget_high_u64":
		b.writes("vreinterpret_u8_u64(")
		b.writes(methodStr)
		b.writes("(vreinterpretq_u64_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(")))")
		return nil

	case "vget_low_u8", "vget_high_u8":
		b.writes(methodStr)
		b.writes("(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil

	case "vmlal_u16":
		b.writes("vreinterpretq_u8_u32(")
		b.writes(methodStr)
		b.writes("(vreinterpretq_u32_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(")")
		for i := range args {
			b.writes(", vreinterpret_u16_u8(")
			if err := g.writeExpr(b, args[i].AsArg().Value(), false, depth); err != nil {
				return err
			}
			b.writes(")")
		}
		b.writes("))")

	case "vshlq_n_u32":
		b.writes("vreinterpretq_u8_u32(")
		b.writes(methodStr)
		b.writes("(vreinterpretq_u32_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes("), ")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes("))")

	case "vpadalq_u16":
		b.writes("vreinterpretq_u8_u32(vpadalq_u16(vreinterpretq_u32_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes("), vreinterpretq_u16_u8(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes(")))")
		return nil

	case "vpadalq_u32":
		b.writes("vreinterpretq_u8_u64(vpadalq_u32(vreinterpretq_u64_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes("), vreinterpretq_u32_u8(")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes(")))")
		return nil

	case "vpadalq_u8":
		b.writes("vreinterpretq_u8_u16(vpadalq_u8(vreinterpretq_u16_u8(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes("), ")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes("))")
		return nil

	default:
		postArgsAfter := ""
		if armCRC32U32 {
			b.writeb('_')
		} else if armNeon {
			// TODO: generate this table automatically?
			postArgsAfter = ")"
			switch methodStr {
			case "vpadd_u16":
				b.writes("vreinterpret_u8_u16(")
			case "vpadd_u32":
				b.writes("vreinterpret_u8_u32(")
			case "vabdl_u8", "vaddl_u8",
				"vabdq_u16", "vcleq_u16",
				"vaddq_u16",
				"vpaddlq_u8":
				b.writes("vreinterpretq_u8_u16(")
			case "vabdl_u16", "vaddl_u16",
				"vabdq_u32", "vcleq_u32",
				"vaddq_u32",
				"vpaddlq_u16":
				b.writes("vreinterpretq_u8_u32(")
			case "vabdl_u32", "vaddl_u32",
				"vaddq_u64",
				"vpaddlq_u32":
				b.writes("vreinterpretq_u8_u64(")
			default:
				postArgsAfter = ""
			}
		}
		b.printf("%s(", methodStr)

		if armNeon && recv.MType().IsCPUArchType() {
			b.writes(vreinterpretUxxU8)
		}
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		if armNeon && recv.MType().IsCPUArchType() {
			b.writes(vreinterpretClose)
		}

		for _, o := range args {
			b.writes(", ")
			after := ""
			v := o.AsArg().Value()
			if armCRC32U32 {
				// No-op.
			} else if armNeon {
				if v.MType().IsCPUArchType() {
					b.writes(vreinterpretUxxU8)
					after = vreinterpretClose
				}
			} else if !v.MType().IsCPUArchType() {
				b.writes("(int32_t)(")
				after = ")"
			}
			if err := g.writeExpr(b, v, false, depth); err != nil {
				return err
			}
			b.writes(after)
		}
		b.writes(")")
		b.writes(postArgsAfter)
	}
	return nil
}

func (g *gen) writeBuiltinCPUArchX86(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, sideEffectsOnly bool, depth uint32) error {
	methodStr := method.Str(g.tm)
	if strings.HasPrefix(methodStr, "make_") {
		fName, tName, ptr := "", "", false
		switch methodStr {
		case "make_m128i_multiple_u8":
			fName, tName = "_mm_set_epi8", "int8_t"
		case "make_m128i_multiple_u16":
			fName, tName = "_mm_set_epi16", "int16_t"
		case "make_m128i_multiple_u32":
			fName, tName = "_mm_set_epi32", "int32_t"
		case "make_m128i_multiple_u64":
			fName, tName = "_mm_set_epi64x", "int64_t"
		case "make_m128i_repeat_u8":
			fName, tName = "_mm_set1_epi8", "int8_t"
		case "make_m128i_repeat_u16":
			fName, tName = "_mm_set1_epi16", "int16_t"
		case "make_m128i_repeat_u32":
			fName, tName = "_mm_set1_epi32", "int32_t"
		case "make_m128i_repeat_u64":
			fName, tName = "_mm_set1_epi64x", "int64_t"
		case "make_m128i_single_u32":
			fName, tName = "_mm_cvtsi32_si128", "int32_t"
		case "make_m128i_single_u64":
			fName, tName = "_mm_cvtsi64x_si128", "int64_t"
		case "make_m128i_slice128":
			fName, tName, ptr = "_mm_lddqu_si128", "const __m128i*)(const void*", true
		case "make_m128i_zeroes":
			fName, tName = "_mm_setzero_si128", ""
		default:
			return fmt.Errorf("internal error: unsupported cpu_arch method %q", methodStr)
		}
		b.printf("%s(", fName)
		for i := range args {
			// Iterate backwards to match _mm_setetc arg order.
			o := args[len(args)-1-i].AsArg()

			if i > 0 {
				b.writes(", ")
			}
			b.printf("(%s)(", tName)
			if ptr {
				if err := g.writeExprDotPtr(b, o.Value(), false, depth); err != nil {
					return err
				}
			} else {
				if err := g.writeExpr(b, o.Value(), false, depth); err != nil {
					return err
				}
			}
			b.writes(")")
		}
		b.writes(")")
		return nil
	}
	return nil
}

func (g *gen) writeExprDotPtr(b *buffer, n *a.Expr, sideEffectsOnly bool, depth uint32) error {
	if n.Operator() == t.IDDotDot {
		if err := g.writeExpr(b, n.LHS().AsExpr(), sideEffectsOnly, depth); err != nil {
			return err
		}
		b.writes(".ptr")
		if n.MHS() != nil {
			b.writes(" + ")
			if err := g.writeExpr(b, n.MHS().AsExpr(), sideEffectsOnly, depth); err != nil {
				return err
			}
		}
		return nil
	}

	if err := g.writeExpr(b, n, sideEffectsOnly, depth); err != nil {
		return err
	}
	b.writes(".ptr")
	return nil
}

func (g *gen) writeBuiltinNumType(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, depth uint32) error {
	switch method {
	case t.IDLowBits:
		// "recv.low_bits(n:etc)" in C is one of:
		//  - "((recv) & constant)"
		//  - "((recv) & WUFFS_BASE__LOW_BITS_MASK__UXX(n))"
		b.writes("((")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
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
			if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
				return err
			}
			b.writes(")")
		}

		b.writes(")")
		return nil

	case t.IDHighBits:
		// "recv.high_bits(n:etc)" in C is "((recv) >> (8*sizeof(recv) - (n)))".
		b.writes("((")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(") >> (")
		if sz, err := g.sizeof(recv.MType()); err != nil {
			return err
		} else {
			b.printf("%d", 8*sz)
		}
		b.writes(" - (")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
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
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(", ")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
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
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(", ")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes(")")
		return nil
	}
	return errNoSuchBuiltin
}

func (g *gen) writeBuiltinSlice(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, sideEffectsOnly bool, depth uint32) error {
	switch method {
	case t.IDCopyFromSlice:
		if err := g.writeBuiltinSliceCopyFromSlice8(b, recv, method, args, depth); err != errOptimizationNotApplicable {
			return err
		}

		// TODO: don't assume that the slice is a slice of base.u8.
		b.writes("wuffs_base__slice_u8__copy_from_slice(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(", ")
		return g.writeArgs(b, args, depth)

	case t.IDLength:
		b.writes("((uint64_t)(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(".len))")
		return nil

	case t.IDUintptrLow12Bits:
		b.writes("((uint32_t)(0xFFF & (uintptr_t)(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(".ptr)))")
		return nil

	case t.IDSuffix:
		// TODO: don't assume that the slice is a slice of base.u8.
		b.writes("wuffs_base__slice_u8__suffix(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(", ")
		return g.writeArgs(b, args, depth)
	}

	if (t.IDPeekU8 <= method) && (method <= t.IDPeekU64LE) {
		s := method.Str(g.tm)
		if i := strings.Index(s, "_as_"); i >= 0 {
			s = s[:i]
		}
		b.printf("wuffs_base__%s__no_bounds_check(", s)
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(".ptr)")
		return nil
	}

	if (t.IDPokeU8 <= method) && (method <= t.IDPokeU64LE) {
		if !sideEffectsOnly {
			// Generate a two part expression using the comma operator: "(etc,
			// return_empty_struct call)". The final part is a function call
			// (to a static inline function) instead of a struct literal, to
			// avoid a "expression result unused" compiler error.
			b.writes("(")
		}
		b.printf("wuffs_base__%s__no_bounds_check(", method.Str(g.tm))
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(".ptr, ")
		if err := g.writeExpr(b, args[0].AsArg().Value(), false, depth); err != nil {
			return err
		}
		b.writes(")")
		if !sideEffectsOnly {
			b.writes(", wuffs_base__make_empty_struct())")
		}
		return nil
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
	if err := g.writeExpr(b, foo, false, depth); err != nil {
		return err
	}
	if foo.MType().IsSliceType() {
		b.writes(".ptr")
	}
	if fIndex != nil {
		b.writes(")+(")
		if err := g.writeExpr(b, fIndex, false, depth); err != nil {
			return err
		}
	}
	b.writes("), (")
	if err := g.writeExpr(b, bar, false, depth); err != nil {
		return err
	}
	if bar.MType().IsSliceType() {
		b.writes(".ptr")
	}
	if bIndex != nil {
		b.writes(")+(")
		if err := g.writeExpr(b, bIndex, false, depth); err != nil {
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

func (g *gen) writeBuiltinTable(b *buffer, recv *a.Expr, method t.ID, args []*a.Node, sideEffectsOnly bool, depth uint32) error {
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
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.writes(", ")
		return g.writeArgs(b, args, depth)
	}

	if field != "" {
		b.writes("((uint64_t)(")
		if err := g.writeExpr(b, recv, false, depth); err != nil {
			return err
		}
		b.printf(".%s))", field)
		return nil
	}

	return errNoSuchBuiltin
}

func (g *gen) writeArgs(b *buffer, args []*a.Node, depth uint32) error {
	if len(args) >= 4 {
		b.writeb('\n')
	}
	for i, o := range args {
		if i > 0 {
			if len(args) >= 4 {
				b.writes(",\n")
			} else {
				b.writes(", ")
			}
		}
		if err := g.writeExpr(b, o.AsArg().Value(), false, depth); err != nil {
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
	if !recvTyp.IsIOTokenType() {
		return errNoSuchBuiltin
	}
	recvName, err := g.recvName(recv)
	if err != nil {
		return err
	}

	switch recvTyp.QID()[1] {
	case t.IDIOReader:
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

			b.printf("if (WUFFS_BASE__UNLIKELY(iop_%s == io2_%s)) {\n"+
				"status = wuffs_base__make_status(wuffs_base__suspension__short_read);\n"+
				"goto suspend;\n}\n",
				recvName, recvName)

			// TODO: watch for passing an array type to writeCTypeName? In C, an
			// array type can decay into a pointer.
			if err := g.writeCTypeName(b, n.MType(), tPrefix, fmt.Sprint(temp)); err != nil {
				return err
			}
			b.printf(" = *iop_%s++;\n", recvName)
			return nil

		case t.IDSkip, t.IDSkipU32:
			x := n.Args()[0].AsArg().Value()
			if cv := x.ConstValue(); cv != nil && cv.Cmp(one) == 0 {
				if err := g.writeCoroSuspPoint(b, false); err != nil {
					return err
				}
				b.printf("if (WUFFS_BASE__UNLIKELY(iop_%s == io2_%s)) {\n"+
					"status = wuffs_base__make_status(wuffs_base__suspension__short_read);\n"+
					"goto suspend;\n}\n",
					recvName, recvName)
				b.printf("iop_%s++;\n", recvName)
				return nil
			}

			g.currFunk.usesScratch = true
			// TODO: don't hard-code [0], and allow recursive coroutines.
			scratchName := fmt.Sprintf("self->private_data.%s%s[0].scratch",
				sPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

			b.printf("%s = ", scratchName)
			if err := g.writeExpr(b, x, false, depth); err != nil {
				return err
			}
			b.writes(";\n")

			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}

			b.printf("if (%s > ((uint64_t)(io2_%s - iop_%s))) {\n", scratchName, recvName, recvName)
			b.printf("%s -= ((uint64_t)(io2_%s - iop_%s));\n", scratchName, recvName, recvName)
			b.printf("iop_%s = io2_%s;\n", recvName, recvName)

			b.writes("status = wuffs_base__make_status(wuffs_base__suspension__short_read);\ngoto suspend;\n}\n")
			b.printf("iop_%s += %s;\n", recvName, scratchName)
			return nil
		}

		if method.Ident() >= readMethodsBase {
			if m := method.Ident() - readMethodsBase; m < t.ID(len(readMethods)) {
				if p := readMethods[m]; p.n != 0 {
					if err := g.writeCoroSuspPoint(b, false); err != nil {
						return err
					}
					return g.writeReadUxxAsUyy(b, n, recvName, p.n, p.size, p.endianness)
				}
			}
		}

	case t.IDIOWriter:
		switch method.Ident() {
		case t.IDWriteU8:
			g.currFunk.usesScratch = true
			// TODO: don't hard-code [0], and allow recursive coroutines.
			scratchName := fmt.Sprintf("self->private_data.%s%s[0].scratch",
				sPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

			b.printf("%s = ", scratchName)
			x := n.Args()[0].AsArg().Value()
			if err := g.writeExpr(b, x, false, depth); err != nil {
				return err
			}
			b.writes(";\n")

			if err := g.writeCoroSuspPoint(b, false); err != nil {
				return err
			}
			b.printf("if (iop_%s == io2_%s) {\n"+
				"status = wuffs_base__make_status(wuffs_base__suspension__short_write);\n"+
				"goto suspend;\n}\n"+
				"*iop_%s++ = ((uint8_t)(%s));\n",
				recvName, recvName, recvName, scratchName)
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
	b.writes(";\n")

	method := n.LHS().AsExpr()
	recv := method.LHS().AsExpr()
	recvName, err := g.recvName(recv)
	if err != nil {
		return err
	}

	g.currFunk.usesScratch = true
	// TODO: don't hard-code [0], and allow recursive coroutines.
	scratchName := fmt.Sprintf("self->private_data.%s%s[0].scratch",
		sPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

	b.printf("if (WUFFS_BASE__LIKELY(io2_%s - iop_%s >= %d)) {\n", recvName, recvName, xx/8)
	b.printf("%s%d = ", tPrefix, temp)
	if xx != yy {
		b.printf("((uint%d_t)(", yy)
	}
	b.printf("wuffs_base__peek_u%d%ce__no_bounds_check(iop_%s)", xx, endianness, recvName)
	if xx != yy {
		b.writes("))")
	}
	b.printf(";\niop_%s += %d;\n", recvName, xx/8)
	b.printf("} else {\n")

	b.printf("%s = 0;\n", scratchName)
	if err := g.writeCoroSuspPoint(b, false); err != nil {
		return err
	}
	b.printf("while (true) {\n")

	b.printf("if (WUFFS_BASE__UNLIKELY(iop_%s == io2_%s)) {\n"+
		"status = wuffs_base__make_status(wuffs_base__suspension__short_read);\ngoto suspend;\n}\n",
		preName, preName)

	b.printf("uint64_t* scratch = &%s;\n", scratchName)
	b.printf("uint32_t num_bits_%d = ((uint32_t)(*scratch", temp)
	switch endianness {
	case 'b':
		b.writes(" & 0xFF));\n*scratch >>= 8;\n*scratch <<= 8;\n")
		b.printf("*scratch |= ((uint64_t)(*%s%s++)) << (56 - num_bits_%d);\n",
			iopPrefix, preName, temp)
	case 'l':
		b.writes(" >> 56));\n*scratch <<= 8;\n*scratch >>= 8;\n")
		b.printf("*scratch |= ((uint64_t)(*%s%s++)) << num_bits_%d;\n",
			iopPrefix, preName, temp)
	}

	b.printf("if (num_bits_%d == %d) {\n%s%d = ((uint%d_t)(", temp, xx-8, tPrefix, temp, yy)
	switch endianness {
	case 'b':
		b.printf("*scratch >> %d));\n", 64-xx)
	case 'l':
		b.printf("*scratch));\n")
	}
	b.printf("break;\n")
	b.printf("}\n")

	b.printf("num_bits_%d += 8;\n", temp)
	switch endianness {
	case 'b':
		b.printf("*scratch |= ((uint64_t)(num_bits_%d));\n", temp)
	case 'l':
		b.printf("*scratch |= ((uint64_t)(num_bits_%d)) << 56;\n", temp)
	}

	b.writes("}\n}\n")
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

const writeFastMethodsBase = t.IDWriteU8Fast

var writeFastMethods = [...]struct {
	n          uint8
	endianness uint8
}{
	t.IDWriteU8Fast - writeFastMethodsBase:    {8, 'b'},
	t.IDWriteU16BEFast - writeFastMethodsBase: {16, 'b'},
	t.IDWriteU16LEFast - writeFastMethodsBase: {16, 'l'},
	t.IDWriteU24BEFast - writeFastMethodsBase: {24, 'b'},
	t.IDWriteU24LEFast - writeFastMethodsBase: {24, 'l'},
	t.IDWriteU32BEFast - writeFastMethodsBase: {32, 'b'},
	t.IDWriteU32LEFast - writeFastMethodsBase: {32, 'l'},
	t.IDWriteU40BEFast - writeFastMethodsBase: {40, 'b'},
	t.IDWriteU40LEFast - writeFastMethodsBase: {40, 'l'},
	t.IDWriteU48BEFast - writeFastMethodsBase: {48, 'b'},
	t.IDWriteU48LEFast - writeFastMethodsBase: {48, 'l'},
	t.IDWriteU56BEFast - writeFastMethodsBase: {56, 'b'},
	t.IDWriteU56LEFast - writeFastMethodsBase: {56, 'l'},
	t.IDWriteU64BEFast - writeFastMethodsBase: {64, 'b'},
	t.IDWriteU64LEFast - writeFastMethodsBase: {64, 'l'},
}
