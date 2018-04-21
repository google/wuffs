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
	"fmt"
	"math/big"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var numTypeBounds = [256][2]*big.Int{
	t.IDI8:   {big.NewInt(-1 << 7), big.NewInt(1<<7 - 1)},
	t.IDI16:  {big.NewInt(-1 << 15), big.NewInt(1<<15 - 1)},
	t.IDI32:  {big.NewInt(-1 << 31), big.NewInt(1<<31 - 1)},
	t.IDI64:  {big.NewInt(-1 << 63), big.NewInt(1<<63 - 1)},
	t.IDU8:   {zero, big.NewInt(0).SetUint64(1<<8 - 1)},
	t.IDU16:  {zero, big.NewInt(0).SetUint64(1<<16 - 1)},
	t.IDU32:  {zero, big.NewInt(0).SetUint64(1<<32 - 1)},
	t.IDU64:  {zero, big.NewInt(0).SetUint64(1<<64 - 1)},
	t.IDBool: {zero, one},
}

var (
	minusOne  = big.NewInt(-1)
	zero      = big.NewInt(+0)
	one       = big.NewInt(+1)
	two       = big.NewInt(+2)
	four      = big.NewInt(+4)
	eight     = big.NewInt(+8)
	sixteen   = big.NewInt(+16)
	thirtyTwo = big.NewInt(+32)
	sixtyFour = big.NewInt(+64)
	ffff      = big.NewInt(0xFFFF)

	maxIntBits = big.NewInt(t.MaxIntBits)

	zeroExpr = a.NewExpr(a.FlagsTypeChecked, 0, 0, t.IDZero, nil, nil, nil, nil)
)

func init() {
	zeroExpr.SetConstValue(zero)
	zeroExpr.SetMType(typeExprIdeal)
}

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

func min(i, j *big.Int) *big.Int {
	if i.Cmp(j) < 0 {
		return i
	}
	return j
}

func max(i, j *big.Int) *big.Int {
	if i.Cmp(j) > 0 {
		return i
	}
	return j
}

// bitMask returns (1<<nBits - 1) as a big integer.
func bitMask(nBits int) *big.Int {
	switch nBits {
	case 0:
		return zero
	case 1:
		return one
	case 8:
		return numTypeBounds[t.IDU8][1]
	case 16:
		return numTypeBounds[t.IDU16][1]
	case 32:
		return numTypeBounds[t.IDU32][1]
	case 64:
		return numTypeBounds[t.IDU64][1]
	}
	z := big.NewInt(0).Lsh(one, uint(nBits))
	return z.Sub(z, one)
}

func invert(tm *t.Map, n *a.Expr) (*a.Expr, error) {
	if !n.MType().IsBool() {
		return nil, fmt.Errorf("check: invert(%q) called on non-bool-typed expression", n.Str(tm))
	}
	if cv := n.ConstValue(); cv != nil {
		return nil, fmt.Errorf("check: invert(%q) called on constant expression", n.Str(tm))
	}
	op, lhs, rhs, args := n.Operator(), n.LHS().Expr(), n.RHS().Expr(), []*a.Node(nil)
	switch op {
	case t.IDXUnaryNot:
		return rhs, nil
	case t.IDXBinaryNotEq:
		op = t.IDXBinaryEqEq
	case t.IDXBinaryLessThan:
		op = t.IDXBinaryGreaterEq
	case t.IDXBinaryLessEq:
		op = t.IDXBinaryGreaterThan
	case t.IDXBinaryEqEq:
		op = t.IDXBinaryNotEq
	case t.IDXBinaryGreaterEq:
		op = t.IDXBinaryLessThan
	case t.IDXBinaryGreaterThan:
		op = t.IDXBinaryLessEq
	case t.IDXBinaryAnd, t.IDXBinaryOr:
		var err error
		lhs, err = invert(tm, lhs)
		if err != nil {
			return nil, err
		}
		rhs, err = invert(tm, rhs)
		if err != nil {
			return nil, err
		}
		if op == t.IDXBinaryAnd {
			op = t.IDXBinaryOr
		} else {
			op = t.IDXBinaryAnd
		}
	case t.IDXAssociativeAnd, t.IDXAssociativeOr:
		args = make([]*a.Node, 0, len(n.Args()))
		for _, a := range n.Args() {
			v, err := invert(tm, a.Expr())
			if err != nil {
				return nil, err
			}
			args = append(args, v.Node())
		}
		if op == t.IDXAssociativeAnd {
			op = t.IDXAssociativeOr
		} else {
			op = t.IDXAssociativeAnd
		}
	default:
		op, lhs, rhs = t.IDXUnaryNot, nil, n
	}
	o := a.NewExpr(n.Node().Raw().Flags(), op, 0, 0, lhs.Node(), nil, rhs.Node(), args)
	o.SetMType(n.MType())
	return o, nil
}

