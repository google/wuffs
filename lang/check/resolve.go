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

//go:generate go run gen.go

package check

import (
	"fmt"

	"github.com/google/wuffs/lang/builtin"
	"github.com/google/wuffs/lang/parse"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var (
	exprIn   = a.NewExpr(a.FlagsTypeChecked, 0, t.IDIn, nil, nil, nil, nil)
	exprOut  = a.NewExpr(a.FlagsTypeChecked, 0, t.IDOut, nil, nil, nil, nil)
	exprThis = a.NewExpr(a.FlagsTypeChecked, 0, t.IDThis, nil, nil, nil, nil)
)

// typeExprFoo is an *ast.Node MType (implicit type).
var (
	typeExprGeneric = a.NewTypeExpr(0, t.IDDiamond, nil, nil, nil)
	typeExprIdeal   = a.NewTypeExpr(0, t.IDDoubleZ, nil, nil, nil)
	typeExprList    = a.NewTypeExpr(0, t.IDDollar, nil, nil, nil)

	typeExprU8      = a.NewTypeExpr(0, t.IDU8, nil, nil, nil)
	typeExprU16     = a.NewTypeExpr(0, t.IDU16, nil, nil, nil)
	typeExprU32     = a.NewTypeExpr(0, t.IDU32, nil, nil, nil)
	typeExprU64     = a.NewTypeExpr(0, t.IDU64, nil, nil, nil)
	typeExprBool    = a.NewTypeExpr(0, t.IDBool, nil, nil, nil)
	typeExprStatus  = a.NewTypeExpr(0, t.IDStatus, nil, nil, nil)
	typeExprReader1 = a.NewTypeExpr(0, t.IDReader1, nil, nil, nil)
	typeExprWriter1 = a.NewTypeExpr(0, t.IDWriter1, nil, nil, nil)

	typeExprSliceU8 = a.NewTypeExpr(t.IDColon, 0, nil, nil, typeExprU8)

	// TODO: delete this.
	typeExprPlaceholder   = a.NewTypeExpr(0, t.IDU8, nil, nil, nil)
	typeExprPlaceholder16 = a.NewTypeExpr(0, t.IDU16, nil, nil, nil)
	typeExprPlaceholder32 = a.NewTypeExpr(0, t.IDU32, nil, nil, nil)
)

// TypeMap maps from variable names (as token IDs) to types.
type TypeMap map[t.ID]*a.TypeExpr

var builtInTypeMap = TypeMap{
	t.IDU8:      typeExprU8,
	t.IDU16:     typeExprU16,
	t.IDU32:     typeExprU32,
	t.IDU64:     typeExprU64,
	t.IDBool:    typeExprBool,
	t.IDStatus:  typeExprStatus,
	t.IDReader1: typeExprReader1,
	t.IDWriter1: typeExprWriter1,
}

func (c *Checker) builtInFunc(qid t.QID) (*a.Func, error) {
	if c.builtInFuncs == nil {
		err := error(nil)
		c.builtInFuncs, err = parseBuiltInFuncs(c.tm, builtin.Funcs, false)
		if err != nil {
			return nil, err
		}
	}
	return c.builtInFuncs[qid], nil
}

func (c *Checker) builtInSliceFunc(qid t.QID) (*a.Func, error) {
	if c.builtInSliceFuncs == nil {
		err := error(nil)
		c.builtInSliceFuncs, err = parseBuiltInFuncs(c.tm, builtin.SliceFuncs, true)
		if err != nil {
			return nil, err
		}
	}
	return c.builtInSliceFuncs[qid], nil
}

func parseBuiltInFuncs(tm *t.Map, ss []string, generic bool) (map[t.QID]*a.Func, error) {
	m := map[t.QID]*a.Func{}
	buf := []byte(nil)
	opts := parse.Options{
		AllowBuiltIns: true,
	}
	for _, s := range ss {
		buf = buf[:0]
		buf = append(buf, "pub func "...)
		buf = append(buf, s...)
		buf = append(buf, "{}\n"...)

		const filename = "builtin.wuffs"
		tokens, _, err := t.Tokenize(tm, filename, buf)
		if err != nil {
			return nil, fmt.Errorf("check: could not tokenize built-in funcs: %v", err)
		}
		if generic {
			for i := range tokens {
				if tokens[i].ID == builtin.GenericReplaceFrom {
					tokens[i].ID = builtin.GenericReplaceTo
				}
			}
		}
		file, err := parse.Parse(tm, filename, tokens, &opts)
		if err != nil {
			return nil, fmt.Errorf("check: could not parse built-in funcs: %v", err)
		}

		tlds := file.TopLevelDecls()
		if len(tlds) != 1 || tlds[0].Kind() != a.KFunc {
			return nil, fmt.Errorf("check: parsing %q: got %d top level decls, want %d", s, len(tlds), 1)
		}
		f := tlds[0].Func()
		m[f.QID()] = f
	}
	return m, nil
}

func (c *Checker) resolveFunc(typ *a.TypeExpr) (*a.Func, error) {
	if typ.Decorator().Key() != t.KeyOpenParen {
		return nil, fmt.Errorf("check: resolveFunc cannot look up non-func TypeExpr %q", typ.Str(c.tm))
	}
	qid := t.QID{typ.Receiver().Name(), typ.Name()}
	switch lTyp := typ.Receiver(); lTyp.Decorator().Key() {
	case 0:
		if f, err := c.builtInFunc(qid); err != nil {
			return nil, err
		} else if f != nil {
			return f, nil
		}
		if f := c.funcs[qid].Func; f != nil {
			return f, nil
		}

	case t.KeyColon:
		qid[0] = t.IDDiamond
		if f, err := c.builtInSliceFunc(qid); err != nil {
			return nil, err
		} else if f != nil {
			return f, nil
		}

	default:
		u := c.usees[lTyp.Decorator()]
		if u == nil {
			return nil, fmt.Errorf("check: cannot resolve %q in type %q",
				lTyp.Decorator().Str(c.tm), lTyp.Str(c.tm))
		}
		if f := u.funcs[qid]; f != nil {
			// TODO: f.Receiver().Str(c.tm) should be "deflate.decoder", not
			// just "decoder".
			return f, nil
		}
	}
	return nil, fmt.Errorf("check: resolveFunc cannot look up %q", typ.Str(c.tm))
}
