// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"errors"
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

// TypeExprFoo is an *ast.Node MType (implicit type) that means that n is a
// boolean or a ideal numeric constant.
var (
	TypeExprBoolean     = a.NewTypeExpr(0, t.IDBool, nil, nil, nil)
	TypeExprIdealNumber = a.NewTypeExpr(0, t.IDIdeal, nil, nil, nil)
)

func numeric(n *a.TypeExpr) bool { return n == TypeExprIdealNumber || n.IsNumType() }

// TypeMap maps from variable names (as token IDs) to types.
type TypeMap map[t.ID]*a.TypeExpr

type Struct struct {
	ID     t.ID
	Struct *a.Struct
}

type Func struct {
	QID       t.QID
	Func      *a.Func
	LocalVars TypeMap
}

func Check(m *t.IDMap, files ...*a.File) (*Checker, error) {
	for _, f := range files {
		if f == nil {
			return nil, errors.New("check: Check given a nil *ast.File")
		}
	}

	if len(files) > 1 {
		m := map[string]bool{}
		for _, f := range files {
			if m[f.Filename()] {
				return nil, fmt.Errorf("check: Check given duplicate filename %q", f.Filename())
			}
			m[f.Filename()] = true
		}
	}

	c := &Checker{
		idMap:   m,
		structs: map[t.ID]Struct{},
		funcs:   map[t.QID]Func{},
	}
	for _, phase := range phases {
		for _, f := range files {
			for _, n := range f.TopLevelDecls() {
				if n.Kind() != phase.kind {
					continue
				}
				if err := phase.check(c, n); err != nil {
					return nil, err
				}
			}
		}
	}
	return c, nil
}

var phases = [...]struct {
	kind  a.Kind
	check func(*Checker, *a.Node) error
}{
	{a.KUse, (*Checker).checkUse},
	{a.KStruct, (*Checker).checkStruct},
	{a.KFunc, (*Checker).checkFuncSignature},
	{a.KFunc, (*Checker).checkFuncBody},
}

type Checker struct {
	idMap   *t.IDMap
	structs map[t.ID]Struct
	funcs   map[t.QID]Func
}

func (c *Checker) Funcs() map[t.QID]Func { return c.funcs }

