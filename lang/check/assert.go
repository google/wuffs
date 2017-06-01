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

type facts []*a.Expr

func (z *facts) appendFact(x *a.Expr) {
	// TODO: make this faster than O(N) by keeping facts sorted somehow?
	for _, f := range *z {
		if f.Eq(x) {
			return
		}
	}
	*z = append(*z, x)
}

// drop drops any facts that mention x. They are replaced with a nil *a.Expr,
// and then the slice is compacted to remove all nils.
func (z *facts) drop(x *a.Expr) {
	i := 0
	for _, f := range *z {
		if !f.Mentions(x) {
			(*z)[i] = f
			i++
		}
	}
	for j := i; j < len(*z); j++ {
		(*z)[j] = nil
	}
	*z = (*z)[:i]
}

func (z facts) refine(n *a.Expr, nMin *big.Int, nMax *big.Int) (*big.Int, *big.Int) {
	if n.ID0() != 0 || !n.ID1().IsIdent() {
		// TODO.
		return nMin, nMax
	}

	for _, f := range z {
		fMin, fMax := refine(f, n.ID1())
		if fMin != nil && nMin.Cmp(fMin) < 0 {
			nMin = fMin
		}
		if fMax != nil && nMax.Cmp(fMax) > 0 {
			nMax = fMax
		}
	}
	return nMin, nMax
}

// refine returns fact's lower or upper bound for id.
func refine(fact *a.Expr, id t.ID) (*big.Int, *big.Int) {
	op := fact.ID0()
	if !op.IsBinaryOp() || op.Key() == t.KeyAs {
		return nil, nil
	}

	lhs := fact.LHS().Expr()
	rhs := fact.RHS().Expr()
	cv := (*big.Int)(nil)
	idLeft := lhs.ID0() == 0 && lhs.ID1() == id
	if idLeft {
		cv = rhs.ConstValue()
	} else {
		if rhs.ID0() != 0 || rhs.ID1() != id {
			return nil, nil
		}
		cv = lhs.ConstValue()
	}
	if cv == nil {
		return nil, nil
	}

	switch op.Key() {
	case t.KeyXBinaryLessThan:
		if idLeft {
			return nil, sub1(cv)
		} else {
			return add1(cv), nil
		}
	case t.KeyXBinaryLessEq:
		if idLeft {
			return nil, cv
		} else {
			return cv, nil
		}
	case t.KeyXBinaryGreaterEq:
		if idLeft {
			return cv, nil
		} else {
			return nil, cv
		}
	case t.KeyXBinaryGreaterThan:
		if idLeft {
			return add1(cv), nil
		} else {
			return nil, sub1(cv)
		}
	}

	return nil, nil
}

func argValue(m *t.IDMap, args []*a.Node, name string) *a.Expr {
	if x := m.ByName(name); x != 0 {
		for _, a := range args {
			if a.Arg().Name() == x {
				return a.Arg().Value()
			}
		}
	}
	return nil
}

// parseBinaryOp parses n as "lhs op rhs".
func parseBinaryOp(n *a.Expr) (op t.Key, lhs *a.Expr, rhs *a.Expr) {
	if !n.ID0().IsBinaryOp() {
		return 0, nil, nil
	}
	op = n.ID0().Key()
	if op == t.KeyAs {
		return 0, nil, nil
	}
	return op, n.LHS().Expr(), n.RHS().Expr()
}

func proveBinaryOpConstValues(op t.Key, lMin *big.Int, lMax *big.Int, rMin *big.Int, rMax *big.Int) (ok bool) {
	switch op {
	case t.KeyXBinaryNotEq:
		return lMax.Cmp(rMin) < 0 || lMin.Cmp(rMax) > 0
	case t.KeyXBinaryLessThan:
		return lMax.Cmp(rMin) < 0
	case t.KeyXBinaryLessEq:
		return lMax.Cmp(rMin) <= 0
	case t.KeyXBinaryEqEq:
		return lMin.Cmp(rMax) == 0 && lMax.Cmp(rMin) == 0
	case t.KeyXBinaryGreaterEq:
		return lMin.Cmp(rMax) >= 0
	case t.KeyXBinaryGreaterThan:
		return lMin.Cmp(rMax) > 0
	}
	return false
}

func (q *checker) proveBinaryOp(op t.Key, lhs *a.Expr, rhs *a.Expr) (ok bool) {
	lhsCV := lhs.ConstValue()
	if lhsCV != nil {
		rMin, rMax, err := q.bcheckExpr(rhs, 0)
		if err != nil {
			// TODO: should this function return the error?
			return false
		}
		if proveBinaryOpConstValues(op, lhsCV, lhsCV, rMin, rMax) {
			return true
		}
	}
	rhsCV := rhs.ConstValue()
	if rhsCV != nil {
		lMin, lMax, err := q.bcheckExpr(lhs, 0)
		if err != nil {
			// TODO: should this function return the error?
			return false
		}
		if proveBinaryOpConstValues(op, lMin, lMax, rhsCV, rhsCV) {
			return true
		}
	}

	for _, f := range q.facts {
		if !f.LHS().Expr().Eq(lhs) {
			continue
		}
		factOp := f.ID0().Key()
		if factOp == op && f.RHS().Expr().Eq(rhs) {
			return true
		}

		if factOp == t.KeyXBinaryEqEq && rhsCV != nil {
			if factCV := f.RHS().Expr().ConstValue(); factCV != nil {
				switch op {
				case t.KeyXBinaryNotEq:
					return factCV.Cmp(rhsCV) != 0
				case t.KeyXBinaryLessThan:
					return factCV.Cmp(rhsCV) < 0
				case t.KeyXBinaryLessEq:
					return factCV.Cmp(rhsCV) <= 0
				case t.KeyXBinaryEqEq:
					return factCV.Cmp(rhsCV) == 0
				case t.KeyXBinaryGreaterEq:
					return factCV.Cmp(rhsCV) >= 0
				case t.KeyXBinaryGreaterThan:
					return factCV.Cmp(rhsCV) > 0
				}
			}
		}
	}
	return false
}

var errFailed = errors.New("failed")

type reason func(q *checker, n *a.Assert) error

type reasonMap map[t.Key]reason

var reasons = [...]struct {
	s string
	r reason
}{
	{`"a < b: a < c <= b"`, func(q *checker, n *a.Assert) error {
		c := argValue(q.idMap, n.Args(), "c")
		if c == nil {
			return errFailed
		}
		op, a, b := parseBinaryOp(n.Condition())
		if op != t.KeyXBinaryLessThan {
			return errFailed
		}
		if !q.proveBinaryOp(t.KeyXBinaryLessThan, a, c) {
			return fmt.Errorf("cannot prove \"%s < %s\"", a.String(q.idMap), c.String(q.idMap))
		}
		if !q.proveBinaryOp(t.KeyXBinaryLessEq, c, b) {
			return fmt.Errorf("cannot prove \"%s <= %s\"", c.String(q.idMap), b.String(q.idMap))
		}
		return nil
	}},
}
