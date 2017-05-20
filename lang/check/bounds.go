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
	t.KeyI8:    {big.NewInt(-1 << 7), big.NewInt(1<<7 - 1)},
	t.KeyI16:   {big.NewInt(-1 << 15), big.NewInt(1<<15 - 1)},
	t.KeyI32:   {big.NewInt(-1 << 31), big.NewInt(1<<31 - 1)},
	t.KeyI64:   {big.NewInt(-1 << 63), big.NewInt(1<<63 - 1)},
	t.KeyU8:    {zero, big.NewInt(0).SetUint64(1<<8 - 1)},
	t.KeyU16:   {zero, big.NewInt(0).SetUint64(1<<16 - 1)},
	t.KeyU32:   {zero, big.NewInt(0).SetUint64(1<<32 - 1)},
	t.KeyU64:   {zero, big.NewInt(0).SetUint64(1<<64 - 1)},
	t.KeyUsize: {zero, zero},
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

func add1(i *big.Int) *big.Int {
	return big.NewInt(0).Add(i, one)
}

func sub1(i *big.Int) *big.Int {
	return big.NewInt(0).Sub(i, one)
}

func neg(i *big.Int) *big.Int {
	return big.NewInt(0).Neg(i)
}

func (c *checker) bcheckStatement(n *a.Node) error {
	c.errFilename, c.errLine = n.Raw().FilenameLine()

	switch n.Kind() {
	case a.KAssert:
		o := n.Assert()
		condition := o.Condition()
		err := errFailed
		if reasonID := o.Reason(); reasonID != 0 {
			if reasonFunc := c.reasonMap[reasonID.Key()]; reasonFunc != nil {
				err = reasonFunc(c, o)
			} else {
				err = fmt.Errorf("no such reason %s", reasonID.String(c.idMap))
			}
		} else if condition.ID0().IsBinaryOp() && condition.ID0().Key() != t.KeyAs {
			if c.proveBinaryOp(condition.ID0().Key(), condition.LHS().Expr(), condition.RHS().Expr()) {
				err = nil
			}
		}
		if err != nil {
			if err == errFailed {
				return fmt.Errorf("check: cannot prove %q", condition.String(c.idMap))
			}
			return fmt.Errorf("check: cannot prove %q: %v", condition.String(c.idMap), err)
		}
		c.facts.appendFact(condition)

	case a.KAssign:
		o := n.Assign()
		return c.bcheckAssignment(o.LHS(), o.LHS().MType(), o.Operator(), o.RHS())

	case a.KVar:
		o := n.Var()
		return c.bcheckAssignment(nil, o.XType(), t.IDEq, o.Value())

	case a.KWhile:
		return c.bcheckWhile(n.While())

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

func (c *checker) bcheckAssignment(lhs *a.Expr, lTyp *a.TypeExpr, op t.ID, rhs *a.Expr) error {
	if err := c.bcheckAssignment1(lhs, lTyp, op, rhs); err != nil {
		return err
	}
	// TODO: drop any facts involving lhs.
	return nil
}

func (c *checker) bcheckAssignment1(lhs *a.Expr, lTyp *a.TypeExpr, op t.ID, rhs *a.Expr) error {
	switch lTyp.PackageOrDecorator().Key() {
	case t.KeyPtr:
		// TODO: handle.
		return nil
	case t.KeyOpenBracket:
		// TODO: handle.
		return nil
	}
	if lTyp.Name().Key() == t.KeyBool {
		return nil
	}

	lMin, lMax, err := c.bcheckTypeExpr(lTyp)
	if err != nil {
		return err
	}

	rMin, rMax := (*big.Int)(nil), (*big.Int)(nil)
	if op == t.IDEq {
		cv := zero
		// rhs might be nil because "var x T" has an implicit "= 0".
		if rhs != nil {
			cv = rhs.ConstValue()
		}
		if cv != nil {
			if cv.Cmp(lMin) < 0 || cv.Cmp(lMax) > 0 {
				return fmt.Errorf("check: constant %v is not within bounds [%v..%v]", cv, lMin, lMax)
			}
			return nil
		}
		rMin, rMax, err = c.bcheckExpr(rhs, 0)
	} else {
		rMin, rMax, err = c.bcheckExprBinaryOp(lhs, op.BinaryForm().Key(), rhs, 0)
	}
	if err != nil {
		return err
	}
	if rMin.Cmp(lMin) < 0 || rMax.Cmp(lMax) > 0 {
		if op == t.IDEq {
			return fmt.Errorf("check: expression %q bounds [%v..%v] is not within bounds [%v..%v]",
				rhs.String(c.idMap), rMin, rMax, lMin, lMax)
		} else {
			return fmt.Errorf("check: assignment %q bounds [%v..%v] is not within bounds [%v..%v]",
				lhs.String(c.idMap)+" "+op.String(c.idMap)+" "+rhs.String(c.idMap),
				rMin, rMax, lMin, lMax)
		}
	}
	return nil
}

func (c *checker) bcheckWhile(n *a.While) error {
	if _, _, err := c.bcheckExpr(n.Condition(), 0); err != nil {
		return err
	}

	// Check the pre and assert conditions on entry.
	for _, m := range n.Asserts() {
		if m.Assert().Keyword().Key() == t.KeyPost {
			continue
		}
		// TODO
	}

	// Assume the pre and assert conditions, and the while condition. Check
	// the body.
	c.facts = c.facts[:0]
	for _, m := range n.Asserts() {
		if m.Assert().Keyword().Key() == t.KeyPost {
			continue
		}
		c.facts.appendFact(m.Assert().Condition())
	}
	c.facts.appendFact(n.Condition())
	for _, m := range n.Body() {
		if err := c.bcheckStatement(m); err != nil {
			return err
		}
	}

	// TODO: check the assert and post conditions on exit.

	// Assume the assert and post conditions.
	c.facts = c.facts[:0]
	for _, m := range n.Asserts() {
		if m.Assert().Keyword().Key() == t.KeyPre {
			continue
		}
		c.facts.appendFact(m.Assert().Condition())
	}
	return nil
}

func (c *checker) bcheckExpr(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	if depth > a.MaxExprDepth {
		return nil, nil, fmt.Errorf("check: expression recursion depth too large")
	}
	depth++
	if cv := n.ConstValue(); cv != nil {
		return cv, cv, nil
	}

	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		nMin, nMax, err := c.bcheckTypeExpr(n.MType())
		if err != nil {
			return nil, nil, err
		}
		nMin, nMax = c.facts.refine(n, nMin, nMax)
		return nMin, nMax, nil
	case t.FlagsUnaryOp:
		return c.bcheckExprUnaryOp(n, depth)
	case t.FlagsBinaryOp:
		if n.ID0().Key() == t.KeyXBinaryAs {
			// TODO.
			return zero, zero, nil
		}
		return c.bcheckExprBinaryOp(n.LHS().Expr(), n.ID0().Key(), n.RHS().Expr(), depth)
	case t.FlagsAssociativeOp:
		return c.bcheckExprAssociativeOp(n, depth)
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExpr", n.ID0().Key())
}

func (c *checker) bcheckExprUnaryOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	rMin, rMax, err := c.bcheckExpr(n.RHS().Expr(), depth)
	if err != nil {
		return nil, nil, err
	}

	switch n.ID0().Key() {
	case t.KeyXUnaryPlus:
		return rMin, rMax, nil
	case t.KeyXUnaryMinus:
		return neg(rMax), neg(rMin), nil
	case t.KeyXUnaryNot:
		return zero, one, nil
	}

	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprUnaryOp", n.ID0().Key())
}

func (c *checker) bcheckExprBinaryOp(lhs *a.Expr, op t.Key, rhs *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	lMin, lMax, err := c.bcheckExpr(lhs, depth)
	if err != nil {
		return nil, nil, err
	}
	rMin, rMax, err := c.bcheckExpr(rhs, depth)
	if err != nil {
		return nil, nil, err
	}

	switch op {
	case t.KeyXBinaryPlus:
		return big.NewInt(0).Add(lMin, rMin), big.NewInt(0).Add(lMax, rMax), nil
	case t.KeyXBinaryMinus:
		return big.NewInt(0).Sub(lMin, rMin), big.NewInt(0).Sub(lMax, rMax), nil
	case t.KeyXBinaryStar:
		// TODO.
		if cv := lhs.ConstValue(); cv != nil && cv.Cmp(zero) == 0 {
			return zero, zero, nil
		}
		if cv := rhs.ConstValue(); cv != nil && cv.Cmp(zero) == 0 {
			return zero, zero, nil
		}
	case t.KeyXBinarySlash:
	case t.KeyXBinaryShiftL:
	case t.KeyXBinaryShiftR:
	case t.KeyXBinaryAmp:
	case t.KeyXBinaryAmpHat:
	case t.KeyXBinaryPipe:
	case t.KeyXBinaryHat:

	case t.KeyXBinaryNotEq, t.KeyXBinaryLessThan, t.KeyXBinaryLessEq, t.KeyXBinaryEqEq,
		t.KeyXBinaryGreaterEq, t.KeyXBinaryGreaterThan, t.KeyXBinaryAnd, t.KeyXBinaryOr:
		return zero, one, nil

	case t.KeyXBinaryAs:
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprBinaryOp", op)
}

func (c *checker) bcheckExprAssociativeOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprAssociativeOp", n.ID0().Key())
}

func (c *checker) bcheckTypeExpr(n *a.TypeExpr) (*big.Int, *big.Int, error) {
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
				b[1] = sub1(cv)
			}
		}
	}
	return b[0], b[1], nil
}