func typeBounds(tm *t.Map, typ *a.TypeExpr) (*big.Int, *big.Int, error) {
	b := [2]*big.Int{}
	if typ.Decorator() == 0 {
		if qid := typ.QID(); qid[0] == t.IDBase && qid[1] < t.ID(len(numTypeBounds)) {
			b = numTypeBounds[qid[1]]
		}
	}
	if b[0] == nil || b[1] == nil {
		return nil, nil, nil
	}
	if o := typ.Min(); o != nil {
		cv := o.ConstValue()
		if cv.Cmp(b[0]) < 0 {
			return nil, nil, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.Str(tm))
		}
		b[0] = cv
	}
	if o := typ.Max(); o != nil {
		cv := o.ConstValue()
		if cv.Cmp(b[1]) > 0 {
			return nil, nil, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.Str(tm))
		}
		b[1] = cv
	}
	return b[0], b[1], nil
}

func bcheckField(tm *t.Map, n *a.Field) error {
	innTyp := n.XType().Innermost()
	nMin, nMax, err := typeBounds(tm, innTyp)
	if err != nil {
		return err
	}
	if nMin == nil {
		if n.DefaultValue() != nil {
			return fmt.Errorf("check: explicit default value %v for field %q of non-numeric innermost type %q",
				n.DefaultValue().ConstValue(), n.Name().Str(tm), innTyp.Str(tm))
		}
		return nil
	}
	dv := zero
	if o := n.DefaultValue(); o != nil {
		dv = o.ConstValue()
	}
	if dv.Cmp(nMin) < 0 || dv.Cmp(nMax) > 0 {
		return fmt.Errorf("check: default value %v is not within bounds [%v..%v] for field %q",
			dv, nMin, nMax, n.Name().Str(tm))
	}
	return nil
}

func (q *checker) bcheckBlock(block []*a.Node) error {
loop:
	for _, o := range block {
		if err := q.bcheckStatement(o); err != nil {
			return err
		}
		switch o.Kind() {
		case a.KJump:
			break loop
		case a.KRet:
			if o.Ret().Keyword() == t.IDReturn {
				break loop
			}
			// o is a yield statement.
			//
			// Drop any facts involving in, out or this.
			if err := q.facts.update(func(x *a.Expr) (*a.Expr, error) {
				if x.Mentions(exprIn) || x.Mentions(exprOut) || x.Mentions(exprThis) {
					return nil, nil
				}
				return x, nil
			}); err != nil {
				return err
			}
			// TODO: drop any facts involving ptr-typed local variables?
		}
	}
	return nil
}

func (q *checker) bcheckStatement(n *a.Node) error {
	q.errFilename, q.errLine = n.Raw().FilenameLine()

	// TODO: be principled about checking for provenNotToSuspend. Should we
	// call optimizeSuspendible only for assignments, for var statements too,
	// or for all statements generally?

	switch n.Kind() {
	case a.KAssert:
		return q.bcheckAssert(n.Assert())

	case a.KAssign:
		n := n.Assign()
		return q.bcheckAssignment(n.LHS(), n.Operator(), n.RHS())

	case a.KExpr:
		n := n.Expr()
		if _, _, err := q.bcheckExpr(n, 0); err != nil {
			return err
		}
		if n.Suspendible() {
			if err := q.optimizeSuspendible(n, 0); err != nil {
				return err
			}
		}
		return nil

	case a.KIf:
		return q.bcheckIf(n.If())

	case a.KJump:
		n := n.Jump()
		skip := t.IDPost
		if n.Keyword() == t.IDBreak {
			skip = t.IDPre
		}
		for _, o := range n.JumpTarget().Asserts() {
			if o.Assert().Keyword() == skip {
				continue
			}
			if err := q.bcheckAssert(o.Assert()); err != nil {
				return err
			}
		}
		q.facts = q.facts[:0]
		return nil

	case a.KRet:
		// TODO.

	case a.KVar:
		return q.bcheckVar(n.Var())

	case a.KIterate:
		n := n.Iterate()
		for _, o := range n.Variables() {
			if err := q.bcheckVar(o.Var()); err != nil {
				return err
			}
		}
		// TODO: this isn't right, as the body is a loop, not an
		// execute-exactly-once block. We should have pre / inv / post
		// conditions, a la bcheckWhile.
		for _, o := range n.Body() {
			if err := q.bcheckStatement(o); err != nil {
				return err
			}
		}
		return nil

	case a.KWhile:
		return q.bcheckWhile(n.While())

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%s) for bcheckStatement", n.Kind())
	}

	return nil
}

func (q *checker) bcheckAssert(n *a.Assert) error {
	// TODO: check, here or elsewhere, that the condition is pure.
	condition := n.Condition()
	for _, x := range q.facts {
		if x.Eq(condition) {
			return nil
		}
	}
	err := errFailed

	if cv := condition.ConstValue(); cv != nil {
		if cv.Cmp(one) == 0 {
			err = nil
		}
	} else if reasonID := n.Reason(); reasonID != 0 {
		if reasonFunc := q.reasonMap[reasonID]; reasonFunc != nil {
			err = reasonFunc(q, n)
		} else {
			err = fmt.Errorf("no such reason %s", reasonID.Str(q.tm))
		}
	} else if condition.Operator().IsBinaryOp() && condition.Operator() != t.IDAs {
		err = q.proveBinaryOp(condition.Operator(), condition.LHS().Expr(), condition.RHS().Expr())
	}

	if err != nil {
		if err == errFailed {
			return fmt.Errorf("check: cannot prove %q", condition.Str(q.tm))
		}
		return fmt.Errorf("check: cannot prove %q: %v", condition.Str(q.tm), err)
	}
	o, err := simplify(q.tm, condition)
	if err != nil {
		return err
	}
	q.facts.appendFact(o)
	return nil
}

