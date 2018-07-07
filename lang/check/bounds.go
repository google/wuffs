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

var numTypeBounds = [...]a.Bounds{
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
	minusOne       = big.NewInt(-1)
	zero           = big.NewInt(+0)
	one            = big.NewInt(+1)
	two            = big.NewInt(+2)
	three          = big.NewInt(+3)
	four           = big.NewInt(+4)
	five           = big.NewInt(+5)
	six            = big.NewInt(+6)
	seven          = big.NewInt(+7)
	eight          = big.NewInt(+8)
	sixteen        = big.NewInt(+16)
	thirtyTwo      = big.NewInt(+32)
	sixtyFour      = big.NewInt(+64)
	oneTwentyEight = big.NewInt(+128)
	ffff           = big.NewInt(0xFFFF)

	minIdeal = big.NewInt(0).Lsh(minusOne, 1000)
	maxIdeal = big.NewInt(0).Lsh(one, 1000)

	maxIntBits = big.NewInt(t.MaxIntBits)

	zeroExpr = a.NewExpr(0, 0, 0, t.ID0, nil, nil, nil, nil)
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
	op, lhs, rhs, args := n.Operator(), n.LHS().AsExpr(), n.RHS().AsExpr(), []*a.Node(nil)
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
			v, err := invert(tm, a.AsExpr())
			if err != nil {
				return nil, err
			}
			args = append(args, v.AsNode())
		}
		if op == t.IDXAssociativeAnd {
			op = t.IDXAssociativeOr
		} else {
			op = t.IDXAssociativeAnd
		}
	default:
		op, lhs, rhs = t.IDXUnaryNot, nil, n
	}
	o := a.NewExpr(n.AsNode().AsRaw().Flags(), op, 0, 0, lhs.AsNode(), nil, rhs.AsNode(), args)
	o.SetMType(n.MType())
	return o, nil
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
			if o.AsRet().Keyword() == t.IDReturn {
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
	q.errFilename, q.errLine = n.AsRaw().FilenameLine()

	// TODO: be principled about checking for provenNotToSuspend. Should we
	// call optimizeSuspendible only for assignments, for var statements too,
	// or for all statements generally?

	switch n.Kind() {
	case a.KAssert:
		return q.bcheckAssert(n.AsAssert())

	case a.KAssign:
		n := n.AsAssign()
		return q.bcheckAssignment(n.LHS(), n.Operator(), n.RHS())

	case a.KExpr:
		n := n.AsExpr()
		if _, err := q.bcheckExpr(n, 0); err != nil {
			return err
		}
		if n.Suspendible() {
			if err := q.optimizeSuspendible(n, 0); err != nil {
				return err
			}
		}
		return nil

	case a.KIOBind:
		n := n.AsIOBind()
		if err := q.bcheckBlock(n.Body()); err != nil {
			return err
		}
		// TODO: invalidate any facts regarding the io_bind expressions.
		return nil

	case a.KIf:
		return q.bcheckIf(n.AsIf())

	case a.KIterate:
		n := n.AsIterate()
		for _, o := range n.Variables() {
			if err := q.bcheckVar(o.AsVar(), true); err != nil {
				return err
			}
		}
		// TODO: this isn't right, as the body is a loop, not an
		// execute-exactly-once block. We should have pre / inv / post
		// conditions, a la bcheckWhile.

		variables := n.Variables()
		for ; n != nil; n = n.ElseIterate() {
			q.facts = q.facts[:0]
			for _, o := range variables {
				v := o.AsVar()
				q.facts = append(q.facts, makeSliceLengthEqEq(v.Name(), v.XType(), n.Length()))
			}
			for _, o := range n.Body() {
				if err := q.bcheckStatement(o); err != nil {
					return err
				}
			}
		}

		q.facts = q.facts[:0]
		return nil

	case a.KJump:
		n := n.AsJump()
		skip := t.IDPost
		if n.Keyword() == t.IDBreak {
			skip = t.IDPre
		}
		for _, o := range n.JumpTarget().Asserts() {
			if o.AsAssert().Keyword() == skip {
				continue
			}
			if err := q.bcheckAssert(o.AsAssert()); err != nil {
				return err
			}
		}
		q.facts = q.facts[:0]
		return nil

	case a.KRet:
		// TODO.

	case a.KVar:
		return q.bcheckVar(n.AsVar(), false)

	case a.KWhile:
		return q.bcheckWhile(n.AsWhile())

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
		err = q.proveBinaryOp(condition.Operator(),
			condition.LHS().AsExpr(), condition.RHS().AsExpr())
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
			o := a.NewExpr(0, t.IDXBinaryEqEq, 0, 0, lhs.AsNode(), nil, rhs.AsNode(), nil)
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
				oRHS := a.NewExpr(0, op.BinaryForm(), 0, 0, xRHS.AsNode(), nil, rhs.AsNode(), nil)
				oRHS.SetMType(xRHS.MType())
				oRHS, err := simplify(q.tm, oRHS)
				if err != nil {
					return nil, err
				}
				o := a.NewExpr(0, xOp, 0, 0, xLHS.AsNode(), nil, oRHS.AsNode(), nil)
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
		// TODO: t.IDSlice, t.IDTable?
	}

	if _, err := q.bcheckExpr(lhs, 0); err != nil {
		return err
	}
	return q.bcheckAssignment2(lhs, lhs.MType(), op, rhs)
}

