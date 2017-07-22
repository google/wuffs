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
		for _, a := range fact.Args() {
			z.appendFact(a.Expr())
		}
		return
	}

	*z = append(*z, fact)
}

// update applies f to each fact, replacing the slice element with the result
// of the function call. The slice is then compacted to remove all nils.
func (z *facts) update(f func(*a.Expr) (*a.Expr, error)) error {
	i := 0
	for _, x := range *z {
		x, err := f(x)
		if err != nil {
			return err
		}
		if x != nil {
			(*z)[i] = x
			i++
		}
	}
	for j := i; j < len(*z); j++ {
		(*z)[j] = nil
	}
	*z = (*z)[:i]
	return nil
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
func simplify(tm *t.Map, n *a.Expr) (*a.Expr, error) {
	// TODO: be rigorous about this, not ad hoc.
	op, lhs, rhs := parseBinaryOp(n)
	if lhs != nil && rhs != nil {
		if lcv, rcv := lhs.ConstValue(), rhs.ConstValue(); lcv != nil && rcv != nil {
			ncv, err := evalConstValueBinaryOp(tm, n, lcv, rcv)
			if err != nil {
				return nil, err
			}
			id, err := tm.Insert(ncv.String())
			if err != nil {
				return nil, err
			}
			o := a.NewExpr(a.FlagsTypeChecked, 0, id, nil, nil, nil, nil)
			o.SetConstValue(ncv)
			o.SetMType(typeExprIdeal)
			return o, nil
		}
	}

	switch op.Key() {
	case t.KeyXBinaryPlus:
		// TODO: more constant folding, so ((x + 1) + 1) becomes (x + 2).

	case t.KeyXBinaryMinus:
		if lhs.Eq(rhs) {
			return zeroExpr, nil
		}
		if lOp, lLHS, lRHS := parseBinaryOp(lhs); lOp.Key() == t.KeyXBinaryPlus {
			if lLHS.Eq(rhs) {
				return lRHS, nil
			}
			if lRHS.Eq(rhs) {
				return lLHS, nil
			}
		}

	case t.KeyXBinaryNotEq, t.KeyXBinaryLessThan, t.KeyXBinaryLessEq,
		t.KeyXBinaryEqEq, t.KeyXBinaryGreaterEq, t.KeyXBinaryGreaterThan:

		l, err := simplify(tm, lhs)
		if err != nil {
			return nil, err
		}
		r, err := simplify(tm, rhs)
		if err != nil {
			return nil, err
		}
		if l != lhs || r != rhs {
			o := a.NewExpr(a.FlagsTypeChecked, op, 0, l.Node(), nil, r.Node(), nil)
			o.SetConstValue(n.ConstValue())
			o.SetMType(n.MType())
			return o, nil
		}
	}
	return n, nil
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
	lcv := lhs.ConstValue()
	if lcv != nil {
		rMin, rMax, err := q.bcheckExpr(rhs, 0)
		if err != nil {
			return err
		}
		if proveBinaryOpConstValues(op, lcv, lcv, rMin, rMax) {
			return nil
		}
	}
	rcv := rhs.ConstValue()
	if rcv != nil {
		lMin, lMax, err := q.bcheckExpr(lhs, 0)
		if err != nil {
			return err
		}
		if proveBinaryOpConstValues(op, lMin, lMax, rcv, rcv) {
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

		if factOp == t.KeyXBinaryEqEq && rcv != nil {
			if factCV := x.RHS().Expr().ConstValue(); factCV != nil {
				switch op {
				case t.KeyXBinaryNotEq:
					return errFailedOrNil(factCV.Cmp(rcv) != 0)
				case t.KeyXBinaryLessThan:
					return errFailedOrNil(factCV.Cmp(rcv) < 0)
				case t.KeyXBinaryLessEq:
					return errFailedOrNil(factCV.Cmp(rcv) <= 0)
				case t.KeyXBinaryEqEq:
					return errFailedOrNil(factCV.Cmp(rcv) == 0)
				case t.KeyXBinaryGreaterEq:
					return errFailedOrNil(factCV.Cmp(rcv) >= 0)
				case t.KeyXBinaryGreaterThan:
					return errFailedOrNil(factCV.Cmp(rcv) > 0)
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

func proveReasonRequirement(q *checker, op t.ID, lhs *a.Expr, rhs *a.Expr) error {
	if err := q.proveBinaryOp(op.Key(), lhs, rhs); err != nil {
		n := a.NewExpr(a.FlagsTypeChecked, op, 0, lhs.Node(), nil, rhs.Node(), nil)
		return fmt.Errorf("cannot prove %q: %v", n.String(q.tm), err)
	}
	return nil
}