func (q *checker) bcheckAssignment(lhs *a.Expr, op t.ID, rhs *a.Expr) error {
	if err := q.bcheckAssignment1(lhs, op, rhs); err != nil {
		return err
	}
	// TODO: check lhs and rhs are pure expressions.
	if op == t.IDEq {
		// Drop any facts involving lhs.
		if err := q.facts.update(func(x *a.Expr) (*a.Expr, error) {
			if x.Mentions(lhs) {
				return nil, nil
			}
			return x, nil
		}); err != nil {
			return err
		}

		if lhs.Pure() && rhs.Pure() && lhs.MType().IsNumType() {
			o := a.NewExpr(a.FlagsTypeChecked, t.IDXBinaryEqEq, 0, 0, lhs.Node(), nil, rhs.Node(), nil)
			o.SetMType(lhs.MType())
			q.facts.appendFact(o)
		}
	} else {
		// Update any facts involving lhs.
		if err := q.facts.update(func(x *a.Expr) (*a.Expr, error) {
			xOp, xLHS, xRHS := parseBinaryOp(x)
			if xOp == 0 || !xLHS.Eq(lhs) {
				if x.Mentions(lhs) {
					return nil, nil
				}
				return x, nil
			}
			if xRHS.Mentions(lhs) {
				return nil, nil
			}
			switch op {
			case t.IDPlusEq, t.IDMinusEq:
				oRHS := a.NewExpr(a.FlagsTypeChecked, op.BinaryForm(), 0, 0, xRHS.Node(), nil, rhs.Node(), nil)
				oRHS.SetMType(xRHS.MType())
				oRHS, err := simplify(q.tm, oRHS)
				if err != nil {
					return nil, err
				}
				o := a.NewExpr(a.FlagsTypeChecked, xOp, 0, 0, xLHS.Node(), nil, oRHS.Node(), nil)
				o.SetMType(x.MType())
				return o, nil
			}
			return nil, nil
		}); err != nil {
			return err
		}
	}

	// TODO: be principled about checking for provenNotToSuspend, and for
	// updating facts like "in.src.available() >= 6" after "in.src.read_u8?()".
	// In general, we need to invalidate any "foo.bar()" facts after a call to
	// an impure function like "foo.meth0!()" or "foo.meth1?()".
	//
	// What's here is somewhat ad hoc. Perhaps we need a call keyword and an
	// explicit "foo = call bar?()" syntax.
	if rhs.Suspendible() {
		if err := q.optimizeSuspendible(rhs, 0); err != nil {
			return err
		}
	}

	return nil
}

func (q *checker) bcheckAssignment1(lhs *a.Expr, op t.ID, rhs *a.Expr) error {
	switch lhs.MType().Decorator() {
	case t.IDPtr:
		// TODO: handle.
		return nil
	case t.IDArray:
		// TODO: handle.
		return nil
		// TODO: t.IDSlice?
	}

	_, _, err := q.bcheckExpr(lhs, 0)
	if err != nil {
		return err
	}
	return q.bcheckAssignment2(lhs, lhs.MType(), op, rhs)
}

func (q *checker) bcheckAssignment2(lhs *a.Expr, lTyp *a.TypeExpr, op t.ID, rhs *a.Expr) error {
	if lhs == nil && op != t.IDEq {
		return fmt.Errorf("check: internal error: missing LHS for op key 0x%02X", op)
	}

	lMin, lMax, err := q.bcheckTypeExpr(lTyp)
	if err != nil {
		return err
	}

	rMin, rMax := (*big.Int)(nil), (*big.Int)(nil)
	if op == t.IDEq {
		if cv := rhs.ConstValue(); cv != nil {
			if (lMin != nil && cv.Cmp(lMin) < 0) || (lMax != nil && cv.Cmp(lMax) > 0) {
				return fmt.Errorf("check: constant %v is not within bounds [%v..%v]", cv, lMin, lMax)
			}
			return nil
		}
		rMin, rMax, err = q.bcheckExpr(rhs, 0)
	} else {
		rMin, rMax, err = q.bcheckExprBinaryOp(op.BinaryForm(), lhs, rhs, 0)
	}
	if err != nil {
		return err
	}

	if (rMin != nil && lMin != nil && rMin.Cmp(lMin) < 0) ||
		(rMax != nil && lMax != nil && rMax.Cmp(lMax) > 0) {

		if op == t.IDEq {
			return fmt.Errorf("check: expression %q bounds [%v..%v] is not within bounds [%v..%v]",
				rhs.Str(q.tm), rMin, rMax, lMin, lMax)
		} else {
			return fmt.Errorf("check: assignment %q bounds [%v..%v] is not within bounds [%v..%v]",
				lhs.Str(q.tm)+" "+op.Str(q.tm)+" "+rhs.Str(q.tm),
				rMin, rMax, lMin, lMax)
		}
	}
	return nil
}

