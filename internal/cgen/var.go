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

var errNeedDerivedVar = errors.New("cgen: internal error: need derived var")

func (g *gen) needDerivedVar(name t.ID) bool {
	for _, o := range g.currFunk.astFunc.Body() {
		err := o.Walk(func(p *a.Node) error {
			// Look for p matching "args.name.etc(etc)".
			if p.Kind() != a.KExpr {
				return nil
			}
			q := p.AsExpr()
			if q.Operator() != t.IDOpenParen {
				return nil
			}
			q = q.LHS().AsExpr()
			if q.Operator() != t.IDDot {
				return nil
			}
			q = q.LHS().AsExpr()
			if q.Operator() != t.IDDot || q.Ident() != name {
				return nil
			}
			q = q.LHS().AsExpr()
			if q.Operator() != 0 || q.Ident() != t.IDArgs {
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
		o := o.AsField()
		if !o.XType().IsIOType() || !g.needDerivedVar(o.Name()) {
			continue
		}
		if g.currFunk.derivedVars == nil {
			g.currFunk.derivedVars = map[t.ID]struct{}{}
		}
		g.currFunk.derivedVars[o.Name()] = struct{}{}
	}
}

func (g *gen) writeLoadDerivedVar(b *buffer, hack string, prefix string, name t.ID, typ *a.TypeExpr, header bool) error {
	// TODO: remove this hack. We're picking up the wrong name for "src:r,
	// dummy:args.src".
	if name.Str(g.tm) == "dummy" {
		name = g.tm.ByName("src")
	}
	// TODO: also remove this hack.
	if hack == "w" {
		b.printf("%s%sw = %sw.data.ptr + %sw.meta.wi;\n", iopPrefix, vPrefix, uPrefix, uPrefix)
		return nil
	} else if hack == "r" {
		b.printf("%s%sr = %sr.data.ptr + %sr.meta.ri;\n", iopPrefix, vPrefix, uPrefix, uPrefix)
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

	preName := prefix + name.Str(g.tm)
	i1, i2 := "meta.ri", "meta.wi"
	if typ.QID()[1] == t.IDIOWriter {
		i1, i2 = "meta.wi", "data.len"
	}

	if header {
		b.printf("uint8_t* %s%s = NULL;", iopPrefix, preName)
		b.printf("uint8_t* %s%s WUFFS_BASE__POTENTIALLY_UNUSED = NULL;", io0Prefix, preName)
		b.printf("uint8_t* %s%s WUFFS_BASE__POTENTIALLY_UNUSED = NULL;", io1Prefix, preName)
		b.printf("uint8_t* %s%s WUFFS_BASE__POTENTIALLY_UNUSED = NULL;", io2Prefix, preName)
	}

	b.printf("if (%s) {", preName)

	if header {
		b.printf("%s%s = %s->data.ptr;", io0Prefix, preName, preName)
		b.printf("%s%s = %s%s + %s->%s;", io1Prefix, preName, io0Prefix, preName, preName, i1)
		b.printf("%s%s = %s%s;", iopPrefix, preName, io1Prefix, preName)
		b.printf("%s%s = %s%s + %s->%s;", io2Prefix, preName, io0Prefix, preName, preName, i2)

		if typ.QID()[1] == t.IDIOWriter {
			b.printf("if (%s->meta.closed) {", preName)
			b.printf("%s%s = %s%s;", io2Prefix, preName, iopPrefix, preName)
			b.printf("}\n")
		}
	} else {
		b.printf("%s%s = %s->data.ptr + %s->%s;", iopPrefix, preName, preName, preName, i1)
	}

	b.printf("}\n")
	return nil
}

func (g *gen) writeSaveDerivedVar(b *buffer, hack string, prefix string, name t.ID, typ *a.TypeExpr) error {
	// TODO: remove this hack. We're picking up the wrong name for "src:r,
	// dummy:args.src".
	if name.Str(g.tm) == "dummy" {
		name = g.tm.ByName("src")
	}
	// TODO: also remove this hack.
	if hack == "w" {
		b.printf("%sw.meta.wi = ((size_t)(%s%sw - %sw.data.ptr));\n", uPrefix, iopPrefix, vPrefix, uPrefix)
		return nil
	} else if hack == "r" {
		b.printf("%sr.meta.ri = ((size_t)(%s%sr - %sr.data.ptr));\n", uPrefix, iopPrefix, vPrefix, uPrefix)
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

	preName := prefix + name.Str(g.tm)
	index := "ri"
	if typ.QID()[1] == t.IDIOWriter {
		index = "wi"
	}

	b.printf("if (%s) { %s->meta.%s = ((size_t)(%s%s - %s->data.ptr)); }\n",
		preName, preName, index, iopPrefix, preName, preName)
	return nil
}

func (g *gen) writeLoadExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if n.Operator() != t.IDOpenParen {
		return nil
	}
	for _, o := range n.Args() {
		o := o.AsArg()
		prefix := aPrefix
		// TODO: don't hard-code these.
		hack := ""
		if s := o.Value().Str(g.tm); s != "args.dst" && s != "args.src" && s != "w" && s != "r" {
			continue
		} else if s == "w" || s == "r" {
			prefix = vPrefix
			hack = s
		}
		if err := g.writeLoadDerivedVar(b, hack, prefix, o.Name(), o.Value().MType(), false); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeSaveExprDerivedVars(b *buffer, n *a.Expr) error {
	if g.currFunk.derivedVars == nil {
		return nil
	}
	if n.Operator() != t.IDOpenParen {
		return nil
	}
	for _, o := range n.Args() {
		o := o.AsArg()
		prefix := aPrefix
		// TODO: don't hard-code these.
		hack := ""
		if s := o.Value().Str(g.tm); s != "args.dst" && s != "args.src" && s != "w" && s != "r" {
			continue
		} else if s == "w" || s == "r" {
			prefix = vPrefix
			hack = s
		}
		if err := g.writeSaveDerivedVar(b, hack, prefix, o.Name(), o.Value().MType()); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeResumeSuspend1(b *buffer, f *funk, n *a.Var, suspend bool) error {
	if typ := n.XType(); typ.HasPointers() {
		return nil
	} else if f.varResumables == nil || !f.varResumables[n.Name()] {
		return nil
	} else {
		local := fmt.Sprintf("%s%s", vPrefix, n.Name().Str(g.tm))
		lhs := local
		// TODO: don't hard-code [0], and allow recursive coroutines.
		rhs := fmt.Sprintf("self->private_data.%s%s[0].%s", sPrefix, g.currFunk.astFunc.FuncName().Str(g.tm), lhs)
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

	return fmt.Errorf("cannot resume or suspend a local variable %q of type %q",
		n.Name().Str(g.tm), n.XType().Str(g.tm))
}

func (g *gen) writeResumeSuspend(b *buffer, f *funk, suspend bool) error {
	for _, n := range f.varList {
		if err := g.writeResumeSuspend1(b, f, n, suspend); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeVars(b *buffer, f *funk, inStructDecl bool) error {
	for _, n := range f.varList {
		typ := n.XType()
		if inStructDecl {
			if typ.HasPointers() || f.varResumables == nil || !f.varResumables[n.Name()] {
				continue
			}
		}

		name := n.Name().Str(g.tm)

		if typ.IsIOType() {
			b.printf("wuffs_base__io_buffer %s%s = wuffs_base__empty_io_buffer();\n", uPrefix, name)
		}

		if err := g.writeCTypeName(b, typ, vPrefix, name); err != nil {
			return err
		}
		if inStructDecl {
			b.writes(";\n")
		} else if typ.IsNumType() {
			b.writes(" = 0;\n")
		} else if typ.IsBool() {
			b.writes(" = false;\n")
		} else if typ.IsStatus() {
			b.writes(" = NULL;\n")
		} else if typ.IsIOType() {
			b.printf(" = &%s%s;\n", uPrefix, name)
		} else {
			b.writes(" = {0};\n")
		}

		if typ.IsIOType() {
			b.printf("uint8_t* %s%s%s WUFFS_BASE__POTENTIALLY_UNUSED = NULL;\n", iopPrefix, vPrefix, name)
			b.printf("uint8_t* %s%s%s WUFFS_BASE__POTENTIALLY_UNUSED = NULL;\n", io0Prefix, vPrefix, name)
			b.printf("uint8_t* %s%s%s WUFFS_BASE__POTENTIALLY_UNUSED = NULL;\n", io1Prefix, vPrefix, name)
			b.printf("uint8_t* %s%s%s WUFFS_BASE__POTENTIALLY_UNUSED = NULL;\n", io2Prefix, vPrefix, name)
		}
	}
	return nil
}
