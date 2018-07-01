// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package check

import (
	"errors"
	"fmt"
	"math/big"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

// otherHandSide returns the operator and other hand side when n is an
// binary-op expression like "thisHS == thatHS" or "thatHS < thisHS" (which is
// equivalent to "thisHS > thatHS"). If not, it returns (0, nil).
func otherHandSide(n *a.Expr, thisHS *a.Expr) (op t.ID, thatHS *a.Expr) {
	op = n.Operator()

	reverseOp := t.ID(0)
	switch op {
	case t.IDXBinaryNotEq:
		reverseOp = t.IDXBinaryNotEq
	case t.IDXBinaryLessThan:
		reverseOp = t.IDXBinaryGreaterThan
	case t.IDXBinaryLessEq:
		reverseOp = t.IDXBinaryGreaterEq
	case t.IDXBinaryEqEq:
		reverseOp = t.IDXBinaryEqEq
	case t.IDXBinaryGreaterEq:
		reverseOp = t.IDXBinaryLessEq
	case t.IDXBinaryGreaterThan:
		reverseOp = t.IDXBinaryLessThan
	}

	if reverseOp != 0 {
		if thisHS.Eq(n.LHS().AsExpr()) {
			return op, n.RHS().AsExpr()
		}
		if thisHS.Eq(n.RHS().AsExpr()) {
			return reverseOp, n.LHS().AsExpr()
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

	switch fact.Operator() {
	case 0:
		for _, x := range *z {
			if op, other := otherHandSide(x, fact); op == t.IDXBinaryEqEq {
				z.appendFact(other)
			}
		}
	case t.IDXBinaryAnd:
		z.appendFact(fact.LHS().AsExpr())
		z.appendFact(fact.RHS().AsExpr())
		return
	case t.IDXAssociativeAnd:
		for _, a := range fact.Args() {
			z.appendFact(a.AsExpr())
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
		switch op {
		case t.IDXBinaryNotEq:
			if nMin.Cmp(cv) == 0 {
				nMin = add1(nMin)
				changed = true
			} else if nMax.Cmp(cv) == 0 {
				nMax = sub1(nMax)
				changed = true
			}
		case t.IDXBinaryLessThan:
			if nMax.Cmp(cv) >= 0 {
				nMax = sub1(cv)
				changed = true
			}
		case t.IDXBinaryLessEq:
			if nMax.Cmp(cv) > 0 {
				nMax = cv
				changed = true
			}
		case t.IDXBinaryEqEq:
			nMin, nMax = cv, cv
			changed = true
		case t.IDXBinaryGreaterEq:
			if nMin.Cmp(cv) < 0 {
				nMin = cv
				changed = true
			}
		case t.IDXBinaryGreaterThan:
			if nMin.Cmp(cv) <= 0 {
				nMin = add1(cv)
				changed = true
			}
		}

		if changed && nMin.Cmp(nMax) > 0 {
			return nil, nil, fmt.Errorf("check: expression %q bounds [%v..%v] inconsistent with fact %q",
				n.Str(tm), originalNMin, originalNMax, x.Str(tm))
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
			o := a.NewExpr(0, 0, 0, id, nil, nil, nil, nil)
			o.SetConstValue(ncv)
			o.SetMType(typeExprIdeal)
			return o, nil
		}
	}

	switch op {
	case t.IDXBinaryPlus:
		// TODO: more constant folding, so ((x + 1) + 1) becomes (x + 2).

	case t.IDXBinaryMinus:
		if lhs.Eq(rhs) {
			return zeroExpr, nil
		}
		if lOp, lLHS, lRHS := parseBinaryOp(lhs); lOp == t.IDXBinaryPlus {
			if lLHS.Eq(rhs) {
				return lRHS, nil
			}
			if lRHS.Eq(rhs) {
				return lLHS, nil
			}
		}

	case t.IDXBinaryNotEq, t.IDXBinaryLessThan, t.IDXBinaryLessEq,
		t.IDXBinaryEqEq, t.IDXBinaryGreaterEq, t.IDXBinaryGreaterThan:

		l, err := simplify(tm, lhs)
		if err != nil {
			return nil, err
		}
		r, err := simplify(tm, rhs)
		if err != nil {
			return nil, err
		}
		if l != lhs || r != rhs {
			o := a.NewExpr(0, op, 0, 0, l.AsNode(), nil, r.AsNode(), nil)
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
			if a.AsArg().Name() == x {
				return a.AsArg().Value()
			}
		}
	}
	return nil
}

// parseBinaryOp parses n as "lhs op rhs".
func parseBinaryOp(n *a.Expr) (op t.ID, lhs *a.Expr, rhs *a.Expr) {
	if !n.Operator().IsBinaryOp() {
		return 0, nil, nil
	}
	op = n.Operator()
	if op == t.IDAs {
		return 0, nil, nil
	}
	return op, n.LHS().AsExpr(), n.RHS().AsExpr()
}

func proveBinaryOpConstValues(op t.ID, lMin *big.Int, lMax *big.Int, rMin *big.Int, rMax *big.Int) (ok bool) {
	switch op {
	case t.IDXBinaryNotEq:
		return lMax.Cmp(rMin) < 0 || lMin.Cmp(rMax) > 0
	case t.IDXBinaryLessThan:
		return lMax.Cmp(rMin) < 0
	case t.IDXBinaryLessEq:
		return lMax.Cmp(rMin) <= 0
	case t.IDXBinaryEqEq:
		return lMin.Cmp(rMax) == 0 && lMax.Cmp(rMin) == 0
	case t.IDXBinaryGreaterEq:
		return lMin.Cmp(rMax) >= 0
	case t.IDXBinaryGreaterThan:
		return lMin.Cmp(rMax) > 0
	}
	return false
}

func (q *checker) proveBinaryOp(op t.ID, lhs *a.Expr, rhs *a.Expr) error {
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
		if !x.LHS().AsExpr().Eq(lhs) {
			continue
		}
		factOp := x.Operator()
		if opImpliesOp(factOp, op) && x.RHS().AsExpr().Eq(rhs) {
			return nil
		}

		if factOp == t.IDXBinaryEqEq && rcv != nil {
			if factCV := x.RHS().AsExpr().ConstValue(); factCV != nil {
				switch op {
				case t.IDXBinaryNotEq:
					return errFailedOrNil(factCV.Cmp(rcv) != 0)
				case t.IDXBinaryLessThan:
					return errFailedOrNil(factCV.Cmp(rcv) < 0)
				case t.IDXBinaryLessEq:
					return errFailedOrNil(factCV.Cmp(rcv) <= 0)
				case t.IDXBinaryEqEq:
					return errFailedOrNil(factCV.Cmp(rcv) == 0)
				case t.IDXBinaryGreaterEq:
					return errFailedOrNil(factCV.Cmp(rcv) >= 0)
				case t.IDXBinaryGreaterThan:
					return errFailedOrNil(factCV.Cmp(rcv) > 0)
				}
			}
		}
	}
	return errFailed
}

// opImpliesOp returns whether the first op implies the second. For example,
// knowing "x < y" implies that "x != y" and "x <= y".
func opImpliesOp(op0 t.ID, op1 t.ID) bool {
	if op0 == op1 {
		return true
	}
	switch op0 {
	case t.IDXBinaryLessThan:
		return op1 == t.IDXBinaryNotEq || op1 == t.IDXBinaryLessEq
	case t.IDXBinaryGreaterThan:
		return op1 == t.IDXBinaryNotEq || op1 == t.IDXBinaryGreaterEq
	}
	return false
}

func errFailedOrNil(ok bool) error {
	if ok {
		return nil
	}
	return errFailed
}

var errFailed = errors.New("failed")

func proveReasonRequirement(q *checker, op t.ID, lhs *a.Expr, rhs *a.Expr) error {
	if !op.IsXBinaryOp() {
		return fmt.Errorf(
			"check: internal error: proveReasonRequirement token (0x%02X) is not an XBinaryOp", op)
	}
	if err := q.proveBinaryOp(op, lhs, rhs); err != nil {
		n := a.NewExpr(0, op, 0, 0, lhs.AsNode(), nil, rhs.AsNode(), nil)
		return fmt.Errorf("cannot prove %q: %v", n.Str(q.tm), err)
	}
	return nil
}