// terminates returns whether a block of statements terminates. In other words,
// whether the block is non-empty and its final statement is a "return",
// "break", "continue" or an "if-else" chain where all branches terminate.
//
// TODO: strengthen this to include "while" statements? For inspiration, the Go
// spec has https://golang.org/ref/spec#Terminating_statements
func terminates(body []*a.Node) bool {
	if len(body) > 0 {
		n := body[len(body)-1]
		switch n.Kind() {
		case a.KIf:
			n := n.If()
			for {
				if !terminates(n.BodyIfTrue()) {
					return false
				}
				bif := n.BodyIfFalse()
				if len(bif) > 0 && !terminates(bif) {
					return false
				}
				n = n.ElseIf()
				if n == nil {
					return len(bif) > 0
				}
			}
		case a.KJump:
			return true
		case a.KRet:
			return n.Ret().Keyword() == t.IDReturn
		}
		return false
	}
	return false
}

func snapshot(facts []*a.Expr) []*a.Expr {
	return append([]*a.Expr(nil), facts...)
}

func (q *checker) unify(branches [][]*a.Expr) error {
	q.facts = q.facts[:0]
	if len(branches) == 0 {
		return nil
	}
	q.facts = append(q.facts[:0], branches[0]...)
	if len(branches) == 1 {
		return nil
	}
	if len(branches) > 10000 {
		return fmt.Errorf("too many if-else branches")
	}

	m := map[string]int{}
	for _, b := range branches {
		for _, f := range b {
			m[f.Str(q.tm)]++
		}
	}

	return q.facts.update(func(n *a.Expr) (*a.Expr, error) {
		if m[n.Str(q.tm)] == len(branches) {
			return n, nil
		}
		return nil, nil
	})
}

func (q *checker) bcheckIf(n *a.If) error {
	branches := [][]*a.Expr(nil)
	for n != nil {
		snap := snapshot(q.facts)
		// Check the if condition.
		//
		// TODO: check that n.Condition() has no side effects.
		if _, _, err := q.bcheckExpr(n.Condition(), 0); err != nil {
			return err
		}

		if cv := n.Condition().ConstValue(); cv != nil {
			return fmt.Errorf("TODO: bcheckIf for \"if true\" or \"if false\"")
		}

		// Check the if-true branch, assuming the if condition.
		q.facts.appendFact(n.Condition())
		if err := q.bcheckBlock(n.BodyIfTrue()); err != nil {
			return err
		}
		if !terminates(n.BodyIfTrue()) {
			branches = append(branches, snapshot(q.facts))
		}

		// Check the if-false branch, assuming the inverted if condition.
		q.facts = append(q.facts[:0], snap...)
		if inverse, err := invert(q.tm, n.Condition()); err != nil {
			return err
		} else {
			q.facts.appendFact(inverse)
		}
		if bif := n.BodyIfFalse(); len(bif) > 0 {
			if err := q.bcheckBlock(bif); err != nil {
				return err
			}
			if !terminates(bif) {
				branches = append(branches, snapshot(q.facts))
			}
			break
		}
		n = n.ElseIf()
		if n == nil {
			branches = append(branches, snapshot(q.facts))
			break
		}
	}
	return q.unify(branches)
}

