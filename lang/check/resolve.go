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

package check

import (
	"fmt"

	"github.com/google/wuffs/lang/builtin"
	"github.com/google/wuffs/lang/parse"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var (
	exprArgs    = a.NewExpr(0, 0, 0, t.IDArgs, nil, nil, nil, nil)
	exprNullptr = a.NewExpr(0, 0, 0, t.IDNullptr, nil, nil, nil, nil)
	exprThis    = a.NewExpr(0, 0, 0, t.IDThis, nil, nil, nil, nil)
)

var (
	typeExprGeneric1    = a.NewTypeExpr(0, t.IDBase, t.IDDagger1, nil, nil, nil)
	typeExprGeneric2    = a.NewTypeExpr(0, t.IDBase, t.IDDagger2, nil, nil, nil)
	typeExprIdeal       = a.NewTypeExpr(0, t.IDBase, t.IDQIdeal, nil, nil, nil)
	typeExprList        = a.NewTypeExpr(0, t.IDBase, t.IDComma, nil, nil, nil)
	typeExprNullptr     = a.NewTypeExpr(0, t.IDBase, t.IDQNullptr, nil, nil, nil)
	typeExprPlaceholder = a.NewTypeExpr(0, t.IDBase, t.IDQPlaceholder, nil, nil, nil)
	typeExprTypeExpr    = a.NewTypeExpr(0, t.IDBase, t.IDQTypeExpr, nil, nil, nil)

	typeExprU8  = a.NewTypeExpr(0, t.IDBase, t.IDU8, nil, nil, nil)
	typeExprU16 = a.NewTypeExpr(0, t.IDBase, t.IDU16, nil, nil, nil)
	typeExprU32 = a.NewTypeExpr(0, t.IDBase, t.IDU32, nil, nil, nil)
	typeExprU64 = a.NewTypeExpr(0, t.IDBase, t.IDU64, nil, nil, nil)

	typeExprEmptyStruct = a.NewTypeExpr(0, t.IDBase, t.IDEmptyStruct, nil, nil, nil)
	typeExprBool        = a.NewTypeExpr(0, t.IDBase, t.IDBool, nil, nil, nil)
	typeExprUtility     = a.NewTypeExpr(0, t.IDBase, t.IDUtility, nil, nil, nil)

	typeExprRangeIEU32 = a.NewTypeExpr(0, t.IDBase, t.IDRangeIEU32, nil, nil, nil)
	typeExprRangeIIU32 = a.NewTypeExpr(0, t.IDBase, t.IDRangeIIU32, nil, nil, nil)
	typeExprRangeIEU64 = a.NewTypeExpr(0, t.IDBase, t.IDRangeIEU64, nil, nil, nil)
	typeExprRangeIIU64 = a.NewTypeExpr(0, t.IDBase, t.IDRangeIIU64, nil, nil, nil)
	typeExprRectIEU32  = a.NewTypeExpr(0, t.IDBase, t.IDRectIEU32, nil, nil, nil)
	typeExprRectIIU32  = a.NewTypeExpr(0, t.IDBase, t.IDRectIIU32, nil, nil, nil)

	typeExprIOReader = a.NewTypeExpr(0, t.IDBase, t.IDIOReader, nil, nil, nil)
	typeExprIOWriter = a.NewTypeExpr(0, t.IDBase, t.IDIOWriter, nil, nil, nil)
	typeExprStatus   = a.NewTypeExpr(0, t.IDBase, t.IDStatus, nil, nil, nil)

	typeExprFrameConfig   = a.NewTypeExpr(0, t.IDBase, t.IDFrameConfig, nil, nil, nil)
	typeExprImageConfig   = a.NewTypeExpr(0, t.IDBase, t.IDImageConfig, nil, nil, nil)
	typeExprPixelBuffer   = a.NewTypeExpr(0, t.IDBase, t.IDPixelBuffer, nil, nil, nil)
	typeExprPixelConfig   = a.NewTypeExpr(0, t.IDBase, t.IDPixelConfig, nil, nil, nil)
	typeExprPixelSwizzler = a.NewTypeExpr(0, t.IDBase, t.IDPixelSwizzler, nil, nil, nil)

	typeExprDecodeFrameOptions = a.NewTypeExpr(0, t.IDBase, t.IDDecodeFrameOptions, nil, nil, nil)

	typeExprSliceU8 = a.NewTypeExpr(t.IDSlice, 0, 0, nil, nil, typeExprU8)
	typeExprTableU8 = a.NewTypeExpr(t.IDTable, 0, 0, nil, nil, typeExprU8)
)

func setPlaceholderMBoundsMType(n *a.Node) {
	n.SetMBounds(bounds{zero, zero})
	n.SetMType(typeExprPlaceholder)
}

// typeMap maps from variable names (as token IDs) to types.
type typeMap map[t.ID]*a.TypeExpr

var builtInTypeMap = typeMap{
	t.IDU8:  typeExprU8,
	t.IDU16: typeExprU16,
	t.IDU32: typeExprU32,
	t.IDU64: typeExprU64,

	t.IDEmptyStruct: typeExprEmptyStruct,
	t.IDBool:        typeExprBool,
	t.IDUtility:     typeExprUtility,

	t.IDRangeIEU32: typeExprRangeIEU32,
	t.IDRangeIIU32: typeExprRangeIIU32,
	t.IDRangeIEU64: typeExprRangeIEU64,
	t.IDRangeIIU64: typeExprRangeIIU64,
	t.IDRectIEU32:  typeExprRectIEU32,
	t.IDRectIIU32:  typeExprRectIIU32,

	t.IDIOReader: typeExprIOReader,
	t.IDIOWriter: typeExprIOWriter,
	t.IDStatus:   typeExprStatus,

	t.IDFrameConfig:   typeExprFrameConfig,
	t.IDImageConfig:   typeExprImageConfig,
	t.IDPixelBuffer:   typeExprPixelBuffer,
	t.IDPixelConfig:   typeExprPixelConfig,
	t.IDPixelSwizzler: typeExprPixelSwizzler,

	t.IDDecodeFrameOptions: typeExprDecodeFrameOptions,
}

func (c *Checker) parseBuiltInFuncs(ss []string, generic bool) (map[t.QQID]*a.Func, error) {
	m := map[t.QQID]*a.Func(nil)
	if generic {
		m = map[t.QQID]*a.Func{}
	}

	buf := []byte(nil)
	for _, s := range ss {
		buf = buf[:0]
		buf = append(buf, "pub func "...)
		buf = append(buf, s...)
		buf = append(buf, "{}\n"...)

		const filename = "builtin.wuffs"
		tokens, _, err := t.Tokenize(c.tm, filename, buf)
		if err != nil {
			return nil, fmt.Errorf("check: parsing %q: could not tokenize built-in funcs: %v", s, err)
		}
		if generic {
			for i := range tokens {
				if tokens[i].ID == builtin.GenericOldName1 {
					tokens[i].ID = builtin.GenericNewName1
				} else if tokens[i].ID == builtin.GenericOldName2 {
					tokens[i].ID = builtin.GenericNewName2
				}
			}
		}
		file, err := parse.Parse(c.tm, filename, tokens, &parse.Options{
			AllowBuiltInNames: true,
		})
		if err != nil {
			return nil, fmt.Errorf("check: parsing %q: could not parse built-in funcs: %v", s, err)
		}

		tlds := file.TopLevelDecls()
		if len(tlds) != 1 || tlds[0].Kind() != a.KFunc {
			return nil, fmt.Errorf("check: parsing %q: got %d top level decls, want %d", s, len(tlds), 1)
		}
		f := tlds[0].AsFunc()
		f.AsNode().AsRaw().SetPackage(c.tm, t.IDBase)
		if err := c.checkFuncSignature(f.AsNode()); err != nil {
			return nil, err
		}

		if m != nil {
			m[f.QQID()] = f
		}
	}
	return m, nil
}

func (c *Checker) resolveFunc(typ *a.TypeExpr) (*a.Func, error) {
	if typ.Decorator() != t.IDFunc {
		return nil, fmt.Errorf("check: resolveFunc cannot look up non-func TypeExpr %q", typ.Str(c.tm))
	}
	lTyp := typ.Receiver()
	lQID := lTyp.QID()
	qqid := t.QQID{lQID[0], lQID[1], typ.FuncName()}

	if lTyp.IsSliceType() {
		qqid[0] = t.IDBase
		qqid[1] = t.IDDagger1
		if f := c.builtInSliceFuncs[qqid]; f != nil {
			return f, nil
		}

	} else if lTyp.IsTableType() {
		qqid[0] = t.IDBase
		qqid[1] = t.IDDagger2
		if f := c.builtInTableFuncs[qqid]; f != nil {
			return f, nil
		}

	} else if f := c.funcs[qqid]; f != nil {
		return f, nil
	}
	return nil, fmt.Errorf("check: resolveFunc cannot look up %q", typ.Str(c.tm))
}
