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
	"fmt"
	"strings"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

// genFilenameLineComments is whether to print "// foo.wuffs:123\n" comments in
// the generated code. This can be useful for debugging, although it is not
// enabled by default as it can lead to many spurious changes in the generated
// C code (due to line numbers changing) when editing Wuffs code.
const genFilenameLineComments = false

func (g *gen) writeStatement(b *buffer, n *a.Node, depth uint32) error {
	if depth > a.MaxBodyDepth {
		return fmt.Errorf("body recursion depth too large")
	}
	depth++

	if n.Kind() == a.KAssert {
		// Assertions only apply at compile-time.
		return nil
	}

	mightIntroduceTemporaries := false
	switch n.Kind() {
	case a.KAssign:
		n := n.AsAssign()
		mightIntroduceTemporaries = n.LHS().Effect().Coroutine() || n.RHS().Effect().Coroutine()
	case a.KVar:
		v := n.AsVar().Value()
		mightIntroduceTemporaries = v != nil && v.Effect().Coroutine()
	}
	if mightIntroduceTemporaries {
		// Put n's code into its own block, to restrict the scope of the
		// tPrefix temporary variables. This helps avoid "jump bypasses
		// variable initialization" compiler warnings with the coroutine
		// suspension points.
		b.writes("{\n")
		defer b.writes("}\n")
	}

	if genFilenameLineComments {
		filename, line := n.AsRaw().FilenameLine()
		if i := strings.LastIndexByte(filename, '/'); i >= 0 {
			filename = filename[i+1:]
		}
		if i := strings.LastIndexByte(filename, '\\'); i >= 0 {
			filename = filename[i+1:]
		}
		b.printf("// %s:%d\n", filename, line)
	}

	switch n.Kind() {
	case a.KAssign:
		return g.writeStatementAssign(b, n.AsAssign(), depth)
	case a.KExpr:
		return g.writeStatementExpr(b, n.AsExpr(), depth)
	case a.KIOBind:
		return g.writeStatementIOBind(b, n.AsIOBind(), depth)
	case a.KIf:
		return g.writeStatementIf(b, n.AsIf(), depth)
	case a.KIterate:
		return g.writeStatementIterate(b, n.AsIterate(), depth)
	case a.KJump:
		return g.writeStatementJump(b, n.AsJump(), depth)
	case a.KRet:
		return g.writeStatementRet(b, n.AsRet(), depth)
	case a.KVar:
		return g.writeStatementVar(b, n.AsVar(), depth)
	case a.KWhile:
		return g.writeStatementWhile(b, n.AsWhile(), depth)
	}
	return fmt.Errorf("unrecognized ast.Kind (%s) for writeStatement", n.Kind())
}

func (g *gen) writeStatementAssign(b *buffer, n *a.Assign, depth uint32) error {
	if err := g.writeSuspendibles(b, n.LHS(), depth); err != nil {
		return err
	}
	if err := g.writeSuspendibles(b, n.RHS(), depth); err != nil {
		return err
	}
	opName, tilde := "", false

	op := n.Operator()
	switch op {
	case t.IDTildeSatPlusEq, t.IDTildeSatMinusEq:
		uBits := uintBits(n.LHS().MType().QID())
		if uBits == 0 {
			return fmt.Errorf("unsupported tilde-operator type %q", n.LHS().MType().Str(g.tm))
		}
		uOp := "add"
		if op != t.IDTildeSatPlusEq {
			uOp = "sub"
		}
		b.printf("wuffs_base__u%d__sat_%s_indirect(&", uBits, uOp)
		opName, tilde = ",", true

	default:
		opName = cOpName(op)
		if opName == "" {
			return fmt.Errorf("unrecognized operator %q", op.AmbiguousForm().Str(g.tm))
		}
	}

	if err := g.writeExpr(b, n.LHS(), replaceCallSuspendibles, depth); err != nil {
		return err
	}
	b.writes(opName)
	if err := g.writeExpr(b, n.RHS(), replaceCallSuspendibles, depth); err != nil {
		return err
	}
	if tilde {
		b.writeb(')')
	}
	b.writes(";\n")
	return nil
}

