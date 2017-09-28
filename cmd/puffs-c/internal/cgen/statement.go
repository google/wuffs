// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package cgen

import (
	"fmt"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

func (g *gen) writeStatement(b *buffer, n *a.Node, depth uint32) error {
	if depth > a.MaxBodyDepth {
		return fmt.Errorf("body recursion depth too large")
	}
	depth++

	switch n.Kind() {
	case a.KAssert:
		// Assertions only apply at compile-time.
		return nil

	case a.KAssign:
		n := n.Assign()
		if err := g.writeSuspendibles(b, n.LHS(), depth); err != nil {
			return err
		}
		if err := g.writeSuspendibles(b, n.RHS(), depth); err != nil {
			return err
		}
		if err := g.writeExpr(b, n.LHS(), replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		// TODO: does KeyAmpHatEq need special consideration?
		b.writes(cOpNames[0xFF&n.Operator().Key()])
		if err := g.writeExpr(b, n.RHS(), replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")
		return nil

	case a.KExpr:
		n := n.Expr()
		if err := g.writeSuspendibles(b, n, depth); err != nil {
			return err
		}
		if n.CallSuspendible() {
			return nil
		}
		// TODO: delete this hack that only matches "foo.set_literal_width(etc)".
		if isThatMethod(g.tm, n, g.tm.ByName("set_literal_width").Key(), 1) {
			b.printf("%slzw_decoder_set_literal_width(&self->private_impl.f_lzw, ", g.pkgPrefix)
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, a, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writes(");\n")
			return nil
		}
		if err := g.writeExpr(b, n, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")
		return nil

	case a.KIf:
		// TODO: for writeSuspendibles, make sure that we get order of
		// sub-expression evaluation correct.
		n, nCloseCurly := n.If(), 1
		for first := true; ; first = false {
			if n.Condition().Suspendible() {
				if !first {
					b.writeb('{')
					const maxCloseCurly = 1000
					if nCloseCurly == maxCloseCurly {
						return fmt.Errorf("too many nested if's")
					}
					nCloseCurly++
				}
				if err := g.writeSuspendibles(b, n.Condition(), depth); err != nil {
					return err
				}
			}

			b.writes("if (")
			if err := g.writeExpr(b, n.Condition(), replaceCallSuspendibles, parenthesesOptional, 0); err != nil {
				return err
			}
			b.writes(") {\n")
			for _, o := range n.BodyIfTrue() {
				if err := g.writeStatement(b, o, depth); err != nil {
					return err
				}
			}
			if bif := n.BodyIfFalse(); len(bif) > 0 {
				b.writes("} else {")
				for _, o := range bif {
					if err := g.writeStatement(b, o, depth); err != nil {
						return err
					}
				}
				break
			}
			n = n.ElseIf()
			if n == nil {
				break
			}
			b.writes("} else ")
		}
		for ; nCloseCurly > 0; nCloseCurly-- {
			b.writes("}\n")
		}
		return nil

	case a.KJump:
		n := n.Jump()
		jt, err := g.currFunk.jumpTarget(n.JumpTarget())
		if err != nil {
			return err
		}
		keyword := "continue"
		if n.Keyword().Key() == t.KeyBreak {
			keyword = "break"
		}
		b.printf("goto label_%d_%s;\n", jt, keyword)
		return nil

	case a.KReturn:
		n := n.Return()
		ret := status{}
		retExpr := (*a.Expr)(nil)
		if n.Keyword() != 0 {
			ret = g.statusMap[n.Message()]
		} else if retExpr = n.Value(); retExpr == nil {
			ret.name = fmt.Sprintf("%sSTATUS_OK", g.PKGPREFIX)
		}
		if g.currFunk.suspendible {
			b.writes("status = ")
			if retExpr == nil {
				b.writes(ret.name)
				// TODO: check that retExpr has no call-suspendibles.
			} else if err := g.writeExpr(
				b, retExpr, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writes(";")
			if ret.isError {
				b.writes("goto exit;")
			} else {
				b.writes("goto suspend;")
			}
		} else {
			// TODO: consider the return values, especially if they involve
			// suspendible function calls.
			b.writes("return;\n")
		}
		return nil

	case a.KVar:
		n := n.Var()
		if v := n.Value(); v != nil {
			if err := g.writeSuspendibles(b, v, depth); err != nil {
				return err
			}
			if v.ID0().Key() == t.KeyLimit {
				g.currFunk.limitVarName = n.Name().String(g.tm)
				b.printf("%s%s =", lPrefix, g.currFunk.limitVarName)
				if err := g.writeExpr(
					b, v.LHS().Expr(), replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
					return err
				}
				b.writes(";\n")
			}
		}
		switch n.XType().Decorator().Key() {
		case t.KeyOpenBracket:
			if n.Value() != nil {
				// TODO: something like:
				// cv := n.XType().ArrayLength().ConstValue()
				// // TODO: check that cv is within size_t's range.
				// g.printf("{ size_t i; for (i = 0; i < %d; i++) { %s%s[i] = $DEFAULT_VALUE; }}\n",
				// cv, vPrefix, n.Name().String(g.tm))
				return fmt.Errorf("TODO: array initializers for non-zero default values")
			}
			// TODO: arrays of arrays.
			name := n.Name().String(g.tm)
			b.printf("memset(%s%s, 0, sizeof(%s%s));\n", vPrefix, name, vPrefix, name)

		default:
			b.printf("%s%s = ", vPrefix, n.Name().String(g.tm))
			if v := n.Value(); v != nil {
				if err := g.writeExpr(b, v, replaceCallSuspendibles, parenthesesMandatory, 0); err != nil {
					return err
				}
			} else if n.XType().Decorator().Key() == t.KeyColon {
				// TODO: don't assume that the slice is a slice of u8.
				b.printf("((puffs_base_slice_u8){})")
			} else {
				b.writeb('0')
			}
			b.writes(";\n")
		}
		return nil

	case a.KWhile:
		n := n.While()
		// TODO: consider suspendible calls.

		if n.HasContinue() {
			jt, err := g.currFunk.jumpTarget(n)
			if err != nil {
				return err
			}
			b.printf("label_%d_continue:;\n", jt)
		}
		b.writes("while (")
		if err := g.writeExpr(b, n.Condition(), replaceCallSuspendibles, parenthesesOptional, 0); err != nil {
			return err
		}
		b.writes(") {\n")
		for _, o := range n.Body() {
			if err := g.writeStatement(b, o, depth); err != nil {
				return err
			}
		}
		b.writes("}\n")
		if n.HasBreak() {
			jt, err := g.currFunk.jumpTarget(n)
			if err != nil {
				return err
			}
			b.printf("label_%d_break:;\n", jt)
		}
		return nil

	}
	return fmt.Errorf("unrecognized ast.Kind (%s) for writeStatement", n.Kind())
}

func (g *gen) writeCoroSuspPoint(b *buffer) error {
	const maxCoroSuspPoint = 0xFFFFFFFF
	g.currFunk.coroSuspPoint++
	if g.currFunk.coroSuspPoint == maxCoroSuspPoint {
		return fmt.Errorf("too many coroutine suspension points required")
	}

	b.printf("PUFFS_COROUTINE_SUSPENSION_POINT(%d);\n", g.currFunk.coroSuspPoint)
	return nil
}

func (g *gen) writeSuspendibles(b *buffer, n *a.Expr, depth uint32) error {
	if !n.Suspendible() {
		return nil
	}
	if err := g.writeCoroSuspPoint(b); err != nil {
		return err
	}
	return g.writeCallSuspendibles(b, n, depth)
}

func (g *gen) writeCallSuspendibles(b *buffer, n *a.Expr, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("expression recursion depth too large")
	}
	depth++

	// The evaluation order for suspendible calls (which can have side effects)
	// is important here: LHS, MHS, RHS, Args and finally the node itself.
	if !n.CallSuspendible() {
		for _, o := range n.Node().Raw().SubNodes() {
			if o != nil && o.Kind() == a.KExpr {
				if err := g.writeCallSuspendibles(b, o.Expr(), depth); err != nil {
					return err
				}
			}
		}
		for _, o := range n.Args() {
			if o != nil && o.Kind() == a.KExpr {
				if err := g.writeCallSuspendibles(b, o.Expr(), depth); err != nil {
					return err
				}
			}
		}
		return nil
	}

	if err := g.writeSaveExprDerivedVars(b, n); err != nil {
		return err
	}

	// TODO: delete these hacks that only matches "in.src.read_u8?()" etc.
	//
	// TODO: check reader1.buf and writer1.buf is non-NULL.
	if isInSrc(g.tm, n, t.KeyReadU8, 0) {
		if g.currFunk.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.currFunk.tempW
		g.currFunk.tempW++

		b.printf("if (PUFFS_UNLIKELY(%srptr_src == %srend_src)) { goto short_read_src; }", bPrefix, bPrefix)
		g.currFunk.shortReads = append(g.currFunk.shortReads, "src")
		// TODO: watch for passing an array type to writeCTypeName? In C, an
		// array type can decay into a pointer.
		if err := g.writeCTypeName(b, n.MType(), tPrefix, fmt.Sprint(temp)); err != nil {
			return err
		}
		b.printf(" = *%srptr_src++;\n", bPrefix)

	} else if isInSrc(g.tm, n, t.KeyReadU16BE, 0) {
		return g.writeReadUXX(b, n, "src", 16, "be")

	} else if isInSrc(g.tm, n, t.KeyReadU16LE, 0) {
		return g.writeReadUXX(b, n, "src", 16, "le")

	} else if isInSrc(g.tm, n, t.KeyReadU32BE, 0) {
		return g.writeReadUXX(b, n, "src", 32, "be")

	} else if isInSrc(g.tm, n, t.KeyReadU32LE, 0) {
		return g.writeReadUXX(b, n, "src", 32, "le")

	} else if isInSrc(g.tm, n, t.KeySkip32, 1) {
		g.currFunk.usesScratch = true
		// TODO: don't hard-code [0], and allow recursive coroutines.
		scratchName := fmt.Sprintf("self->private_impl.%s%s[0].scratch",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))

		b.printf("%s = ", scratchName)
		x := n.Args()[0].Arg().Value()
		if err := g.writeExpr(b, x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")

		// TODO: the CSP prior to this is probably unnecessary.
		if err := g.writeCoroSuspPoint(b); err != nil {
			return err
		}

		b.printf("if (%s > %srend_src - %srptr_src) {\n", scratchName, bPrefix, bPrefix)
		b.printf("%s -= %srend_src - %srptr_src;\n", scratchName, bPrefix, bPrefix)
		b.printf("%srptr_src = %srend_src;\n", bPrefix, bPrefix)

		// TODO: is ptr_to_len the right check?
		b.printf("if (%ssrc.limit.ptr_to_len) {", aPrefix)
		b.printf("status = %sSUSPENSION_LIMITED_READ;", g.PKGPREFIX)
		b.printf("} else if (%ssrc.buf && %ssrc.buf->closed) {", aPrefix, aPrefix)
		b.printf("status = %sERROR_UNEXPECTED_EOF; goto exit;", g.PKGPREFIX)
		b.printf("} else { status = %sSUSPENSION_SHORT_READ; } goto suspend;\n", g.PKGPREFIX)

		b.writes("}\n")
		b.printf("%srptr_src += %s;\n", bPrefix, scratchName)

	} else if isInDst(g.tm, n, t.KeyWrite, 1) {
		// TODO: don't assume that the argument is "this.stack[s:]".
		b.printf("if (%sdst.buf && %sdst.buf->closed) { status = %sERROR_CLOSED_FOR_WRITES;",
			aPrefix, aPrefix, g.PKGPREFIX)
		b.writes("goto exit;")
		b.writes("}\n")
		b.printf("if ((%swend_dst - %swptr_dst) < (sizeof(self->private_impl.f_stack) - v_s)) {",
			bPrefix, bPrefix)
		b.printf("status = %sSUSPENSION_SHORT_WRITE;", g.PKGPREFIX)
		b.writes("goto suspend;")
		b.writes("}\n")
		b.printf("memmove(b_wptr_dst," +
			"self->private_impl.f_stack + v_s," +
			"sizeof(self->private_impl.f_stack) - v_s);\n")
		b.printf("b_wptr_dst += sizeof(self->private_impl.f_stack) - v_s;\n")

	} else if isInDst(g.tm, n, t.KeyWriteU8, 1) {
		b.printf("if (%swptr_dst == %swend_dst) { status = %sSUSPENSION_SHORT_WRITE;",
			bPrefix, bPrefix, g.PKGPREFIX)
		b.writes("goto suspend;")
		b.writes("}\n")
		b.printf("*%swptr_dst++ = ", bPrefix)
		x := n.Args()[0].Arg().Value()
		if err := g.writeExpr(b, x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")

	} else if isInDst(g.tm, n, t.KeyCopyFrom32, 2) {
		if g.currFunk.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.currFunk.tempW
		g.currFunk.tempW++
		g.currFunk.tempR++

		b.writes("{\n")

		g.currFunk.usesScratch = true
		// TODO: don't hard-code [0], and allow recursive coroutines.
		scratchName := fmt.Sprintf("self->private_impl.%s%s[0].scratch",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))

		b.printf("%s = ", scratchName)
		x := n.Args()[1].Arg().Value()
		if err := g.writeExpr(b, x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")

		if err := g.writeCoroSuspPoint(b); err != nil {
			return err
		}
		b.printf("size_t %s%d = %s;\n", tPrefix, temp, scratchName)

		const wName = "dst"
		b.printf("if (%s%d > %swend_%s - %swptr_%s) {\n", tPrefix, temp, bPrefix, wName, bPrefix, wName)
		b.printf("%s%d = %swend_%s - %swptr_%s;\n", tPrefix, temp, bPrefix, wName, bPrefix, wName)
		b.printf("status = %sSUSPENSION_SHORT_WRITE;\n", g.PKGPREFIX)
		b.writes("}\n")

		// TODO: don't assume that the first argument is "in.src".
		const rName = "src"
		b.printf("if (%s%d > %srend_%s - %srptr_%s) {\n", tPrefix, temp, bPrefix, rName, bPrefix, rName)
		b.printf("%s%d = %srend_%s - %srptr_%s;\n", tPrefix, temp, bPrefix, rName, bPrefix, rName)
		b.printf("status = %sSUSPENSION_SHORT_READ;\n", g.PKGPREFIX)
		b.writes("}\n")

		b.printf("memmove(%swptr_%s, %srptr_%s, %s%d);\n", bPrefix, wName, bPrefix, rName, tPrefix, temp)
		b.printf("%swptr_%s += %s%d;\n", bPrefix, wName, tPrefix, temp)
		b.printf("%srptr_%s += %s%d;\n", bPrefix, rName, tPrefix, temp)
		b.printf("if (status) { %s -= %s%d; goto suspend; }\n", scratchName, tPrefix, temp)

		b.writes("}\n")

	} else if isInDst(g.tm, n, t.KeyCopyHistory32, 2) {
		if g.currFunk.tempW > maxTemp-2 {
			return fmt.Errorf("too many temporary variables required")
		}
		temp0 := g.currFunk.tempW + 0
		temp1 := g.currFunk.tempW + 1
		temp2 := g.currFunk.tempW + 2
		g.currFunk.tempW += 3
		g.currFunk.tempR += 3

		b.writes("{\n")

		g.currFunk.usesScratch = true
		// TODO: don't hard-code [0], and allow recursive coroutines.
		scratchName := fmt.Sprintf("self->private_impl.%s%s[0].scratch",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))

		b.printf("%s = ((uint64_t)(", scratchName)
		x0 := n.Args()[0].Arg().Value()
		if err := g.writeExpr(b, x0, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(")<<32) | (uint64_t)(")
		x1 := n.Args()[1].Arg().Value()
		if err := g.writeExpr(b, x1, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(");\n")
		if err := g.writeCoroSuspPoint(b); err != nil {
			return err
		}

		const wName = "dst"
		b.printf("size_t %s%d = (size_t)(%s >> 32);", tPrefix, temp0, scratchName)
		// TODO: it's not a BAD_ARGUMENT if we can copy from the sliding window.
		//
		// TODO: %s%s.buf->ptr might be a NULL pointer dereference. In any
		// case, we need to compare the (resumed) distance against a mark, not
		// against the beginning of the dst buffer.
		b.printf("if (PUFFS_UNLIKELY((%s%d == 0) || (%s%d > (%swptr_%s - %s%s.buf->ptr)))) { "+
			"status = %sERROR_BAD_ARGUMENT; goto exit; }\n",
			tPrefix, temp0, tPrefix, temp0, bPrefix, wName, aPrefix, wName, g.PKGPREFIX)
		b.printf("uint8_t* %s%d = %swptr_%s - %s%d;\n", tPrefix, temp1, bPrefix, wName, tPrefix, temp0)
		b.printf("uint32_t %s%d = (uint32_t)(%s);", tPrefix, temp2, scratchName)

		b.printf("if (PUFFS_LIKELY((size_t)(%s%d) <= (%swend_%s - %swptr_%s))) {",
			tPrefix, temp2, bPrefix, wName, bPrefix, wName)

		b.printf("for (; %s%d >= 8; %s%d -= 8) {", tPrefix, temp2, tPrefix, temp2)
		for i := 0; i < 8; i++ {
			b.printf("*%swptr_%s++ = *%s%d++;\n", bPrefix, wName, tPrefix, temp1)
		}
		b.writes("}\n")
		b.printf("for (; %s%d; %s%d--) { *%swptr_%s++ = *%s%d++; }\n",
			tPrefix, temp2, tPrefix, temp2, bPrefix, wName, tPrefix, temp1)

		b.writes("} else {\n")

		b.printf("%s%d = (uint32_t)(%swend_%s - %swptr_%s);\n",
			tPrefix, temp2, bPrefix, wName, bPrefix, wName)
		b.printf("%s -= (uint64_t)(%s%d);\n", scratchName, tPrefix, temp2)
		b.printf("for (; %s%d; %s%d--) { *%swptr_%s++ = *%s%d++; }\n",
			tPrefix, temp2, tPrefix, temp2, bPrefix, wName, tPrefix, temp1)
		// TODO: SHORT_WRITE vs LIMITED_WRITE?
		b.printf("status = %sSUSPENSION_SHORT_WRITE; goto suspend;\n", g.PKGPREFIX)

		b.writes("}\n")

		b.writes("}\n")

	} else if isThisMethod(g.tm, n, "decode_header", 1) {
		b.printf("status = %s%s_decode_header(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_lsd", 1) {
		b.printf("status = %s%s_decode_lsd(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_extension", 1) {
		b.printf("status = %s%s_decode_extension(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_id", 2) {
		b.printf("status = %s%s_decode_id(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_blocks", 2) {
		// TODO: don't hard code being inside a try call.
		if g.currFunk.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.currFunk.tempW
		g.currFunk.tempW++

		b.printf("%sstatus %s%d = %s%s_decode_blocks(self, %sdst, %ssrc);\n",
			g.pkgPrefix, tPrefix, temp,
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}

	} else if isThisMethod(g.tm, n, "decode_uncompressed", 2) {
		b.printf("status = %s%s_decode_uncompressed(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_huffman", 2) {
		b.printf("status = %s%s_decode_huffman(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "init_fixed_huffman", 0) {
		b.printf("status = %s%s_init_fixed_huffman(self);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm))
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "init_dynamic_huffman", 1) {
		b.printf("status = %s%s_init_dynamic_huffman(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "init_huff", 4) {
		b.printf("status = %s%s_init_huff(self,",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm))
		for i, o := range n.Args() {
			if i != 0 {
				b.writes(",")
			}
			if err := g.writeExpr(b, o.Arg().Value(), replaceNothing, parenthesesMandatory, depth); err != nil {
				return err
			}
		}
		b.writes(");\n")
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThatMethod(g.tm, n, g.tm.ByName("decode").Key(), 2) {
		switch g.pkgName {
		case "flate":
			b.printf("status = %sdecoder_decode(&self->private_impl.f_dec, %sdst, %ssrc);\n",
				g.pkgPrefix, aPrefix, aPrefix)
			if err := g.writeLoadExprDerivedVars(b, n); err != nil {
				return err
			}
			b.writes("if (status) { goto suspend; }\n")
		case "gif":
			b.printf("status = %slzw_decoder_decode(&self->private_impl.f_lzw, %sdst, %s%s);\n",
				g.pkgPrefix, aPrefix, vPrefix, n.Args()[1].Arg().Value().String(g.tm))
			if err := g.writeLoadExprDerivedVars(b, n); err != nil {
				return err
			}
			// TODO: be principled with "if (l_lzw_src && etc)".
			b.writes("if (l_lzw_src && status) { goto suspend; }\n")
		default:
			return fmt.Errorf("cannot convert Puffs call %q to C", n.String(g.tm))
		}

	} else {
		// TODO: fix this.
		//
		// This might involve calling g.writeExpr with replaceNothing??
		return fmt.Errorf("cannot convert Puffs call %q to C", n.String(g.tm))
	}
	return nil
}

func (g *gen) writeReadUXX(b *buffer, n *a.Expr, name string, size uint32, endianness string) error {
	if size != 16 && size != 32 {
		return fmt.Errorf("internal error: bad writeReadUXX size %d", size)
	}
	if endianness != "be" && endianness != "le" {
		return fmt.Errorf("internal error: bad writeReadUXX endianness %q", endianness)
	}

	if g.currFunk.tempW > maxTemp-1 {
		return fmt.Errorf("too many temporary variables required")
	}
	// temp0 is read by code generated in this function. temp1 is read elsewhere.
	temp0 := g.currFunk.tempW + 0
	temp1 := g.currFunk.tempW + 1
	g.currFunk.tempW += 2
	g.currFunk.tempR += 1

	if err := g.writeCTypeName(b, n.MType(), tPrefix, fmt.Sprint(temp1)); err != nil {
		return err
	}
	b.writes(";")

	g.currFunk.usesScratch = true
	// TODO: don't hard-code [0], and allow recursive coroutines.
	scratchName := fmt.Sprintf("self->private_impl.%s%s[0].scratch",
		cPrefix, g.currFunk.astFunc.Name().String(g.tm))

	b.printf("if (PUFFS_LIKELY(%srend_src - %srptr_src >= %d)) {", bPrefix, bPrefix, size/8)
	b.printf("%s%d = puffs_base_load_u%d%s(%srptr_src);\n", tPrefix, temp1, size, endianness, bPrefix)
	b.printf("%srptr_src += %d;\n", bPrefix, size/8)
	b.printf("} else {")
	b.printf("%s = 0;\n", scratchName)
	if err := g.writeCoroSuspPoint(b); err != nil {
		return err
	}
	b.printf("while (true) {")

	b.printf("if (PUFFS_UNLIKELY(%srptr_%s == %srend_%s)) { goto short_read_%s; }",
		bPrefix, name, bPrefix, name, name)
	g.currFunk.shortReads = append(g.currFunk.shortReads, name)

	b.printf("uint32_t %s%d = %s", tPrefix, temp0, scratchName)
	switch endianness {
	case "be":
		b.printf("& 0xFF;")
		b.printf("%s >>= 8;", scratchName)
		b.printf("%s <<= 8;", scratchName)
		b.printf("%s |= ((uint64_t)(*%srptr_%s++)) << (64 - %s%d);",
			scratchName, bPrefix, name, tPrefix, temp0)
	case "le":
		b.printf(">> 56;")
		b.printf("%s <<= 8;", scratchName)
		b.printf("%s >>= 8;", scratchName)
		b.printf("%s |= ((uint64_t)(*%srptr_%s++)) << %s%d;",
			scratchName, bPrefix, name, tPrefix, temp0)
	}

	b.printf("if (%s%d == %d) {", tPrefix, temp0, size-8)
	switch endianness {
	case "be":
		b.printf("%s%d = %s >> (64 - %d);", tPrefix, temp1, scratchName, size)
	case "le":
		b.printf("%s%d = %s;", tPrefix, temp1, scratchName)
	}
	b.printf("break;")
	b.printf("}")

	b.printf("%s%d += 8;", tPrefix, temp0)
	switch endianness {
	case "be":
		b.printf("%s |= ((uint64_t)(%s%d));", scratchName, tPrefix, temp0)
	case "le":
		b.printf("%s |= ((uint64_t)(%s%d)) << 56;", scratchName, tPrefix, temp0)
	}

	b.writes("}}\n")
	return nil
}

func isInSrc(tm *t.Map, n *a.Expr, methodName t.Key, nArgs int) bool {
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1().Key() != methodName {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName("src") {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyIn
}

func isInDst(tm *t.Map, n *a.Expr, methodName t.Key, nArgs int) bool {
	callSuspendible := methodName != t.KeyMark && methodName != t.KeySlice
	// TODO: check that n.Args() is "(x:bar)".
	if n.ID0().Key() != t.KeyOpenParen || n.CallSuspendible() != callSuspendible || len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1().Key() != methodName {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName("dst") {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyIn
}

func isThisMethod(tm *t.Map, n *a.Expr, methodName string, nArgs int) bool {
	// TODO: check that n.Args() is "(src:in.src)".
	if k := n.ID0().Key(); k != t.KeyOpenParen && k != t.KeyTry {
		return false
	}
	if len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName(methodName) {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyThis
}

// isThatMethod is like isThisMethod but for foo.bar(etc), not this.bar(etc).
func isThatMethod(tm *t.Map, n *a.Expr, methodName t.Key, nArgs int) bool {
	if k := n.ID0().Key(); k != t.KeyOpenParen && k != t.KeyTry {
		return false
	}
	if len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1().Key() == methodName
}