func (q *checker) bcheckAssignment2(lhs *a.Expr, lTyp *a.TypeExpr, op t.ID, rhs *a.Expr) error {
	if lhs == nil && op != t.IDEq {
		return fmt.Errorf("check: internal error: missing LHS for op key 0x%02X", op)
	}

	lb, err := typeBounds(q.tm, lTyp)
	if err != nil {
		return err
	}

	rb := a.Bounds{}
	if op == t.IDEq {
		rb, err = q.bcheckExpr(rhs, 0)
	} else {
		rb, err = q.bcheckExprBinaryOp(op.BinaryForm(), lhs, rhs, 0)
	}
	if err != nil {
		return err
	}

	if (rb[0].Cmp(lb[0]) < 0) || (rb[1].Cmp(lb[1]) > 0) {
		if op == t.IDEq {
			return fmt.Errorf("check: expression %q bounds %v is not within bounds %v",
				rhs.Str(q.tm), rb, lb)
		} else {
			return fmt.Errorf("check: assignment %q bounds %v is not within bounds %v",
				lhs.Str(q.tm)+" "+op.Str(q.tm)+" "+rhs.Str(q.tm), rb, lb)
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
			n := n.AsIf()
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
			return n.AsRet().Keyword() == t.IDReturn
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
		if _, err := q.bcheckExpr(n.Condition(), 0); err != nil {
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
		if o.AsAssert().Keyword() == t.IDPost {
			continue
		}
		if err := q.bcheckAssert(o.AsAssert()); err != nil {
			return err
		}
	}

	// Check the while condition.
	//
	// TODO: check that n.Condition() has no side effects.
	if _, err := q.bcheckExpr(n.Condition(), 0); err != nil {
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
			if o.AsAssert().Keyword() == t.IDPost {
				continue
			}
			q.facts.appendFact(o.AsAssert().Condition())
		}
		if inverse, err := invert(q.tm, n.Condition()); err != nil {
			return err
		} else {
			q.facts.appendFact(inverse)
		}
		for _, o := range n.Asserts() {
			if o.AsAssert().Keyword() == t.IDPost {
				if err := q.bcheckAssert(o.AsAssert()); err != nil {
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
			if o.AsAssert().Keyword() == t.IDPost {
				continue
			}
			q.facts.appendFact(o.AsAssert().Condition())
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
				if o.AsAssert().Keyword() == t.IDPost {
					continue
				}
				if err := q.bcheckAssert(o.AsAssert()); err != nil {
					return err
				}
			}
		}
	}

	// Assume the inv and post conditions.
	q.facts = q.facts[:0]
	for _, o := range n.Asserts() {
		if o.AsAssert().Keyword() == t.IDPre {
			continue
		}
		q.facts.appendFact(o.AsAssert().Condition())
	}
	return nil
}

func (q *checker) bcheckVar(n *a.Var, iterateVariable bool) error {
	if innTyp := n.XType().Innermost(); innTyp.IsRefined() {
		if _, err := typeBounds(q.tm, innTyp); err != nil {
			return err
		}
	}
	if iterateVariable {
		// TODO.
		return nil
	}

	lhs := a.NewExpr(0, 0, 0, n.Name(), nil, nil, nil, nil)
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

func (q *checker) bcheckExpr(n *a.Expr, depth uint32) (a.Bounds, error) {
	if depth > a.MaxExprDepth {
		return a.Bounds{}, fmt.Errorf("check: expression recursion depth too large")
	}
	depth++

	// TODO: check that n.MBounds() is {nil, nil}? Likewise for tcheck?

	nb, err := q.bcheckExpr1(n, depth)
	if err != nil {
		return a.Bounds{}, err
	}
	nb, err = q.facts.refine(n, nb, q.tm)
	if err != nil {
		return a.Bounds{}, err
	}
	tb, err := typeBounds(q.tm, n.MType())
	if err != nil {
		return a.Bounds{}, err
	}

	if (nb[0].Cmp(tb[0]) < 0) || (nb[1].Cmp(tb[1]) > 0) {
		return a.Bounds{}, fmt.Errorf("check: expression %q bounds %v is not within bounds %v",
			n.Str(q.tm), nb, tb)
	}
	if err := q.optimizeNonSuspendible(n); err != nil {
		return a.Bounds{}, err
	}
	// TODO: call n.SetMBounds.
	return nb, nil
}

func (q *checker) bcheckExpr1(n *a.Expr, depth uint32) (a.Bounds, error) {
	if cv := n.ConstValue(); cv != nil {
		return a.Bounds{cv, cv}, nil
	}
	switch op := n.Operator(); {
	case op.IsXUnaryOp():
		return q.bcheckExprUnaryOp(n, depth)
	case op.IsXBinaryOp():
		if op == t.IDXBinaryAs {
			return q.bcheckExpr(n.LHS().AsExpr(), depth)
		}
		return q.bcheckExprBinaryOp(op, n.LHS().AsExpr(), n.RHS().AsExpr(), depth)
	case op.IsXAssociativeOp():
		return q.bcheckExprAssociativeOp(n, depth)
	}

	return q.bcheckExprOther(n, depth)
}

func (q *checker) bcheckExprOther(n *a.Expr, depth uint32) (a.Bounds, error) {
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
				return a.Bounds{cv, cv}, nil
			}
		}

	case t.IDOpenParen, t.IDTry:
		lhs := n.LHS().AsExpr()
		if _, err := q.bcheckExpr(lhs, depth); err != nil {
			return a.Bounds{}, err
		}
		if err := q.bcheckExprCall(n, depth); err != nil {
			return a.Bounds{}, err
		}
		if nb, err := q.bcheckExprCallSpecialCases(n, depth); err == nil {
			return nb, nil
		} else if err != errNotASpecialCase {
			return a.Bounds{}, err
		}

	case t.IDOpenBracket:
		lhs := n.LHS().AsExpr()
		if _, err := q.bcheckExpr(lhs, depth); err != nil {
			return a.Bounds{}, err
		}
		rhs := n.RHS().AsExpr()
		if _, err := q.bcheckExpr(rhs, depth); err != nil {
			return a.Bounds{}, err
		}

		lengthExpr := (*a.Expr)(nil)
		if lTyp := lhs.MType(); lTyp.IsArrayType() {
			lengthExpr = lTyp.ArrayLength()
		} else {
			lengthExpr = makeSliceLength(lhs)
		}

		if err := proveReasonRequirement(q, t.IDXBinaryLessEq, zeroExpr, rhs); err != nil {
			return a.Bounds{}, err
		}
		if err := proveReasonRequirement(q, t.IDXBinaryLessThan, rhs, lengthExpr); err != nil {
			return a.Bounds{}, err
		}

	case t.IDColon:
		lhs := n.LHS().AsExpr()
		if _, err := q.bcheckExpr(lhs, depth); err != nil {
			return a.Bounds{}, err
		}
		mhs := n.MHS().AsExpr()
		if mhs != nil {
			if _, err := q.bcheckExpr(mhs, depth); err != nil {
				return a.Bounds{}, err
			}
		}
		rhs := n.RHS().AsExpr()
		if rhs != nil {
			if _, err := q.bcheckExpr(rhs, depth); err != nil {
				return a.Bounds{}, err
			}
		}

		if mhs == nil && rhs == nil {
			return a.Bounds{zero, zero}, nil
		}

		lengthExpr := (*a.Expr)(nil)
		if lTyp := lhs.MType(); lTyp.IsArrayType() {
			lengthExpr = lTyp.ArrayLength()
		} else {
			lengthExpr = makeSliceLength(lhs)
		}

		if mhs == nil {
			mhs = zeroExpr
		}
		if rhs == nil {
			rhs = lengthExpr
		}

		if mhs != zeroExpr {
			if err := proveReasonRequirement(q, t.IDXBinaryLessEq, zeroExpr, mhs); err != nil {
				return a.Bounds{}, err
			}
		}
		if err := proveReasonRequirement(q, t.IDXBinaryLessEq, mhs, rhs); err != nil {
			return a.Bounds{}, err
		}
		if rhs != lengthExpr {
			if err := proveReasonRequirement(q, t.IDXBinaryLessEq, rhs, lengthExpr); err != nil {
				return a.Bounds{}, err
			}
		}
		return a.Bounds{zero, zero}, nil

	case t.IDDot:
		// TODO: delete this hack that only matches "in".
		if n.LHS().AsExpr().Ident() == t.IDIn {
			for _, o := range q.astFunc.In().Fields() {
				o := o.AsField()
				if o.Name() == n.Ident() {
					return typeBounds(q.tm, o.XType())
				}
			}
			lTyp := n.LHS().AsExpr().MType()
			return a.Bounds{}, fmt.Errorf("check: no field named %q found in struct type %q for expression %q",
				n.Ident().Str(q.tm), lTyp.QID().Str(q.tm), n.Str(q.tm))
		}

		if _, err := q.bcheckExpr(n.LHS().AsExpr(), depth); err != nil {
			return a.Bounds{}, err
		}

	case t.IDError, t.IDStatus, t.IDSuspension:
		// No-op.

	default:
		return a.Bounds{}, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprOther", n.Operator())
	}
	return typeBounds(q.tm, n.MType())
}