func (g *gen) writeStatementExpr(b *buffer, n *a.Expr, depth uint32) error {
	if err := g.writeSuspendibles(b, n, depth); err != nil {
		return err
	}
	if n.Effect().RootCause() && n.Effect().Coroutine() {
		return nil
	}
	if err := g.writeExpr(b, n, replaceCallSuspendibles, depth); err != nil {
		return err
	}
	b.writes(";\n")
	return nil
}

func (g *gen) writeStatementIOBind(b *buffer, n *a.IOBind, depth uint32) error {
	inFields := n.InFields()

	if g.currFunk.ioBinds > maxIOBinds || len(inFields) > maxIOBindInFields {
		return fmt.Errorf("too many temporary variables required")
	}
	ioBindNum := g.currFunk.ioBinds
	g.currFunk.ioBinds++

	// TODO: do these variables need to be func-scoped (bigger scope)
	// instead of block-scoped (smaller scope) if the coro_susp_point
	// switch can jump past this initialization??
	b.writes("{\n")
	for i := 0; i < len(inFields); i++ {
		e := inFields[i].AsExpr()
		prefix := vPrefix
		if e.Operator() != 0 {
			prefix = aPrefix
		}
		cTyp := "reader"
		if e.MType().QID()[1] == t.IDIOWriter {
			cTyp = "writer"
		}
		name := e.Ident().Str(g.tm)
		b.printf("wuffs_base__io_%s %s%d_%s%s = %s%s;\n",
			cTyp, oPrefix, ioBindNum, prefix, name, prefix, name)

		// TODO: save / restore all iop vars, not just for local IO vars? How
		// does this work if the io_bind body advances these pointers, either
		// directly or by calling other funcs?
		if e.Operator() == 0 {
			b.printf("uint8_t *%s%d_%s%s%s = %s%s%s;\n",
				oPrefix, ioBindNum, iopPrefix, prefix, name, iopPrefix, prefix, name)
			b.printf("uint8_t *%s%d_%s%s%s = %s%s%s;\n",
				oPrefix, ioBindNum, io1Prefix, prefix, name, io1Prefix, prefix, name)
		}
	}

	for _, o := range n.Body() {
		if err := g.writeStatement(b, o, depth); err != nil {
			return err
		}
	}

	for i := len(inFields) - 1; i >= 0; i-- {
		e := inFields[i].AsExpr()
		prefix := vPrefix
		if e.Operator() != 0 {
			prefix = aPrefix
		}
		name := e.Ident().Str(g.tm)
		b.printf("%s%s = %s%d_%s%s;\n",
			prefix, name, oPrefix, ioBindNum, prefix, name)
		if e.Operator() == 0 {
			b.printf("%s%s%s = %s%d_%s%s%s;\n",
				iopPrefix, prefix, name, oPrefix, ioBindNum, iopPrefix, prefix, name)
			b.printf("%s%s%s = %s%d_%s%s%s;\n",
				io1Prefix, prefix, name, oPrefix, ioBindNum, io1Prefix, prefix, name)
		}
	}
	b.writes("}\n")
	return nil
}

