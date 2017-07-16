// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package main

import (
	"errors"
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

type perFunc struct {
	funk          *a.Func
	derivedVars   map[t.ID]struct{}
	jumpTargets   map[*a.While]uint32
	coroSuspPoint uint32
	tempW         uint32
	tempR         uint32
	public        bool
	suspendible   bool
	limitVarName  string
	shortReads    []string
}

func (g *gen) writeFuncSignature(n *a.Func) error {
	// TODO: write n's return values.
	if n.Suspendible() {
		g.printf("%sstatus ", g.pkgPrefix)
	} else {
		g.writes("void ")
	}

	g.writes(g.pkgPrefix)
	if r := n.Receiver(); r != 0 {
		g.writes(r.String(g.tm))
		g.writeb('_')
	}
	g.printf("%s(", n.Name().String(g.tm))

	comma := false
	if r := n.Receiver(); r != 0 {
		g.printf("%s%s *self", g.pkgPrefix, r.String(g.tm))
		comma = true
	}
	for _, o := range n.In().Fields() {
		if comma {
			g.writeb(',')
		}
		comma = true
		o := o.Field()
		if err := g.writeCTypeName(o.XType(), aPrefix, o.Name().String(g.tm)); err != nil {
			return err
		}
	}

	g.printf(")")
	return nil
}

func (g *gen) writeFuncPrototype(n *a.Func) error {
	if err := g.writeFuncSignature(n); err != nil {
		return err
	}
	g.writes(";\n\n")
	return nil
}

func (g *gen) writeFuncImpl(n *a.Func) error {
	g.perFunc = perFunc{
		funk:        n,
		public:      n.Public(),
		suspendible: n.Suspendible(),
	}
	if err := g.writeFuncSignature(n); err != nil {
		return err
	}
	g.writes("{\n")

	// Check the previous status and the "self" arg.
	if g.perFunc.public && n.Receiver() != 0 {
		g.writes("if (!self) {")
		if g.perFunc.suspendible {
			g.printf("return PUFFS_%s_ERROR_BAD_RECEIVER;", g.PKGNAME)
		} else {
			g.printf("return;")
		}
		g.writes("}")

		g.printf("if (self->private_impl.magic != PUFFS_MAGIC) {"+
			"self->private_impl.status = PUFFS_%s_ERROR_CONSTRUCTOR_NOT_CALLED; }", g.PKGNAME)

		g.writes("if (self->private_impl.status < 0) {")
		if g.perFunc.suspendible {
			g.writes("return self->private_impl.status;")
		} else {
			g.writes("return;")
		}
		g.writes("}\n")
	}

	if g.perFunc.suspendible {
		g.printf("%sstatus status = PUFFS_%s_STATUS_OK;\n", g.pkgPrefix, g.PKGNAME)
	}

	// For public functions, check (at runtime) the other args for bounds and
	// null-ness. For private functions, those checks are done at compile time.
	if g.perFunc.public {
		if err := g.writeFuncImplArgChecks(n); err != nil {
			return err
		}
	}
	g.writes("\n")

	// Generate the local variables.
	if err := g.writeVars(n.Body()); err != nil {
		return err
	}
	g.writes("\n")

	if g.perFunc.suspendible {
		g.findDerivedVars()
		for _, o := range n.In().Fields() {
			o := o.Field()
			if err := g.writeLoadDerivedVar(o.Name(), o.XType(), true); err != nil {
				return err
			}
		}
		g.writes("\n")

		// TODO: don't hard-code [0], and allow recursive coroutines.
		g.printf("uint32_t coro_susp_point = self->private_impl.%s%s[0].coro_susp_point;\n",
			cPrefix, n.Name().String(g.tm))
		g.printf("if (coro_susp_point) {\n")
		if err := g.writeResumeSuspend(n.Body(), false); err != nil {
			return err
		}
		g.writes("}\n")
		// Generate a coroutine switch similiar to the technique in
		// https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
		//
		// The matching } is written below. See "Close the coroutine switch".
		g.writes("switch (coro_susp_point) {\nPUFFS_COROUTINE_SUSPENSION_POINT(0);\n\n")
	}

	// Generate the function body.
	for _, o := range n.Body() {
		if err := g.writeStatement(o, 0); err != nil {
			return err
		}
	}

	if g.perFunc.suspendible {
		// We've reached the end of the function body. Reset the coroutine
		// suspension point so that the next call to this function starts at
		// the top.
		g.writes("coro_susp_point = 0;\n")
		g.writes("}\n\n") // Close the coroutine switch.

		g.writes("goto suspend;\n") // Avoid the "unused label" warning.
		g.writes("suspend:\n")
		g.printf("self->private_impl.%s%s[0].coro_susp_point = coro_susp_point;\n",
			cPrefix, n.Name().String(g.tm))
		if err := g.writeResumeSuspend(n.Body(), true); err != nil {
			return err
		}
		g.writes("\n")

		for _, o := range n.In().Fields() {
			o := o.Field()
			if err := g.writeSaveDerivedVar(o.Name(), o.XType()); err != nil {
				return err
			}
		}
		g.writes("\n")

		g.writes("goto exit;\n") // Avoid the "unused label" warning.
		g.writes("exit:")
		if g.perFunc.public {
			g.writes("self->private_impl.status = status;\n")
		}
		g.writes("return status;\n\n")

		shortReadsSeen := map[string]struct{}{}
		for _, sr := range g.perFunc.shortReads {
			if _, ok := shortReadsSeen[sr]; ok {
				continue
			}
			shortReadsSeen[sr] = struct{}{}
			if err := g.writeShortRead(sr); err != nil {
				return err
			}
		}
	}

	g.writes("}\n\n")
	if g.perFunc.tempW != g.perFunc.tempR {
		return fmt.Errorf("internal error: temporary variable count out of sync")
	}
	return nil
}

func (g *gen) writeFuncImplArgChecks(n *a.Func) error {
	checks := []string(nil)

	for _, o := range n.In().Fields() {
		o := o.Field()
		oTyp := o.XType()
		if oTyp.Decorator().Key() != t.KeyPtr && !oTyp.IsRefined() {
			// TODO: Also check elements, for array-typed arguments.
			continue
		}

		switch {
		case oTyp.Decorator().Key() == t.KeyPtr:
			checks = append(checks, fmt.Sprintf("!%s%s", aPrefix, o.Name().String(g.tm)))

		case oTyp.IsRefined():
			bounds := [2]*big.Int{}
			for i, b := range oTyp.Bounds() {
				if b != nil {
					if cv := b.ConstValue(); cv != nil {
						bounds[i] = cv
					}
				}
			}
			if key := oTyp.Name().Key(); key < t.Key(len(numTypeBounds)) {
				ntb := numTypeBounds[key]
				for i := 0; i < 2; i++ {
					if bounds[i] != nil && ntb[i] != nil && bounds[i].Cmp(ntb[i]) == 0 {
						bounds[i] = nil
						continue
					}
				}
			}
			for i, b := range bounds {
				if b != nil {
					op := '<'
					if i != 0 {
						op = '>'
					}
					checks = append(checks, fmt.Sprintf("%s%s %c %s", aPrefix, o.Name().String(g.tm), op, b))
				}
			}
		}
	}

	if len(checks) == 0 {
		return nil
	}

	g.writes("if (")
	for i, c := range checks {
		if i != 0 {
			g.writes(" || ")
		}
		g.writes(c)
	}
	g.writes(") {")
	if g.perFunc.suspendible {
		g.printf("status = PUFFS_%s_ERROR_BAD_ARGUMENT; goto exit;", g.PKGNAME)
	} else if n.Receiver() != 0 {
		g.printf("self->private_impl.status = PUFFS_%s_ERROR_BAD_ARGUMENT; return;", g.PKGNAME)
	} else {
		g.printf("return;")
	}
	g.writes("}\n")
	return nil
}

var errNeedDerivedVar = errors.New("internal: need derived var")

func (g *gen) needDerivedVar(name t.ID) bool {
	for _, o := range g.perFunc.funk.Body() {
		err := o.Walk(func(p *a.Node) error {
			// Look for p matching "in.name.etc(etc)".
			if p.Kind() != a.KExpr {
				return nil
			}
			q := p.Expr()
			if q.ID0().Key() != t.KeyOpenParen {
				return nil
			}
			q = q.LHS().Expr()
			if q.ID0().Key() != t.KeyDot {
				return nil
			}
			q = q.LHS().Expr()
			if q.ID0().Key() != t.KeyDot || q.ID1() != name {
				return nil
			}
			q = q.LHS().Expr()
			if q.ID0() != 0 || q.ID1().Key() != t.KeyIn {
				return nil
			}
			return errNeedDerivedVar
		})
		if err == errNeedDerivedVar {
			return true
		}
	}
	return false
}

func (g *gen) findDerivedVars() {
	for _, o := range g.perFunc.funk.In().Fields() {
		o := o.Field()
		oTyp := o.XType()
		if oTyp.Decorator() != 0 {
			continue
		}
		if k := oTyp.Name().Key(); k != t.KeyReader1 && k != t.KeyWriter1 {
			continue
		}
		if !g.needDerivedVar(o.Name()) {
			continue
		}
		if g.perFunc.derivedVars == nil {
			g.perFunc.derivedVars = map[t.ID]struct{}{}
		}
		g.perFunc.derivedVars[o.Name()] = struct{}{}
	}
}

func (g *gen) writeLoadDerivedVar(name t.ID, typ *a.TypeExpr, decl bool) error {
	if g.perFunc.derivedVars == nil {
		return nil
	}
	if _, ok := g.perFunc.derivedVars[name]; !ok {
		return nil
	}
	nameStr := name.String(g.tm)
	switch typ.Name().Key() {
	case t.KeyReader1:
		if decl {
			g.printf("uint8_t* %srptr_%s = NULL;", bPrefix, nameStr)
			g.printf("uint8_t* %srend_%s = NULL;", bPrefix, nameStr)
		}
		g.printf("if (%s%s.buf) {", aPrefix, nameStr)

		g.printf("%srptr_%s = %s%s.buf->ptr + %s%s.buf->ri;",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		g.printf("size_t len = %s%s.buf->wi - %s%s.buf->ri;",
			aPrefix, nameStr, aPrefix, nameStr)
		g.printf("puffs_base_limit1* lim;")
		g.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		g.printf("if (lim->ptr_to_len && (len > *lim->ptr_to_len)) { len = *lim->ptr_to_len; }")
		g.printf("}")
		g.printf("%srend_%s = %srptr_%s + len;", bPrefix, nameStr, bPrefix, nameStr)

		g.printf("}\n")

	case t.KeyWriter1:
		if decl {
			g.printf("uint8_t* %swptr_%s = NULL;", bPrefix, nameStr)
			g.printf("uint8_t* %swend_%s = NULL;", bPrefix, nameStr)
		}
		g.printf("if (%s%s.buf) {", aPrefix, nameStr)

		g.printf("%swptr_%s = %s%s.buf->ptr + %s%s.buf->wi;",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		g.printf("size_t len = %s%s.buf->len - %s%s.buf->wi;",
			aPrefix, nameStr, aPrefix, nameStr)
		g.printf("puffs_base_limit1* lim;")
		g.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		g.printf("if (lim->ptr_to_len && (len > *lim->ptr_to_len)) { len = *lim->ptr_to_len; }")
		g.printf("}")
		g.printf("%swend_%s = %swptr_%s + len;", bPrefix, nameStr, bPrefix, nameStr)

		g.printf("}\n")
	}
	return nil
}

func (g *gen) writeSaveDerivedVar(name t.ID, typ *a.TypeExpr) error {
	if g.perFunc.derivedVars == nil {
		return nil
	}
	if _, ok := g.perFunc.derivedVars[name]; !ok {
		return nil
	}
	nameStr := name.String(g.tm)
	switch typ.Name().Key() {
	case t.KeyReader1:
		g.printf("if (%s%s.buf) {", aPrefix, nameStr)

		g.printf("size_t n = %srptr_%s - (%s%s.buf->ptr + %s%s.buf->ri);",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		g.printf("%s%s.buf->ri += n;", aPrefix, nameStr)
		g.printf("puffs_base_limit1* lim;")
		g.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		g.printf("if (lim->ptr_to_len) { *lim->ptr_to_len -= n; }")
		g.printf("}")

		g.printf("}\n")

	case t.KeyWriter1:
		g.printf("if (%s%s.buf) {", aPrefix, nameStr)

		g.printf("size_t n = %swptr_%s - (%s%s.buf->ptr + %s%s.buf->wi);",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		g.printf("%s%s.buf->wi += n;", aPrefix, nameStr)
		g.printf("puffs_base_limit1* lim;")
		g.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		g.printf("if (lim->ptr_to_len) { *lim->ptr_to_len -= n; }")
		g.printf("}")

		g.printf("}\n")
	}
	return nil
}

func (g *gen) writeLoadExprDerivedVars(n *a.Expr) error {
	if g.perFunc.derivedVars == nil || n.ID0().Key() != t.KeyOpenParen {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		if s := o.Value().String(g.tm); s != "in.src" && s != "lzw_src" {
			continue
		}
		if err := g.writeLoadDerivedVar(o.Name(), o.Value().MType(), false); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeSaveExprDerivedVars(n *a.Expr) error {
	if g.perFunc.derivedVars == nil || n.ID0().Key() != t.KeyOpenParen {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		if s := o.Value().String(g.tm); s != "in.src" && s != "lzw_src" {
			continue
		}
		if err := g.writeSaveDerivedVar(o.Name(), o.Value().MType()); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) visitVars(block []*a.Node, depth uint32, f func(*gen, *a.Var) error) error {
	if depth > a.MaxBodyDepth {
		return fmt.Errorf("body recursion depth too large")
	}
	depth++

	for _, o := range block {
		switch o.Kind() {
		case a.KIf:
			for o := o.If(); o != nil; o = o.ElseIf() {
				if err := g.visitVars(o.BodyIfTrue(), depth, f); err != nil {
					return err
				}
				if err := g.visitVars(o.BodyIfFalse(), depth, f); err != nil {
					return err
				}
			}

		case a.KVar:
			if err := f(g, o.Var()); err != nil {
				return err
			}

		case a.KWhile:
			if err := g.visitVars(o.While().Body(), depth, f); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) writeResumeSuspend1(n *a.Var, prefix string, suspend bool) error {
	lhs := fmt.Sprintf("%s%s", prefix, n.Name().String(g.tm))
	// TODO: don't hard-code [0], and allow recursive coroutines.
	rhs := fmt.Sprintf("self->private_impl.%s%s[0].%s", cPrefix, g.perFunc.funk.Name().String(g.tm), lhs)
	if suspend {
		lhs, rhs = rhs, lhs
	}
	typ := n.XType()
	switch typ.Decorator().Key() {
	case 0:
		g.printf("%s = %s;\n", lhs, rhs)
		return nil
	case t.KeyOpenBracket:
		if inner := typ.Inner(); inner.Decorator() != 0 || inner.Name().Key() != t.KeyU8 {
			break
		}
		cv := typ.ArrayLength().ConstValue()
		// TODO: check that cv is within size_t's range.
		g.printf("memcpy(%s, %s, %v);\n", lhs, rhs, cv)
		return nil
	}
	return fmt.Errorf("cannot resume or suspend a local variable %q of type %q",
		n.Name().String(g.tm), n.XType().String(g.tm))
}

func (g *gen) writeResumeSuspend(block []*a.Node, suspend bool) error {
	return g.visitVars(block, 0, func(g *gen, n *a.Var) error {
		if v := n.Value(); v != nil && v.ID0().Key() == t.KeyLimit {
			if err := g.writeResumeSuspend1(n, lPrefix, suspend); err != nil {
				return err
			}
		}
		return g.writeResumeSuspend1(n, vPrefix, suspend)
	})
}

func (g *gen) writeVars(block []*a.Node) error {
	return g.visitVars(block, 0, func(g *gen, n *a.Var) error {
		if v := n.Value(); v != nil && v.ID0().Key() == t.KeyLimit {
			g.printf("uint64_t %s%v;\n", lPrefix, n.Name().String(g.tm))
		}
		if err := g.writeCTypeName(n.XType(), vPrefix, n.Name().String(g.tm)); err != nil {
			return err
		}
		g.writes(";\n")
		return nil
	})
}

func (g *gen) writeStatement(n *a.Node, depth uint32) error {
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
		if err := g.writeSuspendibles(n.LHS(), depth); err != nil {
			return err
		}
		if err := g.writeSuspendibles(n.RHS(), depth); err != nil {
			return err
		}
		if err := g.writeExpr(n.LHS(), replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		// TODO: does KeyAmpHatEq need special consideration?
		g.writes(cOpNames[0xFF&n.Operator().Key()])
		if err := g.writeExpr(n.RHS(), replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		g.writes(";\n")
		return nil

	case a.KExpr:
		n := n.Expr()
		if err := g.writeSuspendibles(n, depth); err != nil {
			return err
		}
		if n.CallSuspendible() {
			return nil
		}
		// TODO: delete this hack that only matches "foo.set_literal_width(etc)".
		if isSetLiteralWidth(g.tm, n) {
			g.printf("%slzw_decoder_set_literal_width(&self->private_impl.f_lzw, ", g.pkgPrefix)
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(a, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
				return err
			}
			g.writes(");\n")
			return nil
		}
		return fmt.Errorf("TODO: generate code for foo() when foo is not a ? call-suspendible")

	case a.KIf:
		// TODO: for writeSuspendibles, make sure that we get order of
		// sub-expression evaluation correct.
		n, nCloseCurly := n.If(), 1
		for first := true; ; first = false {
			if n.Condition().Suspendible() {
				if !first {
					g.writeb('{')
					const maxCloseCurly = 1000
					if nCloseCurly == maxCloseCurly {
						return fmt.Errorf("too many nested if's")
					}
					nCloseCurly++
				}
				if err := g.writeSuspendibles(n.Condition(), depth); err != nil {
					return err
				}
			}

			g.writes("if (")
			if err := g.writeExpr(n.Condition(), replaceCallSuspendibles, parenthesesOptional, 0); err != nil {
				return err
			}
			g.writes(") {\n")
			for _, o := range n.BodyIfTrue() {
				if err := g.writeStatement(o, depth); err != nil {
					return err
				}
			}
			if bif := n.BodyIfFalse(); len(bif) > 0 {
				g.writes("} else {")
				for _, o := range bif {
					if err := g.writeStatement(o, depth); err != nil {
						return err
					}
				}
				break
			}
			n = n.ElseIf()
			if n == nil {
				break
			}
			g.writes("} else ")
		}
		for ; nCloseCurly > 0; nCloseCurly-- {
			g.writes("}\n")
		}
		return nil

	case a.KJump:
		n := n.Jump()
		jt, err := g.jumpTarget(n.JumpTarget())
		if err != nil {
			return err
		}
		keyword := "continue"
		if n.Keyword().Key() == t.KeyBreak {
			keyword = "break"
		}
		g.printf("goto label_%d_%s;\n", jt, keyword)
		return nil

	case a.KReturn:
		n := n.Return()
		ret := status{}
		if n.Keyword() == 0 {
			ret.name = fmt.Sprintf("PUFFS_%s_STATUS_OK", g.PKGNAME)
		} else {
			ret = g.statusMap[n.Message()]
		}
		if g.perFunc.suspendible {
			g.printf("status = %s;", ret.name)
			if ret.isError {
				g.writes("goto exit;")
			} else {
				g.writes("goto suspend;")
			}
		} else {
			// TODO: consider the return values, especially if they involve
			// suspendible function calls.
			g.writes("return;\n")
		}
		return nil

	case a.KVar:
		n := n.Var()
		if v := n.Value(); v != nil {
			if err := g.writeSuspendibles(v, depth); err != nil {
				return err
			}
			if v.ID0().Key() == t.KeyLimit {
				g.perFunc.limitVarName = n.Name().String(g.tm)
				g.printf("%s%s =", lPrefix, g.perFunc.limitVarName)
				if err := g.writeExpr(
					v.LHS().Expr(), replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
					return err
				}
				g.writes(";\n")
			}
		}
		if n.XType().Decorator().Key() == t.KeyOpenBracket {
			if n.Value() != nil {
				return fmt.Errorf("TODO: array initializers for non-zero default values")
			}
			// TODO: arrays of arrays.
			cv := n.XType().ArrayLength().ConstValue()
			// TODO: check that cv is within size_t's range.
			g.printf("{ size_t i; for (i = 0; i < %d; i++) { %s%s[i] = 0; }}\n",
				cv, vPrefix, n.Name().String(g.tm))
		} else {
			g.printf("%s%s = ", vPrefix, n.Name().String(g.tm))
			if v := n.Value(); v != nil {
				if err := g.writeExpr(v, replaceCallSuspendibles, parenthesesMandatory, 0); err != nil {
					return err
				}
			} else {
				g.writeb('0')
			}
		}
		g.writes(";\n")
		return nil

	case a.KWhile:
		n := n.While()
		// TODO: consider suspendible calls.

		if n.HasContinue() {
			jt, err := g.jumpTarget(n)
			if err != nil {
				return err
			}
			g.printf("label_%d_continue:;\n", jt)
		}
		g.writes("while (")
		if err := g.writeExpr(n.Condition(), replaceCallSuspendibles, parenthesesOptional, 0); err != nil {
			return err
		}
		g.writes(") {\n")
		for _, o := range n.Body() {
			if err := g.writeStatement(o, depth); err != nil {
				return err
			}
		}
		g.writes("}\n")
		if n.HasBreak() {
			jt, err := g.jumpTarget(n)
			if err != nil {
				return err
			}
			g.printf("label_%d_break:;\n", jt)
		}
		return nil

	}
	return fmt.Errorf("unrecognized ast.Kind (%s) for writeStatement", n.Kind())
}

func (g *gen) writeCoroSuspPoint() error {
	const maxCoroSuspPoint = 0xFFFFFFFF
	g.perFunc.coroSuspPoint++
	if g.perFunc.coroSuspPoint == maxCoroSuspPoint {
		return fmt.Errorf("too many coroutine suspension points required")
	}

	g.printf("PUFFS_COROUTINE_SUSPENSION_POINT(%d);\n", g.perFunc.coroSuspPoint)
	return nil
}

func (g *gen) writeSuspendibles(n *a.Expr, depth uint32) error {
	if !n.Suspendible() {
		return nil
	}
	if err := g.writeCoroSuspPoint(); err != nil {
		return err
	}
	return g.writeCallSuspendibles(n, depth)
}

func (g *gen) writeCallSuspendibles(n *a.Expr, depth uint32) error {
	// The evaluation order for suspendible calls (which can have side effects)
	// is important here: LHS, MHS, RHS, Args and finally the node itself.
	if !n.CallSuspendible() {
		if depth > a.MaxExprDepth {
			return fmt.Errorf("expression recursion depth too large")
		}
		depth++

		for _, o := range n.Node().Raw().SubNodes() {
			if o != nil && o.Kind() == a.KExpr {
				if err := g.writeCallSuspendibles(o.Expr(), depth); err != nil {
					return err
				}
			}
		}
		for _, o := range n.Args() {
			if o != nil && o.Kind() == a.KExpr {
				if err := g.writeCallSuspendibles(o.Expr(), depth); err != nil {
					return err
				}
			}
		}
		return nil
	}

	if err := g.writeSaveExprDerivedVars(n); err != nil {
		return err
	}

	// TODO: delete these hacks that only matches "in.src.read_u8?()" etc.
	//
	// TODO: check reader1.buf and writer1.buf is non-NULL.
	if isInSrc(g.tm, n, t.KeyReadU8, 0) {
		if g.perFunc.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.perFunc.tempW
		g.perFunc.tempW++

		g.printf("if (PUFFS_UNLIKELY(%srptr_src == %srend_src)) { goto short_read_src; }", bPrefix, bPrefix)
		g.perFunc.shortReads = append(g.perFunc.shortReads, "src")
		// TODO: watch for passing an array type to writeCTypeName? In C, an
		// array type can decay into a pointer.
		if err := g.writeCTypeName(n.MType(), tPrefix, fmt.Sprint(temp)); err != nil {
			return err
		}
		g.printf(" = *%srptr_src++;\n", bPrefix)

	} else if isInSrc(g.tm, n, t.KeyReadU32LE, 0) {
		return g.writeReadUXX(n, "src", 32, "le")

	} else if isInSrc(g.tm, n, t.KeySkip32, 1) {
		if g.perFunc.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.perFunc.tempW
		g.perFunc.tempW++
		g.perFunc.tempR++

		// TODO: loop over all limits.
		g.printf("size_t %s%d = ", tPrefix, temp)
		x := n.Args()[0].Arg().Value()
		if err := g.writeExpr(x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		g.writes(";\n")

		g.printf("if (%s%d > %srend_src - %srptr_src) {\n", tPrefix, temp, bPrefix, bPrefix)
		// TODO: save tPrefix+temp as coroutine state, and suspend.
		g.printf("%s%d -= %srend_src - %srptr_src;\n", tPrefix, temp, bPrefix, bPrefix)
		g.printf("%ssrc.buf->ri = %ssrc.buf->wi;\n", aPrefix, aPrefix)

		g.printf("status = %ssrc.buf->closed ? PUFFS_%s_ERROR_UNEXPECTED_EOF : PUFFS_%s_SUSPENSION_SHORT_READ;",
			aPrefix, g.PKGNAME, g.PKGNAME)
		g.writes("if (status < 0) { goto exit; } goto suspend;")

		g.writes("}\n")
		g.printf("%srptr_src += %s%d;\n", bPrefix, tPrefix, temp)

	} else if isInDst(g.tm, n, t.KeyWrite, 1) {
		// TODO: don't assume that the argument is "this.stack[s:]".
		g.printf("if (%sdst.buf->closed) { status = PUFFS_%s_ERROR_CLOSED_FOR_WRITES;", aPrefix, g.PKGNAME)
		g.writes("goto exit;")
		g.writes("}\n")
		g.printf("if ((%swend_dst - %swptr_dst) < (sizeof(self->private_impl.f_stack) - v_s)) {",
			bPrefix, bPrefix)
		g.printf("status = PUFFS_%s_SUSPENSION_SHORT_WRITE;", g.PKGNAME)
		g.writes("goto suspend;")
		g.writes("}\n")
		g.printf("memmove(b_wptr_dst," +
			"self->private_impl.f_stack + v_s," +
			"sizeof(self->private_impl.f_stack) - v_s);\n")
		g.printf("b_wptr_dst += sizeof(self->private_impl.f_stack) - v_s;\n")

	} else if isInDst(g.tm, n, t.KeyWriteU8, 1) {
		g.printf("if (%swptr_dst == %swend_dst) { status = PUFFS_%s_SUSPENSION_SHORT_WRITE;",
			bPrefix, bPrefix, g.PKGNAME)
		g.writes("goto suspend;")
		g.writes("}\n")
		g.printf("*%swptr_dst++ = ", bPrefix)
		x := n.Args()[0].Arg().Value()
		if err := g.writeExpr(x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		g.writes(";\n")

	} else if isInDst(g.tm, n, t.KeyCopyFrom32, 2) {
		if g.perFunc.tempW > maxTemp {
			return fmt.Errorf("too many temporary variables required")
		}
		temp := g.perFunc.tempW
		g.perFunc.tempW++
		g.perFunc.tempR++

		g.writes("{\n")

		g.writes("self->private_impl.scratch = ")
		x := n.Args()[1].Arg().Value()
		if err := g.writeExpr(x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		g.writes(";\n")

		if err := g.writeCoroSuspPoint(); err != nil {
			return err
		}
		g.printf("size_t %s%d = self->private_impl.scratch;\n", tPrefix, temp)

		const wName = "dst"
		g.printf("if (%s%d > %swend_%s - %swptr_%s) {\n", tPrefix, temp, bPrefix, wName, bPrefix, wName)
		g.printf("%s%d = %swend_%s - %swptr_%s;\n", tPrefix, temp, bPrefix, wName, bPrefix, wName)
		g.printf("status = PUFFS_%s_SUSPENSION_SHORT_WRITE;\n", g.PKGNAME)
		g.writes("}\n")

		// TODO: don't assume that the first argument is "in.src".
		const rName = "src"
		g.printf("if (%s%d > %srend_%s - %srptr_%s) {\n", tPrefix, temp, bPrefix, rName, bPrefix, rName)
		g.printf("%s%d = %srend_%s - %srptr_%s;\n", tPrefix, temp, bPrefix, rName, bPrefix, rName)
		g.printf("status = PUFFS_%s_SUSPENSION_SHORT_READ;\n", g.PKGNAME)
		g.writes("}\n")

		g.printf("memmove(%swptr_%s, %srptr_%s, %s%d);\n", bPrefix, wName, bPrefix, rName, tPrefix, temp)
		g.printf("%swptr_%s += %s%d;\n", bPrefix, wName, tPrefix, temp)
		g.printf("%srptr_%s += %s%d;\n", bPrefix, rName, tPrefix, temp)
		g.printf("if (status) { self->private_impl.scratch -= %s%d; goto suspend; }\n", tPrefix, temp)

		g.writes("}\n")

	} else if isThisMethod(g.tm, n, "decode_header", 1) {
		g.printf("status = %s%s_decode_header(self, %ssrc);\n",
			g.pkgPrefix, g.perFunc.funk.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(n); err != nil {
			return err
		}
		g.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_lsd", 1) {
		g.printf("status = %s%s_decode_lsd(self, %ssrc);\n",
			g.pkgPrefix, g.perFunc.funk.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(n); err != nil {
			return err
		}
		g.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_extension", 1) {
		g.printf("status = %s%s_decode_extension(self, %ssrc);\n",
			g.pkgPrefix, g.perFunc.funk.Receiver().String(g.tm), aPrefix)
		if err := g.writeLoadExprDerivedVars(n); err != nil {
			return err
		}
		g.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_id", 2) {
		g.printf("status = %s%s_decode_id(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.perFunc.funk.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(n); err != nil {
			return err
		}
		g.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_uncompressed", 2) {
		g.printf("status = %s%s_decode_uncompressed(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.perFunc.funk.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(n); err != nil {
			return err
		}
		g.writes("if (status) { goto suspend; }\n")

	} else if isThisMethod(g.tm, n, "decode_dynamic", 2) {
		g.printf("status = %s%s_decode_dynamic(self, %sdst, %ssrc);\n",
			g.pkgPrefix, g.perFunc.funk.Receiver().String(g.tm), aPrefix, aPrefix)
		if err := g.writeLoadExprDerivedVars(n); err != nil {
			return err
		}
		g.writes("if (status) { goto suspend; }\n")

	} else if isDecode(g.tm, n) {
		g.printf("status = %slzw_decoder_decode(&self->private_impl.f_lzw, %sdst, %s%s);\n",
			g.pkgPrefix, aPrefix, vPrefix, n.Args()[1].Arg().Value().String(g.tm))
		if err := g.writeLoadExprDerivedVars(n); err != nil {
			return err
		}
		// TODO: be principled with "if (status < 0)" vs "if (status)".
		g.writes("if (status < 0) { return status; }\n")

	} else {
		// TODO: fix this.
		//
		// This might involve calling g.writeExpr with replaceNothing??
		return fmt.Errorf("cannot convert Puffs call %q to C", n.String(g.tm))
	}
	return nil
}

func (g *gen) writeShortRead(name string) error {
	g.printf("\nshort_read_%s:\n", name)
	// TODO: ri == wi isn't the right condition.
	g.printf("status = ((%s%s.buf->closed) && (%s%s.buf->ri == %s%s.buf->wi)) ?"+
		"PUFFS_%s_ERROR_UNEXPECTED_EOF : PUFFS_%s_SUSPENSION_SHORT_READ;",
		aPrefix, name, aPrefix, name, aPrefix, name, g.PKGNAME, g.PKGNAME)
	g.writes("if (status < 0) { goto exit; } goto suspend;")
	return nil
}

func (g *gen) writeReadUXX(n *a.Expr, name string, size uint32, endianness string) error {
	if g.perFunc.tempW > maxTemp-1 {
		return fmt.Errorf("too many temporary variables required")
	}
	// temp0 is read by code generated in this function. temp1 is read elsewhere.
	temp0 := g.perFunc.tempW + 0
	temp1 := g.perFunc.tempW + 1
	g.perFunc.tempW += 2
	g.perFunc.tempR += 1

	if err := g.writeCTypeName(n.MType(), tPrefix, fmt.Sprint(temp1)); err != nil {
		return err
	}
	g.writes(";")

	g.printf("if (PUFFS_LIKELY(%srend_src - %srptr_src >= 4)) {", bPrefix, bPrefix)
	g.printf("%s%d = PUFFS_U32LE(%srptr_src);\n", tPrefix, temp1, bPrefix)
	g.printf("%srptr_src += 4;\n", bPrefix)
	g.printf("} else {")
	g.printf("self->private_impl.scratch = 0;\n")
	if err := g.writeCoroSuspPoint(); err != nil {
		return err
	}
	g.printf("while (true) {")

	g.printf("if (PUFFS_UNLIKELY(%srptr_%s == %srend_%s)) { goto short_read_%s; }",
		bPrefix, name, bPrefix, name, name)
	g.perFunc.shortReads = append(g.perFunc.shortReads, name)

	// TODO: look at endianness.
	g.printf("uint32_t %s%d = self->private_impl.scratch >> 56;", tPrefix, temp0)
	g.printf("self->private_impl.scratch <<= 8;")
	g.printf("self->private_impl.scratch >>= 8;")
	g.printf("self->private_impl.scratch |= ((uint64_t)(*%srptr_%s++)) << %s%d;", bPrefix, name, tPrefix, temp0)

	g.printf("if (%s%d == %d) {", tPrefix, temp0, size-8)
	g.printf("%s%d = self->private_impl.scratch;", tPrefix, temp1)
	g.printf("break;")
	g.printf("}")

	g.printf("%s%d += 8;", tPrefix, temp0)
	g.printf("self->private_impl.scratch |= ((uint64_t)(%s%d)) << 56;", tPrefix, temp0)

	g.writes("}}\n")
	return nil
}

func (g *gen) writeExpr(n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("expression recursion depth too large")
	}
	depth++

	if rp == replaceCallSuspendibles && n.CallSuspendible() {
		if g.perFunc.tempR >= g.perFunc.tempW {
			return fmt.Errorf("internal error: temporary variable count out of sync")
		}
		// TODO: check that this works with nested call-suspendibles:
		// "foo?().bar().qux?()(p?(), q?())".
		//
		// Also be aware of evaluation order in the presence of side effects:
		// in "foo(a?(), b!(), c?())", b should be called between a and c.
		g.printf("%s%d", tPrefix, g.perFunc.tempR)
		g.perFunc.tempR++
		return nil
	}

	if cv := n.ConstValue(); cv != nil {
		if !n.MType().IsBool() {
			g.writes(cv.String())
		} else if cv.Cmp(zero) == 0 {
			g.writes("false")
		} else if cv.Cmp(one) == 0 {
			g.writes("true")
		} else {
			return fmt.Errorf("%v has type bool but constant value %v is neither 0 or 1", n.String(g.tm), cv)
		}
		return nil
	}

	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		if err := g.writeExprOther(n, rp, depth); err != nil {
			return err
		}
	case t.FlagsUnaryOp:
		if err := g.writeExprUnaryOp(n, rp, depth); err != nil {
			return err
		}
	case t.FlagsBinaryOp:
		if err := g.writeExprBinaryOp(n, rp, pp, depth); err != nil {
			return err
		}
	case t.FlagsAssociativeOp:
		if err := g.writeExprAssociativeOp(n, rp, depth); err != nil {
			return err
		}
	default:
		return fmt.Errorf("unrecognized token.Key (0x%X) for writeExpr", n.ID0().Key())
	}

	return nil
}

func (g *gen) writeExprOther(n *a.Expr, rp replacementPolicy, depth uint32) error {
	switch n.ID0().Key() {
	case 0:
		if id1 := n.ID1(); id1.Key() == t.KeyThis {
			g.writes("self->private_impl")
		} else {
			if n.GlobalIdent() {
				g.writes(g.pkgPrefix)
			} else {
				g.writes(vPrefix)
			}
			g.writes(id1.String(g.tm))
		}
		return nil

	case t.KeyOpenParen:
		// n is a function call.
		// TODO: delete this hack that only matches "foo.bar_bits(etc)".
		if isLowHighBits(g.tm, n, t.KeyLowBits) {
			// "x.low_bits(n:etc)" in C is "((x) & ((1 << (n)) - 1))".
			x := n.LHS().Expr().LHS().Expr()
			g.writes("((")
			if err := g.writeExpr(x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			g.writes(") & ((1 << (")
			if err := g.writeExpr(n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			g.writes(")) - 1))")
			return nil
		}
		if isLowHighBits(g.tm, n, t.KeyHighBits) {
			// "x.high_bits(n:etc)" in C is "((x) >> (8*sizeof(x) - (n)))".
			x := n.LHS().Expr().LHS().Expr()
			g.writes("((")
			if err := g.writeExpr(x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			g.writes(") >> (")
			if sz, err := g.sizeof(x.MType()); err != nil {
				return err
			} else {
				g.printf("%d", 8*sz)
			}
			g.writes(" - (")
			if err := g.writeExpr(n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			g.writes(")))")
			return nil
		}
		// TODO.

	case t.KeyOpenBracket:
		// n is an index.
		if err := g.writeExpr(n.LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
		}
		g.writeb('[')
		if err := g.writeExpr(n.RHS().Expr(), rp, parenthesesOptional, depth); err != nil {
			return err
		}
		g.writeb(']')
		return nil

	case t.KeyColon:
	// n is a slice.
	// TODO.

	case t.KeyDot:
		if n.LHS().Expr().ID1().Key() == t.KeyIn {
			g.writes(aPrefix)
			g.writes(n.ID1().String(g.tm))
			return nil
		}

		if err := g.writeExpr(n.LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
		}
		// TODO: choose between . vs -> operators.
		//
		// TODO: don't assume that the fPrefix is necessary.
		g.writes(".")
		g.writes(fPrefix)
		g.writes(n.ID1().String(g.tm))
		return nil

	case t.KeyLimit:
		// TODO: don't hard code so much detail.
		g.writes("(puffs_base_reader1) {")
		g.writes(".buf = a_src.buf,")
		g.writes(".limit = (puffs_base_limit1) {")
		g.printf(".ptr_to_len = &%s%s,", lPrefix, g.perFunc.limitVarName)
		g.writes(".next = &a_src.limit,")
		g.writes("}}")
		g.perFunc.limitVarName = ""
		return nil
	}
	return fmt.Errorf("unrecognized token.Key (0x%X) for writeExprOther", n.ID0().Key())
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
	// TODO: check that n.Args() is "(x:bar)".
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != nArgs {
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
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName(methodName) {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyThis
}

func isLowHighBits(tm *t.Map, n *a.Expr, methodName t.Key) bool {
	// TODO: check that n.Args() is "(n:bar)".
	if n.ID0().Key() != t.KeyOpenParen || n.CallImpure() || len(n.Args()) != 1 {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1().Key() == methodName
}

func isSetLiteralWidth(tm *t.Map, n *a.Expr) bool {
	// TODO: check that n.Args() is "(lw:bar)".
	if n.ID0().Key() != t.KeyOpenParen || n.CallImpure() || len(n.Args()) != 1 {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1() == tm.ByName("set_literal_width")
}

func isDecode(tm *t.Map, n *a.Expr) bool {
	// TODO: check that n.Args() is "(dst:bar, src:baz)".
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != 2 {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1() == tm.ByName("decode")
}
