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

func (z facts) refine(n *a.Expr, min *big.Int, max *big.Int) (*big.Int, *big.Int) {
	if n.ID0() != 0 || !n.ID1().IsIdent() {
		// TODO.
		return min, max
	}

	for _, f := range z {
		f0, f1 := refine(f, n.ID1())
		if f0 != nil && min.Cmp(f0) < 0 {
			min = f0
		}
		if f1 != nil && max.Cmp(f1) > 0 {
			max = f1
		}
	}
	return min, max
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
	if cv := lhs.ConstValue(); cv != nil {
		rMin, rMax, err := q.bcheckExpr(rhs, 0)
		if err != nil {
			// TODO: should this function return the error?
			return false
		}
		if proveBinaryOpConstValues(op, cv, cv, rMin, rMax) {
			return true
		}
	}
	if cv := rhs.ConstValue(); cv != nil {
		lMin, lMax, err := q.bcheckExpr(lhs, 0)
		if err != nil {
			// TODO: should this function return the error?
			return false
		}
		if proveBinaryOpConstValues(op, lMin, lMax, cv, cv) {
			return true
		}
	}

	for _, f := range q.facts {
		if f.ID0().Key() == op && f.LHS().Expr().Eq(lhs) && f.RHS().Expr().Eq(rhs) {
			return true
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