func (q *checker) bcheckExprCall(n *a.Expr, depth uint32) error {
	// TODO: handle func pre/post conditions.
	//
	// TODO: bcheck the receiver, e.g. ptr vs nptr.
	lhs := n.LHS().AsExpr()
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
		if err := q.bcheckAssignment2(nil, inFields[i].AsField().XType(), t.IDEq, o.AsArg().Value()); err != nil {
			return err
		}
	}
	return nil
}

var errNotASpecialCase = errors.New("not a special case")

func (q *checker) bcheckExprCallSpecialCases(n *a.Expr, depth uint32) (a.Bounds, error) {
	lhs := n.LHS().AsExpr()
	recv := lhs.LHS().AsExpr()
	method := lhs.Ident()

	if recvTyp := recv.MType(); recvTyp == nil {
		return a.Bounds{}, errNotASpecialCase

	} else if recvTyp.IsNumType() {
		// For a numeric type's low_bits, etc. methods. The bound on the output
		// is dependent on bound on the input, similar to dependent types, and
		// isn't expressible in Wuffs' function syntax and type system.
		switch method {
		case t.IDLowBits, t.IDHighBits:
			ab, err := q.bcheckExpr(n.Args()[0].AsArg().Value(), depth)
			if err != nil {
				return a.Bounds{}, err
			}
			return a.Bounds{
				zero,
				bitMask(int(ab[1].Int64())),
			}, nil

		case t.IDMin, t.IDMax:
			// TODO: lhs has already been bcheck'ed. There should be no
			// need to bcheck lhs.LHS().Expr() twice.
			lb, err := q.bcheckExpr(lhs.LHS().AsExpr(), depth)
			if err != nil {
				return a.Bounds{}, err
			}
			ab, err := q.bcheckExpr(n.Args()[0].AsArg().Value(), depth)
			if err != nil {
				return a.Bounds{}, err
			}
			if method == t.IDMin {
				return a.Bounds{
					min(lb[0], ab[0]),
					min(lb[1], ab[1]),
				}, nil
			} else {
				return a.Bounds{
					max(lb[0], ab[0]),
					max(lb[1], ab[1]),
				}, nil
			}
		}

	} else if recvTyp.IsIOType() {
		advance, update := (*big.Int)(nil), false

		if method == t.IDSkip32Fast {
			args := n.Args()
			if len(args) != 2 {
				return a.Bounds{}, fmt.Errorf("check: internal error: bad skip32_fast arguments")
			}
			actual := args[0].AsArg().Value()
			worstCase := args[1].AsArg().Value()
			if err := q.proveBinaryOp(t.IDXBinaryLessEq, actual, worstCase); err == errFailed {
				return a.Bounds{}, fmt.Errorf("check: could not prove skip32_fast precondition: %s <= %s",
					actual.Str(q.tm), worstCase.Str(q.tm))
			} else if err != nil {
				return a.Bounds{}, err
			}
			advance, update = worstCase.ConstValue(), true
			if advance == nil {
				return a.Bounds{}, fmt.Errorf("check: skip32_fast worst_case is not a constant value")
			}

		} else if method >= t.IDPeekU8 {
			if m := method - t.IDPeekU8; m < t.ID(len(ioMethodAdvances)) {
				advance, update = ioMethodAdvances[m], false
			}
		}

		if advance != nil {
			if ok, err := q.optimizeIOMethodAdvance(recv, advance, update); err != nil {
				return a.Bounds{}, err
			} else if !ok {
				return a.Bounds{}, fmt.Errorf("check: could not prove %s precondition: %s.available() >= %v",
					method.Str(q.tm), recv.Str(q.tm), advance)
			}
		}
	}

	return a.Bounds{}, errNotASpecialCase
}