func (g *gen) writeStatementIf(b *buffer, n *a.If, depth uint32) error {
	for {
		condition := buffer(nil)
		if err := g.writeExpr(&condition, n.Condition(), replaceCallSuspendibles, 0); err != nil {
			return err
		}
		// Calling trimParens avoids clang's -Wparentheses-equality warning.
		b.printf("if (%s) {\n", trimParens(condition))
		for _, o := range n.BodyIfTrue() {
			if err := g.writeStatement(b, o, depth); err != nil {
				return err
			}
		}
		if bif := n.BodyIfFalse(); len(bif) > 0 {
			b.writes("} else {\n")
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
	b.writes("}\n")
	return nil
}

func (g *gen) writeStatementIterate(b *buffer, n *a.Iterate, depth uint32) error {
	vars := n.Variables()
	if len(vars) == 0 {
		return nil
	}
	if len(vars) != 1 {
		return fmt.Errorf("TODO: iterate over more than one variable")
	}
	v := vars[0].AsVar()
	name := v.Name().Str(g.tm)
	b.writes("{\n")

	// TODO: don't assume that the slice is a slice of base.u8. In
	// particular, the code gen can be subtle if the slice element type has
	// zero size, such as the empty struct.
	b.printf("wuffs_base__slice_u8 %sslice_%s =", iPrefix, name)
	if err := g.writeExpr(b, v.Value(), replaceCallSuspendibles, 0); err != nil {
		return err
	}
	b.writes(";\n")
	b.printf("wuffs_base__slice_u8 %s%s = %sslice_%s;\n", vPrefix, name, iPrefix, name)
	// TODO: look at n.HasContinue() and n.HasBreak().

	round := uint32(0)
	for ; n != nil; n = n.ElseIterate() {
		length := n.Length().SmallPowerOf2Value()
		unroll := n.Unroll().SmallPowerOf2Value()
		for {
			if err := g.writeIterateRound(b, name, n.Body(), round, depth, length, unroll); err != nil {
				return err
			}
			round++

			if unroll == 1 {
				break
			}
			unroll = 1
		}
	}

	b.writes("}\n")
	return nil
}

func (g *gen) writeStatementJump(b *buffer, n *a.Jump, depth uint32) error {
	jt, err := g.currFunk.jumpTarget(n.JumpTarget())
	if err != nil {
		return err
	}
	keyword := "continue"
	if n.Keyword() == t.IDBreak {
		keyword = "break"
	}
	b.printf("goto label_%d_%s;\n", jt, keyword)
	return nil
}

func (g *gen) writeStatementRet(b *buffer, n *a.Ret, depth uint32) error {
	retExpr := n.Value()

	if g.currFunk.suspendible {
		isError, isOK := false, false
		b.writes("status = ")
		if retExpr == nil {
			b.writes("NULL")
			isOK = true
		} else {
			if retExpr.Operator() == t.IDStatus {
				msg, _ := t.Unescape(retExpr.Ident().Str(g.tm))
				isError = statusMsgIsError(msg)
				isOK = statusMsgIsWarning(msg)
			}
			// TODO: check that retExpr has no call-suspendibles.
			if err := g.writeExpr(
				b, retExpr, replaceCallSuspendibles, depth); err != nil {
				return err
			}
		}
		b.writes(";")

		if n.Keyword() == t.IDYield {
			return g.writeCoroSuspPoint(b, true)
		}

		if isError {
			b.writes("goto exit;")
		} else if isOK {
			g.currFunk.hasGotoOK = true
			b.writes("goto ok;")
		} else {
			g.currFunk.hasGotoOK = true
			// TODO: the "goto exit"s can be "goto ok".
			b.writes("if (wuffs_base__status__is_error(status)) { goto exit; }" +
				"else if (wuffs_base__status__is_suspension(status)) { " +
				"status = wuffs_base__error__cannot_return_a_suspension; goto exit; } goto ok;")
		}
		return nil
	}

	b.writes("return ")
	if g.currFunk.astFunc.Out() == nil {
		if retExpr != nil {
			return fmt.Errorf("return expression %q incompatible with empty return type", retExpr.Str(g.tm))
		}
	} else if retExpr == nil {
		// TODO: should a bare "return" imply "return out"?
		return fmt.Errorf("empty return expression incompatible with non-empty return type")
	} else if err := g.writeExpr(b, retExpr, replaceCallSuspendibles, depth); err != nil {
		return err
	}
	b.writeb(';')
	return nil
}

func (g *gen) writeStatementVar(b *buffer, n *a.Var, depth uint32) error {
	if v := n.Value(); v != nil {
		if err := g.writeSuspendibles(b, v, depth); err != nil {
			return err
		}
	}
	if nTyp := n.XType(); nTyp.IsArrayType() {
		if n.Value() != nil {
			// TODO: something like:
			// cv := nTyp.ArrayLength().ConstValue()
			// // TODO: check that cv is within size_t's range.
			// g.printf("{ size_t i; for (i = 0; i < %d; i++) { %s%s[i] = $DEFAULT_VALUE; }}\n",
			// cv, vPrefix, n.Name().Str(g.tm))
			return fmt.Errorf("TODO: array initializers for non-zero default values")
		}
		// TODO: arrays of arrays.
		name := n.Name().Str(g.tm)
		b.printf("memset(%s%s, 0, sizeof(%s%s));\n", vPrefix, name, vPrefix, name)

	} else {
		b.printf("%s%s = ", vPrefix, n.Name().Str(g.tm))
		if v := n.Value(); v != nil {
			if err := g.writeExpr(b, v, replaceCallSuspendibles, 0); err != nil {
				return err
			}
		} else if nTyp.IsSliceType() {
			// TODO: don't assume that the slice is a slice of base.u8.
			b.printf("((wuffs_base__slice_u8){})")
		} else if nTyp.IsTableType() {
			// TODO: don't assume that the table is a table of base.u8.
			b.printf("((wuffs_base__table_u8){})")
		} else if nTyp.IsIOType() {
			s := "reader"
			if nTyp.QID()[1] == t.IDIOWriter {
				s = "writer"
			}
			b.printf("((wuffs_base__io_%s){})", s)
		} else {
			b.writeb('0')
		}
		b.writes(";\n")
	}
	return nil
}

func (g *gen) writeStatementWhile(b *buffer, n *a.While, depth uint32) error {
	if n.HasContinue() {
		jt, err := g.currFunk.jumpTarget(n)
		if err != nil {
			return err
		}
		b.printf("label_%d_continue:;\n", jt)
	}
	condition := buffer(nil)
	if err := g.writeExpr(&condition, n.Condition(), replaceCallSuspendibles, 0); err != nil {
		return err
	}
	// Calling trimParens avoids clang's -Wparentheses-equality warning.
	b.printf("while (%s) {\n", trimParens(condition))
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

func (g *gen) writeIterateRound(b *buffer, name string, body []*a.Node, round uint32, depth uint32, length int, unroll int) error {
	b.printf("%s%s.len = %d;\n", vPrefix, name, length)
	b.printf("uint8_t* %send%d_%s = %sslice_%s.ptr + (%sslice_%s.len / %d) * %d;\n",
		iPrefix, round, name, iPrefix, name, iPrefix, name, length*unroll, length*unroll)
	b.printf("while (%s%s.ptr < %send%d_%s) {\n", vPrefix, name, iPrefix, round, name)
	for i := 0; i < unroll; i++ {
		for _, o := range body {
			if err := g.writeStatement(b, o, depth); err != nil {
				return err
			}
		}
		b.printf("%s%s.ptr += %d;\n", vPrefix, name, length)
	}
	b.writes("}\n")
	return nil
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
	b.printf("WUFFS_BASE__COROUTINE_SUSPENSION_POINT%s(%d);\n", macro, g.currFunk.coroSuspPoint)
	return nil
}

func (g *gen) writeSuspendibles(b *buffer, n *a.Expr, depth uint32) error {
	if !n.Effect().Coroutine() {
		return nil
	}
	if n.Operator() != t.IDTry {
		if err := g.writeCoroSuspPoint(b, false); err != nil {
			return err
		}
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
	if e := n.Effect(); !e.RootCause() || !e.Coroutine() {
		for _, o := range n.AsNode().AsRaw().SubNodes() {
			if o != nil && o.Kind() == a.KExpr {
				if err := g.writeCallSuspendibles(b, o.AsExpr(), depth); err != nil {
					return err
				}
			}
		}
		for _, o := range n.Args() {
			if o != nil && o.Kind() == a.KExpr {
				if err := g.writeCallSuspendibles(b, o.AsExpr(), depth); err != nil {
					return err
				}
			}
		}
		return nil
	}

	if err := g.writeSaveExprDerivedVars(b, n); err != nil {
		return err
	}

	if err := g.writeBuiltinCallSuspendibles(b, n, depth); err != errNoSuchBuiltin {
		return err
	}

	if n.Operator() == t.IDTry {
		if g.currFunk.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.currFunk.tempW
		g.currFunk.tempW++

		b.printf("wuffs_base__status %s%d = ", tPrefix, temp)
	} else {
		b.writes("status = ")
	}

	if err := g.writeExprUserDefinedCall(b, n, replaceNothing, depth); err != nil {
		return err
	}
	b.writes(";\n")

	if err := g.writeLoadExprDerivedVars(b, n); err != nil {
		return err
	}

	if n.Operator() != t.IDTry {
		b.writes("if (status) { goto suspend; }\n")
	}
	return nil
}

func (g *gen) writeReadUXX(b *buffer, n *a.Expr, preName string, size uint32, endianness string) error {
	if (size&7 != 0) || (size < 16) || (size > 64) {
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
		cPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))

	b.printf("if (WUFFS_BASE__LIKELY(io1_a_src - iop_a_src >= %d)) {", size/8)
	b.printf("%s%d = wuffs_base__load_u%d%s(iop_a_src);\n", tPrefix, temp1, size, endianness)
	b.printf("iop_a_src += %d;\n", size/8)
	b.printf("} else {")
	b.printf("%s = 0;\n", scratchName)
	if err := g.writeCoroSuspPoint(b, false); err != nil {
		return err
	}
	b.printf("while (true) {")

	b.printf("if (WUFFS_BASE__UNLIKELY(iop_%s == io1_%s)) {"+
		"status = wuffs_base__suspension__short_read; goto suspend; }",
		preName, preName)

	b.printf("uint64_t *scratch = &%s;", scratchName)
	b.printf("uint32_t %s%d = *scratch", tPrefix, temp0)
	switch endianness {
	case "be":
		b.writes("& 0xFF; *scratch >>= 8; *scratch <<= 8;")
		b.printf("*scratch |= ((uint64_t)(*%s%s++)) << (56 - %s%d);",
			iopPrefix, preName, tPrefix, temp0)
	case "le":
		b.writes(">> 56; *scratch <<= 8; *scratch >>= 8;")
		b.printf("*scratch |= ((uint64_t)(*%s%s++)) << %s%d;",
			iopPrefix, preName, tPrefix, temp0)
	}

	b.printf("if (%s%d == %d) {", tPrefix, temp0, size-8)
	switch endianness {
	case "be":
		b.printf("%s%d = *scratch >> (64 - %d);", tPrefix, temp1, size)
	case "le":
		b.printf("%s%d = *scratch;", tPrefix, temp1)
	}
	b.printf("break;")
	b.printf("}")

	b.printf("%s%d += 8;", tPrefix, temp0)
	switch endianness {
	case "be":
		b.printf("*scratch |= ((uint64_t)(%s%d));", tPrefix, temp0)
	case "le":
		b.printf("*scratch |= ((uint64_t)(%s%d)) << 56;", tPrefix, temp0)
	}

	b.writes("}}\n")
	return nil
}

func trimParens(b []byte) []byte {
	if len(b) > 1 && b[0] == '(' && b[len(b)-1] == ')' {
		return b[1 : len(b)-1]
	}
	return b
}
