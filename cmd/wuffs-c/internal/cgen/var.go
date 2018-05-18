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

var errNeedDerivedVar = errors.New("internal: need derived var")

func (g *gen) needDerivedVar(name t.ID) bool {
	for _, o := range g.currFunk.astFunc.Body() {
		err := o.Walk(func(p *a.Node) error {
			// Look for p matching "in.name.etc(etc)".
			if p.Kind() != a.KExpr {
				return nil
			}
			q := p.Expr()
			if q.Operator() != t.IDOpenParen {
				return nil
			}
			q = q.LHS().Expr()
			if q.Operator() != t.IDDot {
				return nil
			}
			q = q.LHS().Expr()
			if q.Operator() != t.IDDot || q.Ident() != name {
				return nil
			}
			q = q.LHS().Expr()
			if q.Operator() != 0 || q.Ident() != t.IDIn {
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
		switch oTyp.QID() {
		default:
			continue
		case t.QID{t.IDBase, t.IDIOReader}, t.QID{t.IDBase, t.IDIOWriter}:
			// No-op.
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

func (g *gen) writeLoadDerivedVar(b *buffer, hack string, name t.ID, typ *a.TypeExpr, header bool) error {
	// TODO: remove this hack. We're picking up the wrong name for "src:r,
	// dummy:in.src".
	if name.Str(g.tm) == "dummy" {
		name = g.tm.ByName("src")
	}
	// TODO: also remove this hack.
	if hack == "w" {
		b.printf("ioptr_w = %sw.ptr + %sw.wi;\n", uPrefix, uPrefix)
		return nil
	}

	if !typ.IsIOType() {
		return nil
	}
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if _, ok := g.currFunk.derivedVars[name]; !ok {
		return nil
	}
	nameStr := name.Str(g.tm)
	i0, i1 := "ri", "wi"
	if typ.QID()[1] == t.IDIOWriter {
		i0, i1 = "wi", "len"
	}

	if header {
		b.printf("uint8_t* ioptr_%s = NULL;", nameStr)
		b.printf("uint8_t* iobounds0orig_%s = NULL;", nameStr)
		b.printf("uint8_t* iobounds1_%s = NULL;", nameStr)
		b.printf("WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(iobounds0orig_%s);", nameStr)
		b.printf("WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(iobounds1_%s);", nameStr)
	}

	b.printf("if (%s%s.private_impl.buf) {", aPrefix, nameStr)

	b.printf("ioptr_%s = %s%s.private_impl.buf->ptr + %s%s.private_impl.buf->%s;",
		nameStr, aPrefix, nameStr, aPrefix, nameStr, i0)

	if header {
		b.printf("if (!%s%s.private_impl.bounds[0]) {", aPrefix, nameStr)
		b.printf("%s%s.private_impl.bounds[0] = ioptr_%s;", aPrefix, nameStr, nameStr)
		b.printf("%s%s.private_impl.bounds[1] = %s%s.private_impl.buf->ptr + %s%s.private_impl.buf->%s;",
			aPrefix, nameStr, aPrefix, nameStr, aPrefix, nameStr, i1)
		b.printf("}\n")

		if typ.QID()[1] == t.IDIOWriter {
			b.printf("if (%s%s.private_impl.buf->closed) {", aPrefix, nameStr)
			b.printf("%s%s.private_impl.bounds[1] = ioptr_%s;", aPrefix, nameStr, nameStr)
			b.printf("}\n")
		}

		b.printf("iobounds0orig_%s = %s%s.private_impl.bounds[0];", nameStr, aPrefix, nameStr)
		b.printf("iobounds1_%s = %s%s.private_impl.bounds[1];", nameStr, aPrefix, nameStr)
	}

	b.printf("}\n")
	return nil
}

func (g *gen) writeSaveDerivedVar(b *buffer, hack string, name t.ID, typ *a.TypeExpr) error {
	// TODO: remove this hack. We're picking up the wrong name for "src:r,
	// dummy:in.src".
	if name.Str(g.tm) == "dummy" {
		name = g.tm.ByName("src")
	}
	// TODO: also remove this hack.
	if hack == "w" {
		b.printf("%sw.wi = ioptr_w - %sw.ptr;\n", uPrefix, uPrefix)
		return nil
	}

	if !typ.IsIOType() {
		return nil
	}
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if _, ok := g.currFunk.derivedVars[name]; !ok {
		return nil
	}
	nameStr := name.Str(g.tm)
	i0 := "ri"
	if typ.QID()[1] == t.IDIOWriter {
		i0 = "wi"
	}

	b.printf("if (%s%s.private_impl.buf) {", aPrefix, nameStr)
	b.printf("%s%s.private_impl.buf->%s = ioptr_%s - %s%s.private_impl.buf->ptr;",
		aPrefix, nameStr, i0, nameStr, aPrefix, nameStr)
	b.printf("}\n")
	return nil
}

func (g *gen) writeLoadExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if k := n.Operator(); k != t.IDOpenParen && k != t.IDTry {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		hack := ""
		if s := o.Value().Str(g.tm); s != "in.dst" && s != "in.src" && s != "w" {
			continue
		} else if s == "w" {
			hack = "w"
		}
		if err := g.writeLoadDerivedVar(b, hack, o.Name(), o.Value().MType(), false); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeSaveExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if k := n.Operator(); k != t.IDOpenParen && k != t.IDTry {
		return nil
	}
	for _, o := range n.Args() {
		o := o.Arg()
		// TODO: don't hard-code these.
		hack := ""
		if s := o.Value().Str(g.tm); s != "in.dst" && s != "in.src" && s != "w" {
			continue
		} else if s == "w" {
			hack = "w"
		}
		if err := g.writeSaveDerivedVar(b, hack, o.Name(), o.Value().MType()); err != nil {
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

		case a.KIOBind:
			if err := g.visitVars(b, o.IOBind().Body(), depth, f); err != nil {
				return err
			}

		case a.KIterate:
			if err := g.visitVars(b, o.Iterate().Variables(), depth, f); err != nil {
				return err
			}
			if err := g.visitVars(b, o.Iterate().Body(), depth, f); err != nil {
				return err
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

func (g *gen) writeResumeSuspend1(b *buffer, n *a.Var, suspend bool, initBoolTypedVars bool) error {
	local := fmt.Sprintf("%s%s", vPrefix, n.Name().Str(g.tm))

	if typ := n.XType(); typ.HasPointers() {
		if suspend {
			return nil
		}
		rhs := ""

		// TODO: move this initialization to writeVars?
		switch typ.Decorator() {
		case 0:
			if qid := typ.QID(); qid[0] == t.IDBase {
				if key := qid[1]; key < t.ID(len(cTypeNames)) {
					rhs = cTypeNames[key]
				}
			}
		case t.IDSlice:
			// TODO: don't assume that the slice is a slice of base.u8.
			rhs = "wuffs_base__slice_u8"
		case t.IDTable:
			// TODO: don't assume that the table is a table of base.u8.
			rhs = "wuffs_base__table_u8"
		}
		if rhs != "" {
			b.printf("%s = ((%s){});\n", local, rhs)
			return nil
		}

	} else {
		lhs := local
		rhs := ""
		// TODO: don't hard-code [0], and allow recursive coroutines.
		if !initBoolTypedVars {
			rhs = fmt.Sprintf("self->private_impl.%s%s[0].%s", cPrefix, g.currFunk.astFunc.FuncName().Str(g.tm), lhs)
		} else if typ.QID() != (t.QID{t.IDBase, t.IDBool}) {
			return nil
		} else if typ.Decorator() != 0 {
			goto fail
		} else {
			// Explicitly initialize the bool-typed local variable to false.
			//
			// Otherwise, the variable (in C) can hold an uninitialized value.
			// In terms of registers and memory (which work in integers, not
			// bools), that uninitialized value could be something like 255,
			// not 0 or 1.
			//
			// In the Wuffs language, a variable "foo" cannot be used before
			// initialization, but that's not easily seen by running C language
			// tools on Wuffs' generated C code. In C, that uninitialized v_foo
			// local variable could be suspended to and then resumed from the
			// f_foo coroutine state.
			//
			// This generated code explicitly initializes bool typed variables,
			// in order to avoid ubsan (undefined behavior sanitizer) warnings.
			//
			// In general, we don't initialize (set to zero or assign from
			// "this" fields) all of the v_etc local variables when not
			// resuming a coroutine, due to the performance impact. Commit
			// 2b6b0ac "Unconditionally resume local vars" has some numbers.
			rhs = "false"
		}
		if suspend {
			lhs, rhs = rhs, lhs
		}

		switch typ.Decorator() {
		case 0:
			b.printf("%s = %s;\n", lhs, rhs)
			return nil
		case t.IDArray:
			inner := typ.Inner()
			if inner.Decorator() != 0 {
				break
			}
			qid := inner.QID()
			if qid[0] != t.IDBase {
				break
			}
			switch qid[1] {
			case t.IDU8, t.IDU16, t.IDU32, t.IDU64:
				b.printf("memcpy(%s, %s, sizeof(%s));\n", lhs, rhs, local)
				return nil
			}
		}
	}

fail:
	return fmt.Errorf("cannot resume or suspend a local variable %q of type %q",
		n.Name().Str(g.tm), n.XType().Str(g.tm))
}

func (g *gen) writeResumeSuspend(b *buffer, block []*a.Node, suspend bool, initBoolTypedVars bool) error {
	return g.visitVars(b, block, 0, func(g *gen, b *buffer, n *a.Var) error {
		return g.writeResumeSuspend1(b, n, suspend, initBoolTypedVars)
	})
}

func (g *gen) writeVars(b *buffer, block []*a.Node, skipPointerTypes bool, skipIterateVariables bool) error {
	return g.visitVars(b, block, 0, func(g *gen, b *buffer, n *a.Var) error {
		typ := n.XType()
		if skipPointerTypes && typ.HasPointers() {
			return nil
		}
		if skipIterateVariables && n.IterateVariable() {
			return nil
		}
		name := n.Name().Str(g.tm)
		if err := g.writeCTypeName(b, typ, vPrefix, name); err != nil {
			return err
		}
		b.writes(";\n")
		if typ.IsIOType() {
			b.printf("wuffs_base__io_buffer %s%s;\n", uPrefix, name)
			// TODO: iobounds0_%s or iobounds0orig_%s variables?
			b.printf("uint8_t* ioptr_%s = NULL;\n", name)
			b.printf("uint8_t* iobounds1_%s = NULL;\n", name)
			b.printf("WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(%s%s);\n", uPrefix, name)
			b.printf("WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(ioptr_%s);\n", name)
			b.printf("WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(iobounds1_%s);\n", name)
		}
		return nil
	})
}
