// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package cgen

import (
	"errors"
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

type funk struct {
	bHeader      buffer
	bBodyResume  buffer
	bBody        buffer
	bBodySuspend buffer
	bFooter      buffer

	astFunc       *a.Func
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

func (k *funk) jumpTarget(n *a.While) (uint32, error) {
	if k.jumpTargets == nil {
		k.jumpTargets = map[*a.While]uint32{}
	}
	if jt, ok := k.jumpTargets[n]; ok {
		return jt, nil
	}
	jt := uint32(len(k.jumpTargets))
	if jt == 1000000 {
		return 0, fmt.Errorf("too many jump targets")
	}
	k.jumpTargets[n] = jt
	return jt, nil
}

func (g *gen) writeFuncSignature(b *buffer, n *a.Func) error {
	// TODO: write n's return values.
	if n.Suspendible() {
		b.printf("%sstatus ", g.pkgPrefix)
	} else {
		b.writes("void ")
	}

	b.writes(g.pkgPrefix)
	if r := n.Receiver(); r != 0 {
		b.writes(r.String(g.tm))
		b.writeb('_')
	}
	b.printf("%s(", n.Name().String(g.tm))

	comma := false
	if r := n.Receiver(); r != 0 {
		b.printf("%s%s *self", g.pkgPrefix, r.String(g.tm))
		comma = true
	}
	for _, o := range n.In().Fields() {
		if comma {
			b.writeb(',')
		}
		comma = true
		o := o.Field()
		if err := g.writeCTypeName(b, o.XType(), aPrefix, o.Name().String(g.tm)); err != nil {
			return err
		}
	}

	b.printf(")")
	return nil
}

func (g *gen) writeFuncPrototype(b *buffer, n *a.Func) error {
	if err := g.writeFuncSignature(b, n); err != nil {
		return err
	}
	b.writes(";\n\n")
	return nil
}

func (g *gen) writeFuncImpl(b *buffer, n *a.Func) error {
	k := g.funks[n.QID()]

	if err := g.writeFuncSignature(b, n); err != nil {
		return err
	}
	b.writes("{\n")
	b.writex(k.bHeader)
	if k.suspendible && k.coroSuspPoint > 0 {
		b.writex(k.bBodyResume)
	}
	b.writex(k.bBody)
	if k.suspendible && k.coroSuspPoint > 0 {
		b.writex(k.bBodySuspend)
	}
	b.writex(k.bFooter)
	b.writes("}\n\n")
	return nil
}

func (g *gen) gatherFuncImpl(_ *buffer, n *a.Func) error {
	g.currFunk = funk{
		astFunc:     n,
		public:      n.Public(),
		suspendible: n.Suspendible(),
	}

	if err := g.writeFuncImplHeader(&g.currFunk.bHeader); err != nil {
		return err
	}
	if err := g.writeFuncImplBodyResume(&g.currFunk.bBodyResume); err != nil {
		return err
	}
	if err := g.writeFuncImplBody(&g.currFunk.bBody); err != nil {
		return err
	}
	if err := g.writeFuncImplBodySuspend(&g.currFunk.bBodySuspend); err != nil {
		return err
	}
	if err := g.writeFuncImplFooter(&g.currFunk.bFooter); err != nil {
		return err
	}

	if g.currFunk.tempW != g.currFunk.tempR {
		return fmt.Errorf("internal error: temporary variable count out of sync")
	}
	g.funks[n.QID()] = g.currFunk
	return nil
}

func (g *gen) writeFuncImplHeader(b *buffer) error {
	// Check the previous status and the "self" arg.
	if g.currFunk.public && g.currFunk.astFunc.Receiver() != 0 {
		b.writes("if (!self) {")
		if g.currFunk.suspendible {
			b.printf("return %sERROR_BAD_RECEIVER;", g.PKGPREFIX)
		} else {
			b.printf("return;")
		}
		b.writes("}")

		b.printf("if (self->private_impl.magic != PUFFS_MAGIC) {"+
			"self->private_impl.status = %sERROR_CONSTRUCTOR_NOT_CALLED; }", g.PKGPREFIX)

		b.writes("if (self->private_impl.status < 0) {")
		if g.currFunk.suspendible {
			b.writes("return self->private_impl.status;")
		} else {
			b.writes("return;")
		}
		b.writes("}\n")
	}

	if g.currFunk.suspendible {
		b.printf("%sstatus status = %sSTATUS_OK;\n", g.pkgPrefix, g.PKGPREFIX)
	}

	// For public functions, check (at runtime) the other args for bounds and
	// null-ness. For private functions, those checks are done at compile time.
	if g.currFunk.public {
		if err := g.writeFuncImplArgChecks(b, g.currFunk.astFunc); err != nil {
			return err
		}
	}
	b.writes("\n")

	// Generate the local variables.
	if err := g.writeVars(b, g.currFunk.astFunc.Body()); err != nil {
		return err
	}
	b.writes("\n")

	if g.currFunk.suspendible {
		g.findDerivedVars()
		for _, o := range g.currFunk.astFunc.In().Fields() {
			o := o.Field()
			if err := g.writeLoadDerivedVar(b, o.Name(), o.XType(), true); err != nil {
				return err
			}
		}
		b.writes("\n")
	}
	return nil
}

func (g *gen) writeFuncImplBodyResume(b *buffer) error {
	if g.currFunk.suspendible {
		// TODO: don't hard-code [0], and allow recursive coroutines.
		b.printf("uint32_t coro_susp_point = self->private_impl.%s%s[0].coro_susp_point;\n",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))
		b.printf("if (coro_susp_point) {\n")
		if err := g.writeResumeSuspend(b, g.currFunk.astFunc.Body(), false); err != nil {
			return err
		}
		b.writes("}\n")
		// Generate a coroutine switch similiar to the technique in
		// https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
		//
		// The matching } is written below. See "Close the coroutine switch".
		b.writes("switch (coro_susp_point) {\nPUFFS_COROUTINE_SUSPENSION_POINT(0);\n\n")
	}
	return nil
}

func (g *gen) writeFuncImplBody(b *buffer) error {
	for _, o := range g.currFunk.astFunc.Body() {
		if err := g.writeStatement(b, o, 0); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeFuncImplBodySuspend(b *buffer) error {
	if g.currFunk.suspendible {
		// We've reached the end of the function body. Reset the coroutine
		// suspension point so that the next call to this function starts at
		// the top.
		b.printf("self->private_impl.%s%s[0].coro_susp_point = 0;\n",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))
		b.writes("goto exit; }\n\n") // Close the coroutine switch.

		b.writes("goto suspend;\n") // Avoid the "unused label" warning.
		b.writes("suspend:\n")

		b.printf("self->private_impl.%s%s[0].coro_susp_point = coro_susp_point;\n",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))
		if err := g.writeResumeSuspend(b, g.currFunk.astFunc.Body(), true); err != nil {
			return err
		}
		b.writes("\n")
	}
	return nil
}

func (g *gen) writeFuncImplFooter(b *buffer) error {
	if g.currFunk.suspendible {
		b.writes("exit:")

		for _, o := range g.currFunk.astFunc.In().Fields() {
			o := o.Field()
			if err := g.writeSaveDerivedVar(b, o.Name(), o.XType()); err != nil {
				return err
			}
		}
		b.writes("\n")

		if g.currFunk.public {
			b.writes("self->private_impl.status = status;\n")
		}
		b.writes("return status;\n\n")

		shortReadsSeen := map[string]struct{}{}
		for _, sr := range g.currFunk.shortReads {
			if _, ok := shortReadsSeen[sr]; ok {
				continue
			}
			shortReadsSeen[sr] = struct{}{}
			if err := template_short_read(b, template_args_short_read{
				PKGPREFIX: g.PKGPREFIX,
				name:      sr,
			}); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) writeFuncImplArgChecks(b *buffer, n *a.Func) error {
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
			for i, bound := range oTyp.Bounds() {
				if bound != nil {
					if cv := bound.ConstValue(); cv != nil {
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
			for i, bound := range bounds {
				if bound != nil {
					op := '<'
					if i != 0 {
						op = '>'
					}
					checks = append(checks, fmt.Sprintf("%s%s %c %s", aPrefix, o.Name().String(g.tm), op, bound))
				}
			}
		}
	}

	if len(checks) == 0 {
		return nil
	}

	b.writes("if (")
	for i, c := range checks {
		if i != 0 {
			b.writes(" || ")
		}
		b.writes(c)
	}
	b.writes(") {")
	if g.currFunk.suspendible {
		b.printf("status = %sERROR_BAD_ARGUMENT; goto exit;", g.PKGPREFIX)
	} else if n.Receiver() != 0 {
		b.printf("self->private_impl.status = %sERROR_BAD_ARGUMENT; return;", g.PKGPREFIX)
	} else {
		b.printf("return;")
	}
	b.writes("}\n")
	return nil
}

var errNeedDerivedVar = errors.New("internal: need derived var")

func (g *gen) needDerivedVar(name t.ID) bool {
	for _, o := range g.currFunk.astFunc.Body() {
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
	for _, o := range g.currFunk.astFunc.In().Fields() {
		o := o.Field()
		oTyp := o.XType()
		if oTyp.Decorator() != 0 {
			continue
		}
		if key := oTyp.Name().Key(); key != t.KeyReader1 && key != t.KeyWriter1 {
			continue
		}
		if !g.needDerivedVar(o.Name()) {
			continue
		}
		if g.currFunk.derivedVars == nil {
			g.currFunk.derivedVars = map[t.ID]struct{}{}
		}
		g.currFunk.derivedVars[o.Name()] = struct{}{}
	}
}

func (g *gen) writeLoadDerivedVar(b *buffer, name t.ID, typ *a.TypeExpr, decl bool) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if _, ok := g.currFunk.derivedVars[name]; !ok {
		return nil
	}
	nameStr := name.String(g.tm)
	switch typ.Name().Key() {
	case t.KeyReader1:
		if decl {
			b.printf("uint8_t* %srptr_%s = NULL;", bPrefix, nameStr)
			b.printf("uint8_t* %srend_%s = NULL;", bPrefix, nameStr)
		}
		b.printf("if (%s%s.buf) {", aPrefix, nameStr)

		b.printf("%srptr_%s = %s%s.buf->ptr + %s%s.buf->ri;",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		b.printf("size_t len = %s%s.buf->wi - %s%s.buf->ri;",
			aPrefix, nameStr, aPrefix, nameStr)
		b.printf("puffs_base_limit1* lim;")
		b.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		b.printf("if (lim->ptr_to_len && (len > *lim->ptr_to_len)) { len = *lim->ptr_to_len; }")
		b.printf("}")
		b.printf("%srend_%s = %srptr_%s + len;", bPrefix, nameStr, bPrefix, nameStr)

		b.printf("}\n")

	case t.KeyWriter1:
		if decl {
			b.printf("uint8_t* %swptr_%s = NULL;", bPrefix, nameStr)
			b.printf("uint8_t* %swend_%s = NULL;", bPrefix, nameStr)
		}
		b.printf("if (%s%s.buf) {", aPrefix, nameStr)

		b.printf("%swptr_%s = %s%s.buf->ptr + %s%s.buf->wi;",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		b.printf("size_t len = %s%s.buf->len - %s%s.buf->wi;",
			aPrefix, nameStr, aPrefix, nameStr)
		b.printf("puffs_base_limit1* lim;")
		b.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		b.printf("if (lim->ptr_to_len && (len > *lim->ptr_to_len)) { len = *lim->ptr_to_len; }")
		b.printf("}")
		b.printf("%swend_%s = %swptr_%s + len;", bPrefix, nameStr, bPrefix, nameStr)

		b.printf("}\n")
	}
	return nil
}

func (g *gen) writeSaveDerivedVar(b *buffer, name t.ID, typ *a.TypeExpr) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if _, ok := g.currFunk.derivedVars[name]; !ok {
		return nil
	}
	nameStr := name.String(g.tm)
	switch typ.Name().Key() {
	case t.KeyReader1:
		b.printf("if (%s%s.buf) {", aPrefix, nameStr)

		b.printf("size_t n = %srptr_%s - (%s%s.buf->ptr + %s%s.buf->ri);",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		b.printf("%s%s.buf->ri += n;", aPrefix, nameStr)
		b.printf("puffs_base_limit1* lim;")
		b.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		b.printf("if (lim->ptr_to_len) { *lim->ptr_to_len -= n; }")
		b.printf("}")

		b.printf("}\n")

	case t.KeyWriter1:
		b.printf("if (%s%s.buf) {", aPrefix, nameStr)

		b.printf("size_t n = %swptr_%s - (%s%s.buf->ptr + %s%s.buf->wi);",
			bPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr)
		b.printf("%s%s.buf->wi += n;", aPrefix, nameStr)
		b.printf("puffs_base_limit1* lim;")
		b.printf("for (lim = &%s%s.limit; lim; lim = lim->next) {", aPrefix, nameStr)
		b.printf("if (lim->ptr_to_len) { *lim->ptr_to_len -= n; }")
		b.printf("}")

		b.printf("}\n")
	}
	return nil
}

func (g *gen) writeLoadExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil || n.ID0().Key() != t.KeyOpenParen {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		if s := o.Value().String(g.tm); s != "in.src" && s != "lzw_src" {
			continue
		}
		if err := g.writeLoadDerivedVar(b, o.Name(), o.Value().MType(), false); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeSaveExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil || n.ID0().Key() != t.KeyOpenParen {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		if s := o.Value().String(g.tm); s != "in.src" && s != "lzw_src" {
			continue
		}
		if err := g.writeSaveDerivedVar(b, o.Name(), o.Value().MType()); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) visitVars(b *buffer, block []*a.Node, depth uint32, f func(*gen, *buffer, *a.Var) error) error {
	if depth > a.MaxBodyDepth {
		return fmt.Errorf("body recursion depth too large")
	}
	depth++

	for _, o := range block {
		switch o.Kind() {
		case a.KIf:
			for o := o.If(); o != nil; o = o.ElseIf() {
				if err := g.visitVars(b, o.BodyIfTrue(), depth, f); err != nil {
					return err
				}
				if err := g.visitVars(b, o.BodyIfFalse(), depth, f); err != nil {
					return err
				}
			}

		case a.KVar:
			if err := f(g, b, o.Var()); err != nil {
				return err
			}

		case a.KWhile:
			if err := g.visitVars(b, o.While().Body(), depth, f); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) writeResumeSuspend1(b *buffer, n *a.Var, prefix string, suspend bool) error {
	local := fmt.Sprintf("%s%s", prefix, n.Name().String(g.tm))
	lhs := local
	// TODO: don't hard-code [0], and allow recursive coroutines.
	rhs := fmt.Sprintf("self->private_impl.%s%s[0].%s", cPrefix, g.currFunk.astFunc.Name().String(g.tm), lhs)
	if suspend {
		lhs, rhs = rhs, lhs
	}
	typ := n.XType()
	switch typ.Decorator().Key() {
	case 0:
		b.printf("%s = %s;\n", lhs, rhs)
		return nil
	case t.KeyOpenBracket:
		inner := typ.Inner()
		if inner.Decorator() != 0 {
			break
		}
		switch inner.Name().Key() {
		case t.KeyU8, t.KeyU16, t.KeyU32, t.KeyU64:
			b.printf("memcpy(%s, %s, sizeof(%s));\n", lhs, rhs, local)
			return nil
		}
	}
	return fmt.Errorf("cannot resume or suspend a local variable %q of type %q",
		n.Name().String(g.tm), n.XType().String(g.tm))
}

func (g *gen) writeResumeSuspend(b *buffer, block []*a.Node, suspend bool) error {
	return g.visitVars(b, block, 0, func(g *gen, b *buffer, n *a.Var) error {
		if v := n.Value(); v != nil && v.ID0().Key() == t.KeyLimit {
			if err := g.writeResumeSuspend1(b, n, lPrefix, suspend); err != nil {
				return err
			}
		}
		return g.writeResumeSuspend1(b, n, vPrefix, suspend)
	})
}

func (g *gen) writeVars(b *buffer, block []*a.Node) error {
	return g.visitVars(b, block, 0, func(g *gen, b *buffer, n *a.Var) error {
		if v := n.Value(); v != nil && v.ID0().Key() == t.KeyLimit {
			b.printf("uint64_t %s%v;\n", lPrefix, n.Name().String(g.tm))
		}
		if err := g.writeCTypeName(b, n.XType(), vPrefix, n.Name().String(g.tm)); err != nil {
			return err
		}
		b.writes(";\n")
		return nil
	})
}

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
		if isSetLiteralWidth(g.tm, n) {
			b.printf("%slzw_decoder_set_literal_width(&self->private_impl.f_lzw, ", g.pkgPrefix)
			a := n.Args()[0].Arg().Value()
			if err := g.writeExpr(b, a, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
				return err
			}
			b.writes(");\n")
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
		if n.Keyword() == 0 {
			ret.name = fmt.Sprintf("%sSTATUS_OK", g.PKGPREFIX)
		} else {
			ret = g.statusMap[n.Message()]
		}
		if g.currFunk.suspendible {
			b.printf("status = %s;", ret.name)
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
		if n.XType().Decorator().Key() == t.KeyOpenBracket {
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
		} else {
			b.printf("%s%s = ", vPrefix, n.Name().String(g.tm))
			if v := n.Value(); v != nil {
				if err := g.writeExpr(b, v, replaceCallSuspendibles, parenthesesMandatory, 0); err != nil {
					return err
				}
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
		return g.writeReadUXX(b, n, "src", 16, "BE")

	} else if isInSrc(g.tm, n, t.KeyReadU16LE, 0) {
		return g.writeReadUXX(b, n, "src", 16, "LE")

	} else if isInSrc(g.tm, n, t.KeyReadU32BE, 0) {
		return g.writeReadUXX(b, n, "src", 32, "BE")

	} else if isInSrc(g.tm, n, t.KeyReadU32LE, 0) {
		return g.writeReadUXX(b, n, "src", 32, "LE")

	} else if isInSrc(g.tm, n, t.KeySkip32, 1) {
		// TODO: is the scratch variable safe to use if callers can trap
		// LIMITED_READ and then go down different code paths?
		b.writes("self->private_impl.scratch = ")
		x := n.Args()[0].Arg().Value()
		if err := g.writeExpr(b, x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")

		// TODO: the CSP prior to this is probably unnecessary.
		if err := g.writeCoroSuspPoint(b); err != nil {
			return err
		}

		b.printf("if (self->private_impl.scratch > %srend_src - %srptr_src) {\n", bPrefix, bPrefix)
		b.printf("self->private_impl.scratch -= %srend_src - %srptr_src;\n", bPrefix, bPrefix)
		b.printf("%srptr_src = %srend_src;\n", bPrefix, bPrefix)

		// TODO: is ptr_to_len the right check?
		b.printf("if (%ssrc.limit.ptr_to_len) {", aPrefix)
		b.printf("status = %sSUSPENSION_LIMITED_READ;", g.PKGPREFIX)
		b.printf("} else if (%ssrc.buf->closed) {", aPrefix)
		b.printf("status = %sERROR_UNEXPECTED_EOF; goto exit;", g.PKGPREFIX)
		b.printf("} else { status = %sSUSPENSION_SHORT_READ; } goto suspend;\n", g.PKGPREFIX)

		b.writes("}\n")
		b.printf("%srptr_src += self->private_impl.scratch;\n", bPrefix)

	} else if isInDst(g.tm, n, t.KeyWrite, 1) {
		// TODO: don't assume that the argument is "this.stack[s:]".
		b.printf("if (%sdst.buf->closed) { status = %sERROR_CLOSED_FOR_WRITES;", aPrefix, g.PKGPREFIX)
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

		b.writes("self->private_impl.scratch = ")
		x := n.Args()[1].Arg().Value()
		if err := g.writeExpr(b, x, replaceCallSuspendibles, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writes(";\n")

		if err := g.writeCoroSuspPoint(b); err != nil {
			return err
		}
		b.printf("size_t %s%d = self->private_impl.scratch;\n", tPrefix, temp)

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
		b.printf("if (status) { self->private_impl.scratch -= %s%d; goto suspend; }\n", tPrefix, temp)

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

		b.writes("self->private_impl.scratch = ((uint64_t)(")
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
		b.printf("size_t %s%d = (size_t)(self->private_impl.scratch >> 32);", tPrefix, temp0)
		// TODO: it's not a BAD_ARGUMENT if we can copy from the sliding window.
		b.printf("if (PUFFS_UNLIKELY((%s%d == 0) || (%s%d > (%swptr_%s - %s%s.buf->ptr)))) { "+
			"status = %sERROR_BAD_ARGUMENT; goto exit; }\n",
			tPrefix, temp0, tPrefix, temp0, bPrefix, wName, aPrefix, wName, g.PKGPREFIX)
		b.printf("uint8_t* %s%d = %swptr_%s - %s%d;\n", tPrefix, temp1, bPrefix, wName, tPrefix, temp0)
		b.printf("uint32_t %s%d = (uint32_t)(self->private_impl.scratch);", tPrefix, temp2)

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
		b.printf("self->private_impl.scratch -= (uint64_t)(%s%d);\n", tPrefix, temp2)
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

	} else if isDecode(g.tm, n) {
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
	if endianness != "BE" && endianness != "LE" {
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

	b.printf("if (PUFFS_LIKELY(%srend_src - %srptr_src >= %d)) {", bPrefix, bPrefix, size/8)
	b.printf("%s%d = PUFFS_U%d%s(%srptr_src);\n", tPrefix, temp1, size, endianness, bPrefix)
	b.printf("%srptr_src += %d;\n", bPrefix, size/8)
	b.printf("} else {")
	b.printf("self->private_impl.scratch = 0;\n")
	if err := g.writeCoroSuspPoint(b); err != nil {
		return err
	}
	b.printf("while (true) {")

	b.printf("if (PUFFS_UNLIKELY(%srptr_%s == %srend_%s)) { goto short_read_%s; }",
		bPrefix, name, bPrefix, name, name)
	g.currFunk.shortReads = append(g.currFunk.shortReads, name)

	b.printf("uint32_t %s%d = self->private_impl.scratch", tPrefix, temp0)
	switch endianness {
	case "BE":
		b.printf("& 0xFF;")
		b.printf("self->private_impl.scratch >>= 8;")
		b.printf("self->private_impl.scratch <<= 8;")
		b.printf("self->private_impl.scratch |= ((uint64_t)(*%srptr_%s++)) << (64 - %s%d);",
			bPrefix, name, tPrefix, temp0)
	case "LE":
		b.printf(">> 56;")
		b.printf("self->private_impl.scratch <<= 8;")
		b.printf("self->private_impl.scratch >>= 8;")
		b.printf("self->private_impl.scratch |= ((uint64_t)(*%srptr_%s++)) << %s%d;",
			bPrefix, name, tPrefix, temp0)
	}

	b.printf("if (%s%d == %d) {", tPrefix, temp0, size-8)
	switch endianness {
	case "BE":
		b.printf("%s%d = self->private_impl.scratch >> (64 - %d);", tPrefix, temp1, size)
	case "LE":
		b.printf("%s%d = self->private_impl.scratch;", tPrefix, temp1)
	}
	b.printf("break;")
	b.printf("}")

	b.printf("%s%d += 8;", tPrefix, temp0)
	switch endianness {
	case "BE":
		b.printf("self->private_impl.scratch |= ((uint64_t)(%s%d));", tPrefix, temp0)
	case "LE":
		b.printf("self->private_impl.scratch |= ((uint64_t)(%s%d)) << 56;", tPrefix, temp0)
	}

	b.writes("}}\n")
	return nil
}

func (g *gen) writeExpr(b *buffer, n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("expression recursion depth too large")
	}
	depth++

	if rp == replaceCallSuspendibles && n.CallSuspendible() {
		if g.currFunk.tempR >= g.currFunk.tempW {
			return fmt.Errorf("internal error: temporary variable count out of sync")
		}
		// TODO: check that this works with nested call-suspendibles:
		// "foo?().bar().qux?()(p?(), q?())".
		//
		// Also be aware of evaluation order in the presence of side effects:
		// in "foo(a?(), b!(), c?())", b should be called between a and c.
		b.printf("%s%d", tPrefix, g.currFunk.tempR)
		g.currFunk.tempR++
		return nil
	}

	if cv := n.ConstValue(); cv != nil {
		if !n.MType().IsBool() {
			b.writes(cv.String())
		} else if cv.Cmp(zero) == 0 {
			b.writes("false")
		} else if cv.Cmp(one) == 0 {
			b.writes("true")
		} else {
			return fmt.Errorf("%v has type bool but constant value %v is neither 0 or 1", n.String(g.tm), cv)
		}
		return nil
	}

	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		if err := g.writeExprOther(b, n, rp, depth); err != nil {
			return err
		}
	case t.FlagsUnaryOp:
		if err := g.writeExprUnaryOp(b, n, rp, depth); err != nil {
			return err
		}
	case t.FlagsBinaryOp:
		if err := g.writeExprBinaryOp(b, n, rp, pp, depth); err != nil {
			return err
		}
	case t.FlagsAssociativeOp:
		if err := g.writeExprAssociativeOp(b, n, rp, pp, depth); err != nil {
			return err
		}
	default:
		return fmt.Errorf("unrecognized token.Key (0x%X) for writeExpr", n.ID0().Key())
	}

	return nil
}

func (g *gen) writeExprOther(b *buffer, n *a.Expr, rp replacementPolicy, depth uint32) error {
	switch n.ID0().Key() {
	case 0:
		if id1 := n.ID1(); id1.Key() == t.KeyThis {
			b.writes("self")
		} else {
			if n.GlobalIdent() {
				b.writes(g.pkgPrefix)
			} else {
				b.writes(vPrefix)
			}
			b.writes(id1.String(g.tm))
		}
		return nil

	case t.KeyOpenParen:
		// n is a function call.
		// TODO: delete this hack that only matches "foo.bar_bits(etc)".
		if isLowHighBits(g.tm, n, t.KeyLowBits) {
			// "x.low_bits(n:etc)" in C is "((x) & ((1 << (n)) - 1))".
			x := n.LHS().Expr().LHS().Expr()
			b.writes("((")
			if err := g.writeExpr(b, x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(") & ((1 << (")
			if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(")) - 1))")
			return nil
		}
		if isLowHighBits(g.tm, n, t.KeyHighBits) {
			// "x.high_bits(n:etc)" in C is "((x) >> (8*sizeof(x) - (n)))".
			x := n.LHS().Expr().LHS().Expr()
			b.writes("((")
			if err := g.writeExpr(b, x, rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(") >> (")
			if sz, err := g.sizeof(x.MType()); err != nil {
				return err
			} else {
				b.printf("%d", 8*sz)
			}
			b.writes(" - (")
			if err := g.writeExpr(b, n.Args()[0].Arg().Value(), rp, parenthesesOptional, depth); err != nil {
				return err
			}
			b.writes(")))")
			return nil
		}
		// TODO.

	case t.KeyOpenBracket:
		// n is an index.
		if err := g.writeExpr(b, n.LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
		}
		b.writeb('[')
		if err := g.writeExpr(b, n.RHS().Expr(), rp, parenthesesOptional, depth); err != nil {
			return err
		}
		b.writeb(']')
		return nil

	case t.KeyColon:
	// n is a slice.
	// TODO.

	case t.KeyDot:
		lhs := n.LHS().Expr()
		if lhs.ID1().Key() == t.KeyIn {
			b.writes(aPrefix)
			b.writes(n.ID1().String(g.tm))
			return nil
		}

		if err := g.writeExpr(b, lhs, rp, parenthesesMandatory, depth); err != nil {
			return err
		}
		if key := lhs.MType().Decorator().Key(); key == t.KeyPtr || key == t.KeyNptr {
			b.writes("->")
		} else {
			b.writes(".")
		}
		b.writes("private_impl." + fPrefix)
		b.writes(n.ID1().String(g.tm))
		return nil

	case t.KeyLimit:
		// TODO: don't hard code so much detail.
		b.writes("(puffs_base_reader1) {")
		b.writes(".buf = a_src.buf,")
		b.writes(".limit = (puffs_base_limit1) {")
		b.printf(".ptr_to_len = &%s%s,", lPrefix, g.currFunk.limitVarName)
		b.writes(".next = &a_src.limit,")
		b.writes("}}")
		g.currFunk.limitVarName = ""
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