func (q *checker) bcheckWhile(n *a.While) error {
	// Check the pre and inv conditions on entry.
	for _, o := range n.Asserts() {
		if o.Assert().Keyword() == t.IDPost {
			continue
		}
		if err := q.bcheckAssert(o.Assert()); err != nil {
			return err
		}
	}

	// Check the while condition.
	//
	// TODO: check that n.Condition() has no side effects.
	if _, _, err := q.bcheckExpr(n.Condition(), 0); err != nil {
		return err
	}

	// Check the post conditions on exit, assuming only the pre and inv
	// (invariant) conditions and the inverted while condition.
	//
	// We don't need to check the inv conditions, even though we add them to
	// the facts after the while loop, since we have already proven each inv
	// condition on entry, and below, proven them on each explicit continue and
	// on the implicit continue after the body.
	if cv := n.Condition().ConstValue(); cv != nil && cv.Cmp(one) == 0 {
		// We effectively have a "while true { etc }" loop. There's no need to
		// prove the post conditions here, since we won't ever exit the while
		// loop naturally. We only exit on an explicit break.
	} else {
		q.facts = q.facts[:0]
		for _, o := range n.Asserts() {
			if o.Assert().Keyword() == t.IDPost {
				continue
			}
			q.facts.appendFact(o.Assert().Condition())
		}
		if inverse, err := invert(q.tm, n.Condition()); err != nil {
			return err
		} else {
			q.facts.appendFact(inverse)
		}
		for _, o := range n.Asserts() {
			if o.Assert().Keyword() == t.IDPost {
				if err := q.bcheckAssert(o.Assert()); err != nil {
					return err
				}
			}
		}
	}

	if cv := n.Condition().ConstValue(); cv != nil && cv.Sign() == 0 {
		// We effectively have a "while false { etc }" loop. There's no need to
		// check the body.
	} else {
		// Assume the pre and inv conditions...
		q.facts = q.facts[:0]
		for _, o := range n.Asserts() {
			if o.Assert().Keyword() == t.IDPost {
				continue
			}
			q.facts.appendFact(o.Assert().Condition())
		}
		// ...and the while condition, unless it is the redundant "true".
		if cv == nil {
			q.facts.appendFact(n.Condition())
		}
		// Check the body.
		if err := q.bcheckBlock(n.Body()); err != nil {
			return err
		}
		// Check the pre and inv conditions on the implicit continue after the
		// body.
		if !terminates(n.Body()) {
			for _, o := range n.Asserts() {
				if o.Assert().Keyword() == t.IDPost {
					continue
				}
				if err := q.bcheckAssert(o.Assert()); err != nil {
					return err
				}
			}
		}
	}

	// Assume the inv and post conditions.
	q.facts = q.facts[:0]
	for _, o := range n.Asserts() {
		if o.Assert().Keyword() == t.IDPre {
			continue
		}
		q.facts.appendFact(o.Assert().Condition())
	}
	return nil
}

func (q *checker) bcheckVar(n *a.Var) error {
	if innTyp := n.XType().Innermost(); innTyp.IsRefined() {
		if _, _, err := typeBounds(q.tm, innTyp); err != nil {
			return err
		}
	}
	lhs := a.NewExpr(a.FlagsTypeChecked, 0, 0, n.Name(), nil, nil, nil, nil)
	lhs.SetMType(n.XType())
	rhs := n.Value()
	if rhs == nil {
		// "var x T" has an implicit "= 0".
		//
		// TODO: check that T is an integer type.
		rhs = zeroExpr
	}
	return q.bcheckAssignment(lhs, t.IDEq, rhs)
}

func (q *checker) bcheckExpr(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	if depth > a.MaxExprDepth {
		return nil, nil, fmt.Errorf("check: expression recursion depth too large")
	}
	depth++

	nMin, nMax, err := q.bcheckExpr1(n, depth)
	if err != nil {
		return nil, nil, err
	}
	nMin, nMax, err = q.facts.refine(n, nMin, nMax, q.tm)
	if err != nil {
		return nil, nil, err
	}
	tMin, tMax, err := q.bcheckTypeExpr(n.MType())
	if err != nil {
		return nil, nil, err
	}
	if (nMin != nil && tMin != nil && nMin.Cmp(tMin) < 0) || (nMax != nil && tMax != nil && nMax.Cmp(tMax) > 0) {
		return nil, nil, fmt.Errorf("check: expression %q bounds [%v..%v] is not within bounds [%v..%v]",
			n.Str(q.tm), nMin, nMax, tMin, tMax)
	}
	if err := q.optimizeNonSuspendible(n); err != nil {
		return nil, nil, err
	}
	return nMin, nMax, nil
}

func (q *checker) bcheckExpr1(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	if cv := n.ConstValue(); cv != nil {
		return cv, cv, nil
	}
	switch op := n.Operator(); {
	case op.IsXUnaryOp():
		return q.bcheckExprUnaryOp(n, depth)
	case op.IsXBinaryOp():
		if op == t.IDXBinaryAs {
			return q.bcheckExpr(n.LHS().Expr(), depth)
		}
		return q.bcheckExprBinaryOp(op, n.LHS().Expr(), n.RHS().Expr(), depth)
	case op.IsXAssociativeOp():
		return q.bcheckExprAssociativeOp(n, depth)
	}

	return q.bcheckExprOther(n, depth)
}

