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

	lBounds0, lBounds1, err := c.bcheckTypeExpr(lTyp)
	if err != nil {
		return err
	}

	cv := zero
	if rhs != nil {
		cv = rhs.ConstValue()
	}
	if cv != nil {
		if cv.Cmp(lBounds0) >= 0 && cv.Cmp(lBounds1) <= 0 {
			return nil
		}
		return fmt.Errorf("check: constant %v is not within bounds [%v..%v]", cv, lBounds0, lBounds1)
	}

	rBounds0, rBounds1, err := c.bcheckExpr(rhs, 0)
	if err != nil {
		return err
	}
	if rBounds0.Cmp(lBounds0) >= 0 && rBounds1.Cmp(lBounds1) <= 0 {
		return nil
	}
	return fmt.Errorf("check: expression %q bounds [%v..%v] is not within bounds [%v..%v]",
		rhs.String(c.idMap), rBounds0, rBounds1, lBounds0, lBounds1)
}

func (c *typeChecker) bcheckExpr(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	if depth > a.MaxExprDepth {
		return nil, nil, fmt.Errorf("check: expression recursion depth too large")
	}
	depth++

	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		if cv := n.ConstValue(); cv != nil {
			return cv, cv, nil
		}
		return c.bcheckTypeExpr(n.MType())
	case t.FlagsUnaryOp:
		return c.bcheckExprUnaryOp(n, depth)
	case t.FlagsBinaryOp:
		return c.bcheckExprBinaryOp(n, depth)
	case t.FlagsAssociativeOp:
		return c.bcheckExprAssociativeOp(n, depth)
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExpr", n.ID0().Key())
}

func (c *typeChecker) bcheckExprUnaryOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprUnaryOp", n.ID0().Key())
}

func (c *typeChecker) bcheckExprBinaryOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	l0, l1, err := c.bcheckExpr(n.LHS().Expr(), depth)
	if err != nil {
		return nil, nil, err
	}
	r0, r1, err := c.bcheckExpr(n.RHS().Expr(), depth)
	if err != nil {
		return nil, nil, err
	}

	switch n.ID0() {
	case t.IDXBinaryPlus:
		return big.NewInt(0).Add(l0, r0), big.NewInt(0).Add(l1, r1), nil
	case t.IDXBinaryMinus:
	case t.IDXBinaryStar:
	case t.IDXBinarySlash:
	case t.IDXBinaryShiftL:
	case t.IDXBinaryShiftR:
	case t.IDXBinaryAmp:
	case t.IDXBinaryAmpHat:
	case t.IDXBinaryPipe:
	case t.IDXBinaryHat:
	case t.IDXBinaryNotEq:
	case t.IDXBinaryLessThan:
	case t.IDXBinaryLessEq:
	case t.IDXBinaryEqEq:
	case t.IDXBinaryGreaterEq:
	case t.IDXBinaryGreaterThan:
	case t.IDXBinaryAnd:
	case t.IDXBinaryOr:
	case t.IDXBinaryAs:
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprBinaryOp", n.ID0().Key())
}

func (c *typeChecker) bcheckExprAssociativeOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprAssociativeOp", n.ID0().Key())
}

func (c *typeChecker) bcheckTypeExpr(n *a.TypeExpr) (*big.Int, *big.Int, error) {
	b := [2]*big.Int{}
	if key := n.Name().Key(); key < t.Key(len(numTypeBounds)) {
		b = numTypeBounds[key]
	}
	if b[0] == nil || b[1] == nil {
		return nil, nil, fmt.Errorf("check: internal error: unknown bounds for %q", n.String(c.idMap))
	}
	if n.IsRefined() {
		if x := n.InclMin(); x != nil {
			if cv := x.ConstValue(); cv != nil && b[0].Cmp(cv) < 0 {
				b[0] = cv
			}
		}
		if x := n.ExclMax(); x != nil {
			if cv := x.ConstValue(); cv != nil && b[1].Cmp(cv) > 0 {
				b[1] = big.NewInt(0).Sub(cv, one)
			}
		}
	}
	return b[0], b[1], nil
}
