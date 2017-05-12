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
		idMap: m,
		funcs: map[t.QID]Func{},
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
	idMap *t.IDMap
	funcs map[t.QID]Func
}

func (c *Checker) Funcs() map[t.QID]Func { return c.funcs }

func (c *Checker) checkUse(n *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkStruct(n *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkFuncSignature(n *a.Node) error {
	f := n.Func()

	// TODO: check the in and out params.

	qid := f.QID()
	if _, ok := c.funcs[qid]; ok {
		if qid[0] == 0 {
			return fmt.Errorf(`check: duplicate function "%s"`, c.idMap.ByID(qid[1]))
		} else {
			return fmt.Errorf(`check: duplicate function "%s.%s"`, c.idMap.ByID(qid[0]), c.idMap.ByID(qid[1]))
		}
	}
	c.funcs[qid] = Func{
		QID:       qid,
		Func:      f,
		LocalVars: TypeMap{},
	}
	return nil
}

func (c *Checker) checkFuncBody(n *a.Node) error {
	f := n.Func()
	tc := &typeChecker{
		c:       c,
		idMap:   c.idMap,
		typeMap: c.funcs[f.QID()].LocalVars,
	}

	// Fill in the TypeMap with all local variables. Note that they have
	// function scope and can be hoisted, JavaScript style, a la
	// https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/var
	for _, m := range f.Body() {
		if err := tc.checkVars(m); err != nil {
			return fmt.Errorf("%v at %s:%d", err, m.Raw().Filename(), m.Raw().Line())
		}
	}

	// Assign ConstValue's (if applicable) and MType's to each Expr.
	for _, m := range f.Body() {
		if err := tc.checkStatement(m); err != nil {
			return fmt.Errorf("%v at %s:%d", err, m.Raw().Filename(), m.Raw().Line())
		}
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
