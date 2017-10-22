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
	"strings"

	"github.com/google/puffs/lang/builtin"

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
		if err := g.writeExpr(b, n, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")
		return nil

	case a.KIO:
		b.writes("/* placeholder */\n")
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
			if ret.name == "" {
				msg := builtin.TrimQuotes(n.Message().String(g.tm))
				z := builtin.StatusMap[msg]
				if z.Message == "" {
					return fmt.Errorf("no status code for %q", msg)
				}
				ret.name = strings.ToUpper(g.cName(z.String()))
				ret.keyword = z.Keyword
			}
		} else if retExpr = n.Value(); retExpr == nil {
			ret.name = g.PKGPREFIX + "STATUS_OK"
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
			if retExpr == nil {
				switch ret.keyword.Key() {
				case 0:
					b.writes("goto ok;")
					return nil
				case t.KeyError:
					b.writes("goto exit;")
					return nil
				case t.KeySuspension:
					// No-op.
				}
			}
			return g.writeCoroSuspPoint(b, true)
		}

		b.writes("return ")
		if len(g.currFunk.astFunc.Out().Fields()) == 0 {
			if retExpr != nil {
				return fmt.Errorf("return expression %q incompatible with empty return type", retExpr.String(g.tm))
			}
		} else if retExpr == nil {
			// TODO: should a bare "return" imply "return out"?
			return fmt.Errorf("empty return expression incompatible with non-empty return type")
		} else if err := g.writeExpr(b, retExpr, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writeb(';')
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
				b.printf("((puffs_base__slice_u8){})")
			} else {
				b.writeb('0')
			}
			b.writes(";\n")
		}
		return nil

	case a.KIterate:
		n := n.Iterate()
		vars := n.Variables()
		if len(vars) == 0 {
			return nil
		}
		if len(vars) != 1 {
			return fmt.Errorf("TODO: iterate over more than one variable")
		}
		v := vars[0].Var()
		name := v.Name().String(g.tm)
		b.writes("{\n")

		// TODO: don't assume that the slice is a slice of u8. In particular,
		// the code gen can be subtle if the slice element type has zero size,
		// such as the empty struct.
		b.printf("puffs_base__slice_u8 %sslice_%s =", iPrefix, name)
		if err := g.writeExpr(b, v.Value(), replaceCallSuspendibles, parenthesesOptional, 0); err != nil {
			return err
		}
		b.writes(";\n")
		b.printf("uint8_t* %s%s = %sslice_%s.ptr;\n", vPrefix, name, iPrefix, name)
		// TODO: look at n.HasContinue() and n.HasBreak().

		unrollCount := int(n.UnrollCount().ConstValue().Int64())
		if unrollCount != 1 {
			b.printf("uint8_t* %send0_%s = %sslice_%s.ptr + (%sslice_%s.len / %d) * %d;\n",
				iPrefix, name, iPrefix, name, iPrefix, name, unrollCount, unrollCount)
			b.printf("while (%s%s < %send0_%s) {\n", vPrefix, name, iPrefix, name)
			for i := 0; i < unrollCount; i++ {
				for _, o := range n.Body() {
					if err := g.writeStatement(b, o, depth); err != nil {
						return err
					}
				}
				b.printf("%s%s++;\n", vPrefix, name)
			}
			b.writes("}\n")
		}

		b.printf("uint8_t* %send1_%s = %sslice_%s.ptr + %sslice_%s.len;\n",
			iPrefix, name, iPrefix, name, iPrefix, name)
		b.printf("while (%s%s < %send1_%s) {\n", vPrefix, name, iPrefix, name)
		for _, o := range n.Body() {
			if err := g.writeStatement(b, o, depth); err != nil {
				return err
			}
		}
		b.printf("%s%s++;\n", vPrefix, name)
		b.writes("}\n")

		b.writes("}\n")
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

func (g *gen) writeCoroSuspPoint(b *buffer, maybeSuspend bool) error {
	const maxCoroSuspPoint = 0xFFFFFFFF
	g.currFunk.coroSuspPoint++
	if g.currFunk.coroSuspPoint == maxCoroSuspPoint {
		return fmt.Errorf("too many coroutine suspension points required")
	}

	macro := ""
	if maybeSuspend {
		macro = "_MAYBE_SUSPEND"
	}
	b.printf("PUFFS_BASE__COROUTINE_SUSPENSION_POINT%s(%d);\n", macro, g.currFunk.coroSuspPoint)
	return nil
}