var ioMethodAdvances = [...]*big.Int{
	t.IDPeekU8 - t.IDPeekU8:    one,
	t.IDPeekU16BE - t.IDPeekU8: two,
	t.IDPeekU16LE - t.IDPeekU8: two,
	t.IDPeekU24BE - t.IDPeekU8: three,
	t.IDPeekU24LE - t.IDPeekU8: three,
	t.IDPeekU32BE - t.IDPeekU8: four,
	t.IDPeekU32LE - t.IDPeekU8: four,
	t.IDPeekU40BE - t.IDPeekU8: five,
	t.IDPeekU40LE - t.IDPeekU8: five,
	t.IDPeekU48BE - t.IDPeekU8: six,
	t.IDPeekU48LE - t.IDPeekU8: six,
	t.IDPeekU56BE - t.IDPeekU8: seven,
	t.IDPeekU56LE - t.IDPeekU8: seven,
	t.IDPeekU64BE - t.IDPeekU8: eight,
	t.IDPeekU64LE - t.IDPeekU8: eight,
}

// makeSliceLength returns "x.length()".
func makeSliceLength(slice *a.Expr) *a.Expr {
	x := a.NewExpr(0, t.IDDot, 0, t.IDLength, slice.AsNode(), nil, nil, nil)
	x.SetMType(a.NewTypeExpr(t.IDFunc, 0, t.IDLength, slice.MType().AsNode(), nil, nil))
	x = a.NewExpr(0, t.IDOpenParen, 0, 0, x.AsNode(), nil, nil, nil)
	x.SetMType(typeExprU64)
	return x
}

