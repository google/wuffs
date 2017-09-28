// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package cgen

import (
	"errors"
	"fmt"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

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

		// Not all writer1 methods use the wend variable.
		b.printf("PUFFS_IGNORE_POTENTIALLY_UNUSED_VARIABLE(%swend_%s);", bPrefix, nameStr)

		b.printf("}\n")
	}
	return nil
}

func (g *gen) writeLoadExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if k := n.ID0().Key(); k != t.KeyOpenParen && k != t.KeyTry {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		if s := o.Value().String(g.tm); s != "in.dst" && s != "in.src" && s != "lzw_src" {
			continue
		}
		if err := g.writeLoadDerivedVar(b, o.Name(), o.Value().MType(), false); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeSaveExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if k := n.ID0().Key(); k != t.KeyOpenParen && k != t.KeyTry {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		if s := o.Value().String(g.tm); s != "in.dst" && s != "in.src" && s != "lzw_src" {
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

	// TODO: explicitly handle prefix == lPrefix?
	if typ := n.XType(); prefix == vPrefix && typ.HasPointers() {
		if suspend {
			return nil
		}
		rhs := ""

		switch typ.Decorator().Key() {
		case 0:
			if key := typ.Name().Key(); key < t.Key(len(cTypeNames)) {
				rhs = cTypeNames[key]
			}
		case t.KeyColon:
			// TODO: don't assume that the slice is a slice of u8.
			rhs = "puffs_base_slice_u8"
		}
		if rhs != "" {
			b.printf("%s = ((%s){});\n", local, rhs)
			return nil
		}

	} else {
		lhs := local
		// TODO: don't hard-code [0], and allow recursive coroutines.
		rhs := fmt.Sprintf("self->private_impl.%s%s[0].%s", cPrefix, g.currFunk.astFunc.Name().String(g.tm), lhs)
		if suspend {
			lhs, rhs = rhs, lhs
		}

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

func (g *gen) writeVars(b *buffer, block []*a.Node, skipPointerTypes bool) error {
	return g.visitVars(b, block, 0, func(g *gen, b *buffer, n *a.Var) error {
		if v := n.Value(); v != nil && v.ID0().Key() == t.KeyLimit {
			b.printf("uint64_t %s%v;\n", lPrefix, n.Name().String(g.tm))
		}
		if skipPointerTypes && n.XType().HasPointers() {
			return nil
		}
		if err := g.writeCTypeName(b, n.XType(), vPrefix, n.Name().String(g.tm)); err != nil {
			return err
		}
		b.writes(";\n")
		return nil
	})
}