func (g *gen) writeSuspendibles(b *buffer, n *a.Expr, depth uint32) error {
	if !n.Suspendible() {
		return nil
	}
	if err := g.writeCoroSuspPoint(b, false); err != nil {
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

		b.printf("if (PUFFS_BASE__UNLIKELY(%srptr_src == %srend_src)) { goto short_read_src; }",
			bPrefix, bPrefix)
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
		if err := g.writeCoroSuspPoint(b, false); err != nil {
			return err
		}

		b.printf("if (%s > %srend_src - %srptr_src) {\n", scratchName, bPrefix, bPrefix)
		b.printf("%s -= %srend_src - %srptr_src;\n", scratchName, bPrefix, bPrefix)
		b.printf("%srptr_src = %srend_src;\n", bPrefix, bPrefix)

		b.writes("goto short_read_src; }\n")
		g.currFunk.shortReads = append(g.currFunk.shortReads, "src")
		b.printf("%srptr_src += %s;\n", bPrefix, scratchName)

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

	} else if isThisMethod(g.tm, n, "decode_header", 1) {
		b.printf("status = %s%s__decode_header(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		// TODO: should we goto exit/suspend depending on if status < or > 0?
		// Ditto below.
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_lsd", 1) {
		b.printf("status = %s%s__decode_lsd(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_extension", 1) {
		b.printf("status = %s%s__decode_extension(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_id", 2) {
		b.printf("status = %s%s__decode_id(self, %sdst, %ssrc);\n",
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

		b.printf("%sstatus %s%d = %s%s__decode_blocks(self, %sdst, %ssrc);\n",
			g.pkgPrefix, tPrefix, temp,
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		// TODO: check if tPrefix_temp is an error, and return?

	} else if isThisMethod(g.tm, n, "decode_uncompressed", 2) {
		b.printf("status = %s%s__decode_uncompressed(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_huffman", 2) {
		b.printf("status = %s%s__decode_huffman(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "init_fixed_huffman", 0) {
		b.printf("status = %s%s__init_fixed_huffman(self);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm))
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "init_dynamic_huffman", 1) {
		b.printf("status = %s%s__init_dynamic_huffman(self, %ssrc);\n",
			g.pkgPrefix, g.currFunk.astFunc.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(b, n); err != nil {
			return err
		}
		b.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "init_huff", 4) {
		b.printf("status = %s%s__init_huff(self,",
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
			// TODO: don't hard code being inside a try call.
			if g.currFunk.tempW > maxTemp {
				return fmt.Errorf("too many temporary variables required")
			}
			temp := g.currFunk.tempW
			g.currFunk.tempW++

			b.printf("%sstatus %s%d = %sflate_decoder__decode(&self->private_impl.f_flate, %sdst, %ssrc);\n",
				g.pkgPrefix, tPrefix, temp,
				g.pkgPrefix, aPrefix, aPrefix)
			if err := g.writeLoadExprDerivedVars(b, n); err != nil {
				return err
			}
			// TODO: check if tPrefix_temp is an error, and return?

		case "gif":
			b.printf("status = %slzw_decoder__decode(&self->private_impl.f_lzw, %sdst, %s%s);\n",
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

	b.printf("if (PUFFS_BASE__LIKELY(%srend_src - %srptr_src >= %d)) {", bPrefix, bPrefix, size/8)
	b.printf("%s%d = puffs_base__load_u%d%s(%srptr_src);\n", tPrefix, temp1, size, endianness, bPrefix)
	b.printf("%srptr_src += %d;\n", bPrefix, size/8)
	b.printf("} else {")
	b.printf("%s = 0;\n", scratchName)
	if err := g.writeCoroSuspPoint(b, false); err != nil {
		return err
	}
	b.printf("while (true) {")

	b.printf("if (PUFFS_BASE__UNLIKELY(%srptr_%s == %srend_%s)) { goto short_read_%s; }",
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
	callSuspendible := methodName != t.KeyCopyFromReader32 &&
		methodName != t.KeyCopyFromHistory32 &&
		methodName != t.KeyCopyFromSlice32 &&
		methodName != t.KeyCopyFromSlice &&
		methodName != t.KeySlice
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