// makeSliceLengthEqEq returns "x.length() == n".
//
// n must be the t.ID of a small power of 2.
func makeSliceLengthEqEq(x t.ID, xTyp *a.TypeExpr, n t.ID) *a.Expr {
	xExpr := a.NewExpr(0, 0, 0, x, nil, nil, nil, nil)
	xExpr.SetMType(xTyp)

	lhs := makeSliceLength(xExpr)

	nValue := n.SmallPowerOf2Value()
	if nValue == 0 {
		panic("check: internal error: makeSliceLengthEqEq called but not with a small power of 2")
	}
	rhs := a.NewExpr(0, 0, 0, n, nil, nil, nil, nil)
	rhs.SetConstValue(big.NewInt(int64(nValue)))
	rhs.SetMType(typeExprIdeal)

	ret := a.NewExpr(0, t.IDXBinaryEqEq, 0, 0, lhs.AsNode(), nil, rhs.AsNode(), nil)
	ret.SetMType(typeExprBool)
	return ret
}

func (q *checker) bcheckExprUnaryOp(n *a.Expr, depth uint32) (a.Bounds, error) {
	rb, err := q.bcheckExpr(n.RHS().AsExpr(), depth)
	if err != nil {
		return a.Bounds{}, err
	}

	switch n.Operator() {
	case t.IDXUnaryPlus:
		return rb, nil
	case t.IDXUnaryMinus:
		return a.Bounds{neg(rb[1]), neg(rb[0])}, nil
	case t.IDXUnaryNot:
		return a.Bounds{zero, one}, nil
	case t.IDXUnaryRef, t.IDXUnaryDeref:
		return typeBounds(q.tm, n.MType())
	}

	return a.Bounds{}, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprUnaryOp", n.Operator())
}

