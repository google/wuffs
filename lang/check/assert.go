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

func (z *facts) appendFact(fact *a.Expr) {
	// TODO: make this faster than O(N) by keeping facts sorted somehow?
	for _, x := range *z {
		if x.Eq(fact) {
			return
		}
	}

	switch fact.ID0().Key() {
	case 0:
		for _, f := range *z {
			if f.ID0().Key() != t.KeyXBinaryEqEq {
				continue
			}
			if fact.Eq(f.LHS().Expr()) {
				z.appendFact(f.RHS().Expr())
			} else if fact.Eq(f.RHS().Expr()) {
				z.appendFact(f.LHS().Expr())
			}
		}
	case t.KeyXBinaryAnd:
		z.appendFact(fact.LHS().Expr())
		z.appendFact(fact.RHS().Expr())
		return
	case t.KeyXAssociativeAnd:
		// TODO.
	}

	*z = append(*z, fact)
}

// update applies f to each fact, replacing the slice element with the result
// of the function call. The slice is then compacted to remove all nils.
func (z *facts) update(f func(*a.Expr) *a.Expr) {
	i := 0
	for _, x := range *z {
		x = f(x)
		if x != nil {
			(*z)[i] = x
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

	for _, x := range z {
		xMin, xMax := refine(x, n.ID1())
		if xMin != nil && nMin.Cmp(xMin) < 0 {
			nMin = xMin
		}
		if xMax != nil && nMax.Cmp(xMax) > 0 {
			nMax = xMax
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

// simplify returns a simplified form of n. For example, (x - x) becomes 0.
func simplify(n *a.Expr) *a.Expr {
	// TODO: be rigorous about this, not ad hoc.
	switch op, lhs, rhs := parseBinaryOp(n); op.Key() {
	case t.KeyXBinaryPlus:
		// TODO: constant folding, so ((x + 1) + 1) becomes (x + 2).

	case t.KeyXBinaryMinus:
		if lhs.Eq(rhs) {
			return zeroExpr
		}
		if lOp, lLHS, lRHS := parseBinaryOp(lhs); lOp.Key() == t.KeyXBinaryPlus {
			if lLHS.Eq(rhs) {
				return lRHS
			}
			if lRHS.Eq(rhs) {
				return lLHS
			}
		}
	}
	return n
}

func argValue(tm *t.Map, args []*a.Node, name string) *a.Expr {
	if x := tm.ByName(name); x != 0 {
		for _, a := range args {
			if a.Arg().Name() == x {
				return a.Arg().Value()
			}
		}
	}
	return nil
}

// parseBinaryOp parses n as "lhs op rhs".
func parseBinaryOp(n *a.Expr) (op t.ID, lhs *a.Expr, rhs *a.Expr) {
	if !n.ID0().IsBinaryOp() {
		return 0, nil, nil
	}
	op = n.ID0()
	if op.Key() == t.KeyAs {
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

	for _, x := range q.facts {
		if !x.LHS().Expr().Eq(lhs) {
			continue
		}
		factOp := x.ID0().Key()
		if factOp == op && x.RHS().Expr().Eq(rhs) {
			return true
		}

		if factOp == t.KeyXBinaryEqEq && rhsCV != nil {
			if factCV := x.RHS().Expr().ConstValue(); factCV != nil {
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
	{`"a < (b + c): a < c; 0 <= b"`, func(q *checker, n *a.Assert) error {
		op, a, bc := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessThan {
			return errFailed
		}
		op, b, c := parseBinaryOp(bc)
		if op.Key() != t.KeyXBinaryPlus {
			return errFailed
		}
		if !q.proveBinaryOp(t.KeyXBinaryLessThan, a, c) {
			return fmt.Errorf("cannot prove \"%s < %s\"", a.String(q.tm), c.String(q.tm))
		}
		if !q.proveBinaryOp(t.KeyXBinaryLessEq, zeroExpr, b) {
			return fmt.Errorf("cannot prove \"%s <= %s\"", zeroExpr.String(q.tm), b.String(q.tm))
		}
		return nil
	}},
	{`"a < b: a < c; c <= b"`, func(q *checker, n *a.Assert) error {
		c := argValue(q.tm, n.Args(), "c")
		if c == nil {
			return errFailed
		}
		op, a, b := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessThan {
			return errFailed
		}
		if !q.proveBinaryOp(t.KeyXBinaryLessThan, a, c) {
			return fmt.Errorf("cannot prove \"%s < %s\"", a.String(q.tm), c.String(q.tm))
		}
		if !q.proveBinaryOp(t.KeyXBinaryLessEq, c, b) {
			return fmt.Errorf("cannot prove \"%s <= %s\"", c.String(q.tm), b.String(q.tm))
		}
		return nil
	}},
}