func (q *checker) bcheckExprOther(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	switch n.Operator() {
	case 0:
		// Look for named consts.
		//
		// TODO: look up "foo[i]" const expressions.
		//
		// TODO: allow imported consts, "foo.bar", not just "bar"?
		qid := t.QID{0, n.Ident()}
		if c, ok := q.c.consts[qid]; ok {
			if cv := c.Value().ConstValue(); cv != nil {
				return cv, cv, nil
			}
		}

	case t.IDOpenParen, t.IDTry:
		lhs := n.LHS().Expr()
		if _, _, err := q.bcheckExpr(lhs, depth); err != nil {
			return nil, nil, err
		}

		// TODO: delete this hack that only matches "foo.set_literal_width(etc)".
		if isThatMethod(q.tm, n, q.tm.ByName("set_literal_width"), 1) {
			a := n.Args()[0].Arg().Value()
			aMin, aMax, err := q.bcheckExpr(a, depth)
			if err != nil {
				return nil, nil, err
			}
			if aMin.Cmp(big.NewInt(2)) < 0 || aMax.Cmp(big.NewInt(8)) > 0 {
				return nil, nil, fmt.Errorf("check: %q not in range [2..8]", a.Str(q.tm))
			}
			// TODO: should return be break?
			return nil, nil, nil
		}
		// TODO: delete this hack that only matches "foo.decode(etc)".
		if isThatMethod(q.tm, n, q.tm.ByName("decode"), 3) {
			for _, o := range n.Args() {
				a := o.Arg().Value()
				aMin, aMax, err := q.bcheckExpr(a, depth)
				if err != nil {
					return nil, nil, err
				}
				// TODO: check that aMin and aMax is within the function's
				// declared arg bounds.
				_, _ = aMin, aMax
			}
			return nil, nil, nil
		}

		if err := q.bcheckExprCall(n, depth); err != nil {
			return nil, nil, err
		}

		// Special case for a numeric type's low_bits and high_bits methods.
		// The bound on the output is dependent on bound on the input, similar
		// to dependent types, and isn't expressible in Wuffs' function syntax
		// and type system.
		if methodName := lhs.Ident(); methodName == t.IDLowBits || methodName == t.IDHighBits {
			if recv := lhs.MType().Receiver(); recv != nil && recv.IsNumType() {
				return q.bcheckLowHighBitsCall(n, depth)
			}
		}

	case t.IDOpenBracket:
		lhs := n.LHS().Expr()
		if _, _, err := q.bcheckExpr(lhs, depth); err != nil {
			return nil, nil, err
		}
		rhs := n.RHS().Expr()
		if _, _, err := q.bcheckExpr(rhs, depth); err != nil {
			return nil, nil, err
		}

		lengthExpr := (*a.Expr)(nil)
		if lTyp := lhs.MType(); lTyp.IsArrayType() {
			lengthExpr = lTyp.ArrayLength()
		} else {
			lengthExpr = makeSliceLengthExpr(lhs)
		}

		if err := proveReasonRequirement(q, t.IDXBinaryLessEq, zeroExpr, rhs); err != nil {
			return nil, nil, err
		}
		if err := proveReasonRequirement(q, t.IDXBinaryLessThan, rhs, lengthExpr); err != nil {
			return nil, nil, err
		}

	case t.IDColon:
		lhs := n.LHS().Expr()
		mhs := n.MHS().Expr()
		rhs := n.RHS().Expr()
		if mhs == nil && rhs == nil {
			return nil, nil, nil
		}

		lengthExpr := (*a.Expr)(nil)
		if lTyp := lhs.MType(); lTyp.IsArrayType() {
			lengthExpr = lTyp.ArrayLength()
		} else {
			lengthExpr = makeSliceLengthExpr(lhs)
		}

		if mhs == nil {
			mhs = zeroExpr
		}
		if rhs == nil {
			rhs = lengthExpr
		}

		if mhs != zeroExpr {
			if err := proveReasonRequirement(q, t.IDXBinaryLessEq, zeroExpr, mhs); err != nil {
				return nil, nil, err
			}
		}
		if err := proveReasonRequirement(q, t.IDXBinaryLessEq, mhs, rhs); err != nil {
			return nil, nil, err
		}
		if rhs != lengthExpr {
			if err := proveReasonRequirement(q, t.IDXBinaryLessEq, rhs, lengthExpr); err != nil {
				return nil, nil, err
			}
		}
		return nil, nil, nil

	case t.IDDot:
		// TODO: delete this hack that only matches "in".
		if n.LHS().Expr().Ident() == t.IDIn {
			for _, o := range q.astFunc.In().Fields() {
				o := o.Field()
				if o.Name() == n.Ident() {
					return q.bcheckTypeExpr(o.XType())
				}
			}
			lTyp := n.LHS().Expr().MType()
			return nil, nil, fmt.Errorf("check: no field named %q found in struct type %q for expression %q",
				n.Ident().Str(q.tm), lTyp.QID().Str(q.tm), n.Str(q.tm))
		}

		if _, _, err := q.bcheckExpr(n.LHS().Expr(), depth); err != nil {
			return nil, nil, err
		}

	case t.IDError, t.IDStatus, t.IDSuspension:
		// No-op.

	default:
		return nil, nil, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprOther", n.Operator())
	}
	return q.bcheckTypeExpr(n.MType())
}

func (q *checker) bcheckLowHighBitsCall(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	max := (*big.Int)(nil)
	qid := n.LHS().Expr().MType().Receiver().QID()
	switch qid[1] {
	case t.IDU8:
		max = eight
	case t.IDU16:
		max = sixteen
	case t.IDU32:
		max = thirtyTwo
	case t.IDU64:
		max = sixtyFour
	}
	if qid[0] != t.IDBase || max == nil {
		return nil, nil, fmt.Errorf("check: internal error: bad low_bits / high_bits receiver")
	}

	a := n.Args()[0].Arg().Value()
	_, aMax, err := q.bcheckExpr(a, depth)
	if err != nil {
		return nil, nil, err
	}
	if aMax.Cmp(max) > 0 {
		return nil, nil, fmt.Errorf(
			"check: internal error: bad low_bits / high_bits argument, despite type checking")
	}

	return zero, bitMask(int(aMax.Int64())), nil
}