func (q *checker) bcheckExprXBinaryPlus(lhs *a.Expr, lb a.Bounds, rhs *a.Expr, rb a.Bounds) (a.Bounds, error) {
	return a.Bounds{
		big.NewInt(0).Add(lb[0], rb[0]),
		big.NewInt(0).Add(lb[1], rb[1]),
	}, nil
}

func (q *checker) bcheckExprXBinaryMinus(lhs *a.Expr, lb a.Bounds, rhs *a.Expr, rb a.Bounds) (a.Bounds, error) {
	nMin := big.NewInt(0).Sub(lb[0], rb[1])
	nMax := big.NewInt(0).Sub(lb[1], rb[0])
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
	return a.Bounds{nMin, nMax}, nil
}

func (q *checker) bcheckExprBinaryOp(op t.ID, lhs *a.Expr, rhs *a.Expr, depth uint32) (a.Bounds, error) {
	lb, err := q.bcheckExpr(lhs, depth)
	if err != nil {
		return a.Bounds{}, err
	}
	return q.bcheckExprBinaryOp1(op, lhs, lb, rhs, depth)
}

func (q *checker) bcheckExprBinaryOp1(op t.ID, lhs *a.Expr, lb a.Bounds, rhs *a.Expr, depth uint32) (a.Bounds, error) {
	rb, err := q.bcheckExpr(rhs, depth)
	if err != nil {
		return a.Bounds{}, err
	}

	switch op {
	case t.IDXBinaryPlus:
		return q.bcheckExprXBinaryPlus(lhs, lb, rhs, rb)

	case t.IDXBinaryMinus:
		return q.bcheckExprXBinaryMinus(lhs, lb, rhs, rb)

	case t.IDXBinaryStar:
		// TODO: handle multiplication by negative numbers. Note that this
		// might reverse the inequality: if 0 < a < b but c < 0 then a*c > b*c.
		if lb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: multiply op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: multiply op argument %q is possibly negative", rhs.Str(q.tm))
		}
		return a.Bounds{
			big.NewInt(0).Mul(lb[0], rb[0]),
			big.NewInt(0).Mul(lb[1], rb[1]),
		}, nil

	case t.IDXBinarySlash, t.IDXBinaryPercent:
		// Prohibit division by zero.
		if lb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: divide/modulus op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rb[0].Sign() <= 0 {
			return a.Bounds{}, fmt.Errorf("check: divide/modulus op argument %q is possibly non-positive", rhs.Str(q.tm))
		}
		if op == t.IDXBinarySlash {
			return a.Bounds{
				big.NewInt(0).Mul(lb[0], rb[1]),
				big.NewInt(0).Mul(lb[1], rb[0]),
			}, nil
		}
		return a.Bounds{
			zero,
			big.NewInt(0).Sub(rb[1], one),
		}, nil

	case t.IDXBinaryShiftL, t.IDXBinaryTildeModShiftL:
		if lb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: shift op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: shift op argument %q is possibly negative", rhs.Str(q.tm))
		}
		if rb[1].Cmp(ffff) > 0 {
			return a.Bounds{}, fmt.Errorf("check: shift %q out of range", rhs.Str(q.tm))
		}
		nMin := big.NewInt(0).Lsh(lb[0], uint(rb[0].Uint64()))
		nMax := big.NewInt(0).Lsh(lb[1], uint(rb[1].Uint64()))
		if op == t.IDXBinaryTildeModShiftL {
			if qid := lhs.MType().QID(); qid[0] == t.IDBase {
				b := numTypeBounds[qid[1]]
				nMax = min(nMax, b[1])
			}
		}
		return a.Bounds{nMin, nMax}, nil

	case t.IDXBinaryShiftR:
		if lb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: shift op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: shift op argument %q is possibly negative", rhs.Str(q.tm))
		}
		if rb[0].Cmp(maxIntBits) >= 0 {
			return a.Bounds{zero, zero}, nil
		}
		nMax := big.NewInt(0).Rsh(lb[1], uint(rb[0].Uint64()))
		if rb[1].Cmp(maxIntBits) >= 0 {
			return a.Bounds{zero, nMax}, nil
		}
		return a.Bounds{
			big.NewInt(0).Rsh(lb[0], uint(rb[1].Uint64())),
			nMax,
		}, nil

	case t.IDXBinaryAmp, t.IDXBinaryPipe, t.IDXBinaryHat:
		// TODO: should type-checking ensure that bitwise ops only apply to
		// *unsigned* integer types?
		if lb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: bitwise op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if lb[1].Cmp(numTypeBounds[t.IDU64][1]) > 0 {
			return a.Bounds{}, fmt.Errorf("check: bitwise op argument %q is possibly too large", lhs.Str(q.tm))
		}
		if rb[0].Sign() < 0 {
			return a.Bounds{}, fmt.Errorf("check: bitwise op argument %q is possibly negative", rhs.Str(q.tm))
		}
		if rb[1].Cmp(numTypeBounds[t.IDU64][1]) > 0 {
			return a.Bounds{}, fmt.Errorf("check: bitwise op argument %q is possibly too large", rhs.Str(q.tm))
		}
		z := (*big.Int)(nil)
		switch op {
		case t.IDXBinaryAmp:
			z = min(lb[1], rb[1])
		case t.IDXBinaryPipe, t.IDXBinaryHat:
			z = max(lb[1], rb[1])
		}
		// Return [0, z rounded up to the next power-of-2-minus-1]. This is
		// conservative, but works fine in practice.
		return a.Bounds{
			zero,
			bitMask(z.BitLen()),
		}, nil

	case t.IDXBinaryTildeModPlus, t.IDXBinaryTildeModMinus:
		typ := lhs.MType()
		if typ.IsIdeal() {
			typ = rhs.MType()
		}
		if qid := typ.QID(); qid[0] == t.IDBase {
			return numTypeBounds[qid[1]], nil
		}

	case t.IDXBinaryTildeSatPlus, t.IDXBinaryTildeSatMinus:
		typ := lhs.MType()
		if typ.IsIdeal() {
			typ = rhs.MType()
		}
		if qid := typ.QID(); qid[0] == t.IDBase {
			b := numTypeBounds[qid[1]]

			nFunc := (*checker).bcheckExprXBinaryPlus
			if op != t.IDXBinaryTildeSatPlus {
				nFunc = (*checker).bcheckExprXBinaryMinus
			}
			nb, err := nFunc(q, lhs, lb, rhs, rb)
			if err != nil {
				return a.Bounds{}, err
			}

			if op == t.IDXBinaryTildeSatPlus {
				nb[0] = min(nb[0], b[1])
				nb[1] = min(nb[1], b[1])
			} else {
				nb[0] = max(nb[0], b[0])
				nb[1] = max(nb[1], b[0])
			}
			return nb, nil
		}

	case t.IDXBinaryNotEq, t.IDXBinaryLessThan, t.IDXBinaryLessEq, t.IDXBinaryEqEq,
		t.IDXBinaryGreaterEq, t.IDXBinaryGreaterThan, t.IDXBinaryAnd, t.IDXBinaryOr:
		return a.Bounds{zero, one}, nil

	case t.IDXBinaryAs:
		// Unreachable, as this is checked by the caller.
	}
	return a.Bounds{}, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprBinaryOp", op)
}

