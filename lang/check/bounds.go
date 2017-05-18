// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

var numTypeBounds = [256][2]*big.Int{
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

func (c *typeChecker) bcheckStatement(n *a.Node) error {
	c.errFilename, c.errLine = n.Raw().FilenameLine()

	switch n.Kind() {
	case a.KAssert:
		// TODO.

	case a.KAssign:
		// TODO.

	case a.KVar:
		o := n.Var()
		return c.bcheckAssignment(o.XType(), o.Value())

	case a.KWhile:
		// TODO.

	case a.KIf:
		// TODO.

	case a.KReturn:
		// TODO.

	case a.KJump:
		// No-op.

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%d) for checkStatement", n.Kind())
	}

	return nil
}

func (c *typeChecker) bcheckAssignment(lTyp *a.TypeExpr, rhs *a.Expr) error {
	switch lTyp.PackageOrDecorator() {
	case t.IDPtr:
		// TODO: handle.
		return nil
	case t.IDOpenBracket:
		// TODO: handle.
		return nil
	}
	if lTyp.Name() == t.IDBool {
		return nil
	}

	bounds := [2]*big.Int{}
	if key := lTyp.Name().Key(); key < t.Key(len(numTypeBounds)) {
		bounds = numTypeBounds[key]
	}
	if bounds[0] == nil || bounds[1] == nil {
		return fmt.Errorf("check: internal error: unknown bounds for %q", lTyp.String(c.idMap))
	}

	if lTyp.IsRefined() {
		if x := lTyp.InclMin(); x != nil {
			if cv := x.ConstValue(); cv != nil && bounds[0].Cmp(cv) < 0 {
				bounds[0] = cv
			}
		}
		if x := lTyp.ExclMax(); x != nil {
			if cv := x.ConstValue(); cv != nil && bounds[1].Cmp(cv) > 0 {
				bounds[1] = big.NewInt(0).Sub(cv, one)
			}
		}
	}

	cv := zero
	if rhs != nil {
		cv = rhs.ConstValue()
	}
	if cv != nil {
		if cv.Cmp(bounds[0]) >= 0 && cv.Cmp(bounds[1]) <= 0 {
			return nil
		}
		return fmt.Errorf("check: cannot prove constant %v within bounds [%v..%v]",
			cv, bounds[0], bounds[1])
	}

	return fmt.Errorf("check: cannot prove expression %q within bounds [%v..%v]",
		rhs.String(c.idMap), bounds[0], bounds[1])
}