func (q *checker) bcheckExprCall(n *a.Expr, depth uint32) error {
	// TODO: handle func pre/post conditions.
	//
	// TODO: bcheck the receiver, e.g. ptr vs nptr.
	lhs := n.LHS().Expr()
	f, err := q.c.resolveFunc(lhs.MType())
	if err != nil {
		return err
	}
	inFields := f.In().Fields()
	if len(inFields) != len(n.Args()) {
		return fmt.Errorf("check: %q has %d arguments but %d were given",
			lhs.MType().Str(q.tm), len(inFields), len(n.Args()))
	}
	for i, o := range n.Args() {
		if err := q.bcheckAssignment2(nil, inFields[i].Field().XType(), t.IDEq, o.Arg().Value()); err != nil {
			return err
		}
	}
	return nil
}

func makeSliceLengthExpr(slice *a.Expr) *a.Expr {
	x := a.NewExpr(a.FlagsTypeChecked, t.IDDot, 0, t.IDLength, slice.Node(), nil, nil, nil)
	x.SetMType(a.NewTypeExpr(t.IDFunc, 0, t.IDLength, slice.MType().Node(), nil, nil))
	x = a.NewExpr(a.FlagsTypeChecked, t.IDOpenParen, 0, 0, x.Node(), nil, nil, nil)
	x.SetMType(typeExprU64)
	return x
}

func (q *checker) bcheckExprUnaryOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	rMin, rMax, err := q.bcheckExpr(n.RHS().Expr(), depth)
	if err != nil {
		return nil, nil, err
	}

	switch n.Operator() {
	case t.IDXUnaryPlus:
		return rMin, rMax, nil
	case t.IDXUnaryMinus:
		return neg(rMax), neg(rMin), nil
	case t.IDXUnaryNot:
		return zero, one, nil
	case t.IDXUnaryRef, t.IDXUnaryDeref:
		return q.bcheckTypeExpr(n.MType())
	}

	return nil, nil, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprUnaryOp", n.Operator())
}

func (q *checker) bcheckExprBinaryOp(op t.ID, lhs *a.Expr, rhs *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	lMin, lMax, err := q.bcheckExpr(lhs, depth)
	if err != nil {
		return nil, nil, err
	}
	return q.bcheckExprBinaryOp1(op, lhs, lMin, lMax, rhs, depth)
}

func (q *checker) bcheckExprBinaryOp1(op t.ID, lhs *a.Expr, lMin *big.Int, lMax *big.Int, rhs *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	rMin, rMax, err := q.bcheckExpr(rhs, depth)
	if err != nil {
		return nil, nil, err
	}

	switch op {
	case t.IDXBinaryPlus:
		return big.NewInt(0).Add(lMin, rMin), big.NewInt(0).Add(lMax, rMax), nil

	case t.IDXBinaryMinus:
		nMin := big.NewInt(0).Sub(lMin, rMax)
		nMax := big.NewInt(0).Sub(lMax, rMin)
		for _, x := range q.facts {
			xOp, xLHS, xRHS := parseBinaryOp(x)
			if !lhs.Eq(xLHS) || !rhs.Eq(xRHS) {
				continue
			}
			switch xOp {
			case t.IDXBinaryLessThan:
				nMax = min(nMax, minusOne)
			case t.IDXBinaryLessEq:
				nMax = min(nMax, zero)
			case t.IDXBinaryGreaterEq:
				nMin = max(nMin, zero)
			case t.IDXBinaryGreaterThan:
				nMin = max(nMin, one)
			}
		}
		return nMin, nMax, nil

	case t.IDXBinaryStar:
		// TODO: handle multiplication by negative numbers. Note that this
		// might reverse the inequality: if 0 < a < b but c < 0 then a*c > b*c.
		if lMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: multiply op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: multiply op argument %q is possibly negative", rhs.Str(q.tm))
		}
		return big.NewInt(0).Mul(lMin, rMin), big.NewInt(0).Mul(lMax, rMax), nil

	case t.IDXBinarySlash:
		// TODO.

	case t.IDXBinaryShiftL:
		if lMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", rhs.Str(q.tm))
		}
		if rMax.Cmp(ffff) > 0 {
			return nil, nil, fmt.Errorf("check: shift %q out of range", rhs.Str(q.tm))
		}
		return big.NewInt(0).Lsh(lMin, uint(rMin.Uint64())), big.NewInt(0).Lsh(lMax, uint(rMax.Uint64())), nil

	case t.IDXBinaryShiftR:
		if lMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", rhs.Str(q.tm))
		}
		if rMin.Cmp(maxIntBits) >= 0 {
			return zero, zero, nil
		}
		nMax := big.NewInt(0).Rsh(lMax, uint(rMin.Uint64()))
		if rMax.Cmp(maxIntBits) >= 0 {
			return zero, nMax, nil
		}
		return big.NewInt(0).Rsh(lMin, uint(rMax.Uint64())), nMax, nil

	case t.IDXBinaryAmp, t.IDXBinaryPipe, t.IDXBinaryHat:
		// TODO: should type-checking ensure that bitwise ops only apply to
		// *unsigned* integer types?
		if lMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if lMax.Cmp(numTypeBounds[t.IDU64][1]) > 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly too large", lhs.Str(q.tm))
		}
		if rMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly negative", rhs.Str(q.tm))
		}
		if rMax.Cmp(numTypeBounds[t.IDU64][1]) > 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly too large", rhs.Str(q.tm))
		}
		z := (*big.Int)(nil)
		switch op {
		case t.IDXBinaryAmp:
			z = min(lMax, rMax)
		case t.IDXBinaryPipe, t.IDXBinaryHat:
			z = max(lMax, rMax)
		}
		// Return [0, z rounded up to the next power-of-2-minus-1]. This is
		// conservative, but works fine in practice.
		return zero, bitMask(z.BitLen()), nil

	case t.IDXBinaryAmpHat:
		// TODO.

	case t.IDXBinaryPercent:
		if lMin.Sign() < 0 {
			return nil, nil, fmt.Errorf("check: modulus op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rMin.Sign() <= 0 {
			return nil, nil, fmt.Errorf("check: modulus op argument %q is possibly non-positive", rhs.Str(q.tm))
		}
		return zero, big.NewInt(0).Sub(rMax, one), nil

	case t.IDXBinaryNotEq, t.IDXBinaryLessThan, t.IDXBinaryLessEq, t.IDXBinaryEqEq,
		t.IDXBinaryGreaterEq, t.IDXBinaryGreaterThan, t.IDXBinaryAnd, t.IDXBinaryOr:
		return zero, one, nil

	case t.IDXBinaryAs:
		// Unreachable, as this is checked by the caller.

	case t.IDXBinaryTildePlus:
		typ := lhs.MType()
		if typ.IsIdeal() {
			typ = rhs.MType()
		}
		if qid := typ.QID(); qid[0] == t.IDBase {
			b := numTypeBounds[qid[1]]
			return b[0], b[1], nil
		}
	}
	return nil, nil, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprBinaryOp", op)
}

func (q *checker) bcheckExprAssociativeOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	op := n.Operator().AmbiguousForm().BinaryForm()
	if op == 0 {
		return nil, nil, fmt.Errorf(
			"check: unrecognized token (0x%X) for bcheckExprAssociativeOp", n.Operator())
	}
	args := n.Args()
	if len(args) < 1 {
		return nil, nil, fmt.Errorf("check: associative op has no arguments")
	}
	lMin, lMax, err := q.bcheckExpr(args[0].Expr(), depth)
	if err != nil {
		return nil, nil, err
	}
	for i, o := range args {
		if i == 0 {
			continue
		}
		lhs := a.NewExpr(n.Node().Raw().Flags(), n.Operator(), 0, n.Ident(), n.LHS(), n.MHS(), n.RHS(), args[:i])
		lMin, lMax, err = q.bcheckExprBinaryOp1(op, lhs, lMin, lMax, o.Expr(), depth)
		if err != nil {
			return nil, nil, err
		}
	}
	return lMin, lMax, nil
}

func (q *checker) bcheckTypeExpr(typ *a.TypeExpr) (*big.Int, *big.Int, error) {
	if typ.IsIdeal() {
		// TODO: can an ideal type be refined??
		return nil, nil, nil
	}

	switch typ.Decorator() {
	// TODO: case t.IDFunc.
	case t.IDPtr, t.IDArray, t.IDSlice:
		return nil, nil, nil
	}

	// TODO: is the special cases for io_reader and io_writer superfluous with
	// the general purpose code for built-ins below?
	if qid := typ.QID(); qid[0] == t.IDBase {
		switch qid[1] {
		case t.IDIOReader, t.IDIOWriter:
			return nil, nil, nil
		}
	}

	b := [2]*big.Int{}
	if qid := typ.QID(); qid[0] == t.IDBase && qid[1] < t.ID(len(numTypeBounds)) {
		b = numTypeBounds[qid[1]]
	}
	// TODO: should || be && instead (see also func typeBounds)? Is this if
	// code superfluous?
	if b[0] == nil || b[1] == nil {
		return nil, nil, nil
	}
	if typ.IsRefined() {
		if x := typ.Min(); x != nil {
			if cv := x.ConstValue(); cv == nil {
				return nil, nil, fmt.Errorf("check: internal error: refinement has no const-value")
			} else if b[0].Cmp(cv) < 0 {
				b[0] = cv
			}
		}
		if x := typ.Max(); x != nil {
			if cv := x.ConstValue(); cv == nil {
				return nil, nil, fmt.Errorf("check: internal error: refinement has no const-value")
			} else if b[1].Cmp(cv) > 0 {
				b[1] = cv
			}
		}
	}
	return b[0], b[1], nil
}