func (q *checker) bcheckExprAssociativeOp(n *a.Expr, depth uint32) (a.Bounds, error) {
	op := n.Operator().AmbiguousForm().BinaryForm()
	if op == 0 {
		return a.Bounds{}, fmt.Errorf(
			"check: unrecognized token (0x%X) for bcheckExprAssociativeOp", n.Operator())
	}
	args := n.Args()
	if len(args) < 1 {
		return a.Bounds{}, fmt.Errorf("check: associative op has no arguments")
	}
	lb, err := q.bcheckExpr(args[0].AsExpr(), depth)
	if err != nil {
		return a.Bounds{}, err
	}
	for i, o := range args {
		if i == 0 {
			continue
		}
		lhs := a.NewExpr(n.AsNode().AsRaw().Flags(),
			n.Operator(), 0, n.Ident(), n.LHS(), n.MHS(), n.RHS(), args[:i])
		lb, err = q.bcheckExprBinaryOp1(op, lhs, lb, o.AsExpr(), depth)
		if err != nil {
			return a.Bounds{}, err
		}
	}
	return lb, nil
}

func typeBounds(tm *t.Map, typ *a.TypeExpr) (a.Bounds, error) {
	if typ.IsIdeal() {
		return a.Bounds{minIdeal, maxIdeal}, nil
	}

	switch typ.Decorator() {
	case t.IDArray, t.IDSlice, t.IDTable:
		return a.Bounds{zero, zero}, nil
	case t.IDNptr:
		return a.Bounds{zero, one}, nil
	case t.IDPtr, t.IDFunc:
		return a.Bounds{one, one}, nil
	}

	b := a.Bounds{zero, zero}

	if qid := typ.QID(); qid[0] == t.IDBase {
		if qid[1] == t.IDDagger1 || qid[1] == t.IDDagger2 {
			return a.Bounds{zero, zero}, nil
		} else if qid[1] < t.ID(len(numTypeBounds)) {
			if x := numTypeBounds[qid[1]]; x[0] != nil {
				b = x
			}
		}
	}

	if typ.IsRefined() {
		if x := typ.Min(); x != nil {
			if cv := x.ConstValue(); cv == nil {
				return a.Bounds{}, fmt.Errorf("check: internal error: refinement has no const-value")
			} else if cv.Cmp(b[0]) < 0 {
				return a.Bounds{}, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.Str(tm))
			} else {
				b[0] = cv
			}
		}

		if x := typ.Max(); x != nil {
			if cv := x.ConstValue(); cv == nil {
				return a.Bounds{}, fmt.Errorf("check: internal error: refinement has no const-value")
			} else if cv.Cmp(b[1]) > 0 {
				return a.Bounds{}, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.Str(tm))
			} else {
				b[1] = cv
			}
		}
	}

	return b, nil
}
