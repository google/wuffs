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

// otherHandSide returns the operator and other hand side when n is an
// binary-op expression like "thisHS == thatHS" or "thatHS < thisHS" (which is
// equivalent to "thisHS > thatHS"). If not, it returns (0, nil).
func otherHandSide(n *a.Expr, thisHS *a.Expr) (op t.ID, thatHS *a.Expr) {
	op = n.ID0()

	reverseOp := t.ID(0)
	switch op.Key() {
	case t.KeyXBinaryNotEq:
		reverseOp = t.IDXBinaryNotEq
	case t.KeyXBinaryLessThan:
		reverseOp = t.IDXBinaryGreaterThan
	case t.KeyXBinaryLessEq:
		reverseOp = t.IDXBinaryGreaterEq
	case t.KeyXBinaryEqEq:
		reverseOp = t.IDXBinaryEqEq
	case t.KeyXBinaryGreaterEq:
		reverseOp = t.IDXBinaryLessEq
	case t.KeyXBinaryGreaterThan:
		reverseOp = t.IDXBinaryLessThan
	}

	if reverseOp != 0 {
		if thisHS.Eq(n.LHS().Expr()) {
			return op, n.RHS().Expr()
		}
		if thisHS.Eq(n.RHS().Expr()) {
			return reverseOp, n.LHS().Expr()
		}
	}
	return 0, nil
}

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
		for _, x := range *z {
			if op, other := otherHandSide(x, fact); op.Key() == t.KeyXBinaryEqEq {
				z.appendFact(other)
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

func (z facts) refine(n *a.Expr, nMin *big.Int, nMax *big.Int, tm *t.Map) (*big.Int, *big.Int, error) {
	if nMin == nil || nMax == nil {
		return nMin, nMax, nil
	}

	for _, x := range z {
		op, other := otherHandSide(x, n)
		if op == 0 {
			continue
		}
		cv := other.ConstValue()
		if cv == nil {
			continue
		}

		originalNMin, originalNMax, changed := nMin, nMax, false
		switch op.Key() {
		case t.KeyXBinaryNotEq:
			if nMin.Cmp(cv) == 0 {
				nMin = add1(nMin)
				changed = true
			} else if nMax.Cmp(cv) == 0 {
				nMax = sub1(nMax)
				changed = true
			}
		case t.KeyXBinaryLessThan:
			if nMax.Cmp(cv) >= 0 {
				nMax = sub1(cv)
				changed = true
			}
		case t.KeyXBinaryLessEq:
			if nMax.Cmp(cv) > 0 {
				nMax = cv
				changed = true
			}
		case t.KeyXBinaryEqEq:
			nMin, nMax = cv, cv
			changed = true
		case t.KeyXBinaryGreaterEq:
			if nMin.Cmp(cv) < 0 {
				nMin = cv
				changed = true
			}
		case t.KeyXBinaryGreaterThan:
			if nMin.Cmp(cv) <= 0 {
				nMin = add1(cv)
				changed = true
			}
		}

		if changed && nMin.Cmp(nMax) > 0 {
			return nil, nil, fmt.Errorf("check: expression %q bounds [%v..%v] inconsistent with fact %q",
				n.String(tm), originalNMin, originalNMax, x.String(tm))
		}
	}

	return nMin, nMax, nil
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

func (q *checker) proveBinaryOp(op t.Key, lhs *a.Expr, rhs *a.Expr) error {
	lhsCV := lhs.ConstValue()
	if lhsCV != nil {
		rMin, rMax, err := q.bcheckExpr(rhs, 0)
		if err != nil {
			return err
		}
		if proveBinaryOpConstValues(op, lhsCV, lhsCV, rMin, rMax) {
			return nil
		}
	}
	rhsCV := rhs.ConstValue()
	if rhsCV != nil {
		lMin, lMax, err := q.bcheckExpr(lhs, 0)
		if err != nil {
			return err
		}
		if proveBinaryOpConstValues(op, lMin, lMax, rhsCV, rhsCV) {
			return nil
		}
	}

	for _, x := range q.facts {
		if !x.LHS().Expr().Eq(lhs) {
			continue
		}
		factOp := x.ID0().Key()
		if factOp == op && x.RHS().Expr().Eq(rhs) {
			return nil
		}

		if factOp == t.KeyXBinaryEqEq && rhsCV != nil {
			if factCV := x.RHS().Expr().ConstValue(); factCV != nil {
				switch op {
				case t.KeyXBinaryNotEq:
					return errFailedOrNil(factCV.Cmp(rhsCV) != 0)
				case t.KeyXBinaryLessThan:
					return errFailedOrNil(factCV.Cmp(rhsCV) < 0)
				case t.KeyXBinaryLessEq:
					return errFailedOrNil(factCV.Cmp(rhsCV) <= 0)
				case t.KeyXBinaryEqEq:
					return errFailedOrNil(factCV.Cmp(rhsCV) == 0)
				case t.KeyXBinaryGreaterEq:
					return errFailedOrNil(factCV.Cmp(rhsCV) >= 0)
				case t.KeyXBinaryGreaterThan:
					return errFailedOrNil(factCV.Cmp(rhsCV) > 0)
				}
			}
		}
	}
	return errFailed
}

func errFailedOrNil(ok bool) error {
	if ok {
		return nil
	}
	return errFailed
}

var errFailed = errors.New("failed")

func binOpReasonError(tm *t.Map, op t.ID, lhs *a.Expr, rhs *a.Expr, err error) error {
	n := a.NewExpr(a.FlagsTypeChecked, op, 0, lhs.Node(), nil, rhs.Node(), nil)
	return fmt.Errorf("cannot prove %q: %v", n.String(tm), err)
}

type reason func(q *checker, n *a.Assert) error

type reasonMap map[t.Key]reason

var reasons = [...]struct {
	s string
	r reason
}{
	{`"a < (b + c): a < c; 0 <= b"`, func(q *checker, n *a.Assert) error {
		op, xa, bc := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessThan {
			return errFailed
		}
		op, xb, xc := parseBinaryOp(bc)
		if op.Key() != t.KeyXBinaryPlus {
			return errFailed
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessThan, xa, xc); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessThan, xa, xc, err)
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, zeroExpr, xb); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, zeroExpr, xb, err)
		}
		return nil
	}},
	{`"(a + b) <= c: a <= (c - b)"`, func(q *checker, n *a.Assert) error {
		op, ab, xc := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessEq {
			return errFailed
		}
		op, xa, xb := parseBinaryOp(ab)
		if op.Key() != t.KeyXBinaryPlus {
			return errFailed
		}
		sub := a.NewExpr(a.FlagsTypeChecked, t.IDXBinaryMinus, 0, xc.Node(), nil, xb.Node(), nil)
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, xa, sub); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, xa, sub, err)
		}
		return nil
	}},
	{`"a < b: a < c; c <= b"`, func(q *checker, n *a.Assert) error {
		xc := argValue(q.tm, n.Args(), "c")
		if xc == nil {
			return errFailed
		}
		op, xa, xb := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessThan {
			return errFailed
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessThan, xa, xc); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessThan, xa, xc, err)
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, xc, xb); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, xc, xb, err)
		}
		return nil
	}},
}