func (c *Checker) checkUse(n *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkFields(fields []*a.Node) error {
	if len(fields) == 0 {
		return nil
	}
	tc := &typeChecker{
		c:     c,
		idMap: c.idMap,
	}
	fieldNames := map[t.ID]bool{}
	for _, n := range fields {
		f := n.Field()
		if _, ok := fieldNames[f.Name()]; ok {
			return fmt.Errorf("check: duplicate field %q", c.idMap.ByID(f.Name()))
		}
		if err := tc.checkTypeExpr(f.XType()); err != nil {
			return fmt.Errorf("%v in field %q", err, c.idMap.ByID(f.Name()))
		}
		fieldNames[f.Name()] = true
	}
	return nil
}

func (c *Checker) checkStruct(n *a.Node) error {
	s := n.Struct()
	if err := c.checkFields(s.Fields()); err != nil {
		return fmt.Errorf("%v in struct %q at %s:%d", err, c.idMap.ByID(s.Name()), s.Filename(), s.Line())
	}
	id := s.Name()
	if other, ok := c.structs[id]; ok {
		return fmt.Errorf("check: duplicate struct %q at %s:%d and %s:%d",
			c.idMap.ByID(id), other.Struct.Filename(), other.Struct.Line(), s.Filename(), s.Line())
	}
	c.structs[id] = Struct{
		ID:     id,
		Struct: s,
	}
	return nil
}

func (c *Checker) checkFuncSignature(n *a.Node) error {
	f := n.Func()
	if err := c.checkFields(f.In().Fields()); err != nil {
		return fmt.Errorf("%v in in-params for func %q at %s:%d",
			err, c.idMap.ByID(f.Name()), f.Filename(), f.Line())
	}
	if err := c.checkFields(f.Out().Fields()); err != nil {
		return fmt.Errorf("%v in out-params for func %q at %s:%d",
			err, c.idMap.ByID(f.Name()), f.Filename(), f.Line())
	}
	qid := f.QID()
	if other, ok := c.funcs[qid]; ok {
		return fmt.Errorf("check: duplicate function %q at %s:%d and %s:%d",
			qid.String(c.idMap), other.Func.Filename(), other.Func.Line(), f.Filename(), f.Line())
	}
	localVars := TypeMap{
		t.IDIn:  a.NewTypeExpr(0, f.In().Name(), nil, nil, nil),
		t.IDOut: a.NewTypeExpr(0, f.Out().Name(), nil, nil, nil),
	}
	if qid[0] != 0 {
		if _, ok := c.structs[qid[0]]; !ok {
			return fmt.Errorf("check: no receiver struct defined for function %q at %s:%d",
				qid.String(c.idMap), f.Filename(), f.Line())
		}
		localVars[t.IDThis] = a.NewTypeExpr(t.IDPtr, 0, nil, nil, a.NewTypeExpr(0, qid[0], nil, nil, nil))
	}
	c.funcs[qid] = Func{
		QID:       qid,
		Func:      f,
		LocalVars: localVars,
	}
	return nil
}

func (c *Checker) checkFuncBody(n *a.Node) error {
	f := n.Func()
	tc := &typeChecker{
		c:     c,
		idMap: c.idMap,
		f:     c.funcs[f.QID()],
	}

	// Fill in the TypeMap with all local variables. Note that they have
	// function scope and can be hoisted, JavaScript style, a la
	// https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/var
	for _, m := range f.Body() {
		if err := tc.checkVars(m); err != nil {
			return fmt.Errorf("%v at %s:%d", err, tc.errFilename, tc.errLine)
		}
	}

	// Assign ConstValue's (if applicable) and MType's to each Expr.
	for _, m := range f.Body() {
		if err := tc.checkStatement(m); err != nil {
			return fmt.Errorf("%v at %s:%d", err, tc.errFilename, tc.errLine)
		}
	}
	if err := f.Node().Walk(func(n *a.Node) error {
		if n.Kind() == a.KExpr {
			e := n.Expr()
			if typ := e.MType(); typ == nil {
				return fmt.Errorf("check: internal error: expression node %q has no (implicit) type",
					e.String(tc.idMap))
			} else if typ == TypeExprIdealNumber && e.ConstValue() == nil {
				return fmt.Errorf("check: internal error: expression node %q has ideal number type "+
					"but no const value", e.String(tc.idMap))
			}
		}
		return nil
	}); err != nil {
		return err
	}

	// TODO: bounds and assertion checking.

	return nil
}

// TODO: use numTypeRanges.

var numTypeRanges = [256][2]*big.Int{
	t.IDI8 >> t.KeyShift:    {big.NewInt(-1 << 7), big.NewInt(1<<7 - 1)},
	t.IDI16 >> t.KeyShift:   {big.NewInt(-1 << 15), big.NewInt(1<<15 - 1)},
	t.IDI32 >> t.KeyShift:   {big.NewInt(-1 << 31), big.NewInt(1<<31 - 1)},
	t.IDI64 >> t.KeyShift:   {big.NewInt(-1 << 63), big.NewInt(1<<63 - 1)},
	t.IDU8 >> t.KeyShift:    {zero, big.NewInt(0).SetUint64(1<<8 - 1)},
	t.IDU16 >> t.KeyShift:   {zero, big.NewInt(0).SetUint64(1<<16 - 1)},
	t.IDU32 >> t.KeyShift:   {zero, big.NewInt(0).SetUint64(1<<32 - 1)},
	t.IDU64 >> t.KeyShift:   {zero, big.NewInt(0).SetUint64(1<<64 - 1)},
	t.IDUsize >> t.KeyShift: {zero, zero},
}

var (
	zero = big.NewInt(0)
	one  = big.NewInt(1)
	ffff = big.NewInt(0xFFFF)
)

func btoi(b bool) *big.Int {
	if b {
		return one
	}
	return zero
}
