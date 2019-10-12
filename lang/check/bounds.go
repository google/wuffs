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

	"github.com/google/wuffs/lib/interval"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

type bounds = interval.IntRange

var numShiftBounds = [...]bounds{
	t.IDU8:  {zero, big.NewInt(7)},
	t.IDU16: {zero, big.NewInt(15)},
	t.IDU32: {zero, big.NewInt(31)},
	t.IDU64: {zero, big.NewInt(63)},
}

var numTypeBounds = [...]bounds{
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
	zeroExpr.SetMBounds(bounds{zero, zero})
	zeroExpr.SetMType(typeExprIdeal)
}

func isErrorStatus(literal t.ID, tm *t.Map) bool {
	s := literal.Str(tm)
	return (len(s) >= 2) && (s[0] == '"') && (s[1] == '#')
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
	// TODO: after a "break loop", do we need to set placeholder MBounds so
	// that everything (including unreachable code) is bounds checked?
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
			// Drop any facts involving args or this.
			if err := q.facts.update(func(x *a.Expr) (*a.Expr, error) {
				if x.Mentions(exprArgs) || x.Mentions(exprThis) {
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

	switch n.Kind() {
	case a.KAssert:
		if err := q.bcheckAssert(n.AsAssert()); err != nil {
			return err
		}

	case a.KAssign:
		n := n.AsAssign()
		if err := q.bcheckAssignment(n.LHS(), n.Operator(), n.RHS()); err != nil {
			return err
		}

	case a.KIOBind:
		n := n.AsIOBind()
		if _, err := q.bcheckExpr(n.IO(), 0); err != nil {
			return err
		}
		if _, err := q.bcheckExpr(n.Arg1(), 0); err != nil {
			return err
		}
		if err := q.bcheckBlock(n.Body()); err != nil {
			return err
		}
		// TODO: invalidate any facts regarding the io_bind expressions.

	case a.KIf:
		if err := q.bcheckIf(n.AsIf()); err != nil {
			return err
		}

	case a.KIterate:
		n := n.AsIterate()
		for _, o := range n.Assigns() {
			o := o.AsAssign()
			if err := q.bcheckAssignment(o.LHS(), o.Operator(), o.RHS()); err != nil {
				return err
			}
		}
		// TODO: this isn't right, as the body is a loop, not an
		// execute-exactly-once block. We should have pre / inv / post
		// conditions, a la bcheckWhile.

		assigns := n.Assigns()
		for ; n != nil; n = n.ElseIterate() {
			q.facts = q.facts[:0]
			for _, o := range assigns {
				lhs := o.AsAssign().LHS()
				q.facts = append(q.facts, makeSliceLengthEqEq(lhs.Ident(), lhs.MType(), n.Length()))
			}
			if err := q.bcheckBlock(n.Body()); err != nil {
				return err
			}
		}

		q.facts = q.facts[:0]

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

	case a.KRet:
		n := n.AsRet()
		lTyp := q.astFunc.Out()
		if q.astFunc.Effect().Coroutine() {
			lTyp = typeExprStatus
		} else if lTyp == nil {
			lTyp = typeExprEmptyStruct
		}
		if _, err := q.bcheckAssignment1(nil, lTyp, t.IDEq, n.Value()); err != nil {
			return err
		}

		if lTyp.IsStatus() {
			if v := n.Value(); v.Operator() == 0 {
				if id := v.Ident(); (id != t.IDOk) && (q.hasIsErrorFact(id) || isErrorStatus(id, q.tm)) {
					n.SetRetsError()
				}
			}
		}

	case a.KVar:
		if err := q.bcheckVar(n.AsVar()); err != nil {
			return err
		}

	case a.KWhile:
		if err := q.bcheckWhile(n.AsWhile()); err != nil {
			return err
		}

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%s) for bcheckStatement", n.Kind())
	}
	return nil
}

func (q *checker) hasIsErrorFact(id t.ID) bool {
	for _, x := range q.facts {
		if (x.Operator() != t.IDOpenParen) || (len(x.Args()) != 0) {
			continue
		}
		x = x.LHS().AsExpr()
		if (x.Operator() != t.IDDot) || (x.Ident() != t.IDIsError) {
			continue
		}
		x = x.LHS().AsExpr()
		if (x.Operator() != 0) || (x.Ident() != id) {
			continue
		}
		return true
	}
	return false
}

func (q *checker) bcheckAssert(n *a.Assert) error {
	condition := n.Condition()
	if _, err := q.bcheckExpr(condition, 0); err != nil {
		return err
	}
	for _, o := range n.Args() {
		if _, err := q.bcheckExpr(o.AsArg().Value(), 0); err != nil {
			return err
		}
	}

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
	lTyp := (*a.TypeExpr)(nil)
	if lhs != nil {
		if _, err := q.bcheckExpr(lhs, 0); err != nil {
			return err
		}
		lTyp = lhs.MType()
	}

	nb, err := q.bcheckAssignment1(lhs, lTyp, op, rhs)
	if err != nil {
		return err
	}

	if lhs == nil {
		return nil
	}

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

		if lhs.MType().IsNumType() && rhs.Effect().Pure() {
			q.facts.appendBinaryOpFact(t.IDXBinaryEqEq, lhs, rhs)
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
				// TODO: call SetMBounds?
				oRHS.SetMType(typeExprIdeal)
				oRHS, err := simplify(q.tm, oRHS)
				if err != nil {
					return nil, err
				}
				o := a.NewExpr(0, xOp, 0, 0, xLHS.AsNode(), nil, oRHS.AsNode(), nil)
				o.SetMBounds(bounds{zero, one})
				o.SetMType(typeExprBool)
				return o, nil
			}
			return nil, nil
		}); err != nil {
			return err
		}
	}

	if lhs.MType().IsNumType() && ((op != t.IDEq) || (rhs.ConstValue() == nil)) {
		lb, err := q.bcheckTypeExpr(lhs.MType())
		if err != nil {
			return err
		}
		if lb[0].Cmp(nb[0]) < 0 {
			c, err := makeConstValueExpr(q.tm, nb[0])
			if err != nil {
				return err
			}
			q.facts.appendBinaryOpFact(t.IDXBinaryGreaterEq, lhs, c)
		}
		if lb[1].Cmp(nb[1]) > 0 {
			c, err := makeConstValueExpr(q.tm, nb[1])
			if err != nil {
				return err
			}
			q.facts.appendBinaryOpFact(t.IDXBinaryLessEq, lhs, c)
		}
	}

	return nil
}

func (q *checker) bcheckAssignment1(lhs *a.Expr, lTyp *a.TypeExpr, op t.ID, rhs *a.Expr) (bounds, error) {
	if lhs == nil && op != t.IDEq {
		return bounds{}, fmt.Errorf("check: internal error: missing LHS for op key 0x%02X", op)
	}

	lb, err := bounds{}, (error)(nil)
	if lTyp != nil {
		lb, err = q.bcheckTypeExpr(lTyp)
		if err != nil {
			return bounds{}, err
		}
	}

	rb := bounds{}
	if op == t.IDEq || op == t.IDEqQuestion {
		rb, err = q.bcheckExpr(rhs, 0)
	} else {
		rb, err = q.bcheckExprBinaryOp(op.BinaryForm(), lhs, rhs, 0)
	}
	if err != nil {
		return bounds{}, err
	}

	if (lTyp != nil) && ((rb[0].Cmp(lb[0]) < 0) || (rb[1].Cmp(lb[1]) > 0)) {
		if op == t.IDEq {
			return bounds{}, fmt.Errorf("check: expression %q bounds %v is not within bounds %v",
				rhs.Str(q.tm), rb, lb)
		} else {
			return bounds{}, fmt.Errorf("check: assignment %q bounds %v is not within bounds %v",
				lhs.Str(q.tm)+" "+op.Str(q.tm)+" "+rhs.Str(q.tm), rb, lb)
		}
	}
	return rb, nil
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
		if _, err := q.bcheckExpr(n.Condition(), 0); err != nil {
			return err
		}

		// Check the if-true branch, assuming the if condition.
		if n.Condition().ConstValue() == nil {
			q.facts.appendFact(n.Condition())
		}
		if err := q.bcheckBlock(n.BodyIfTrue()); err != nil {
			return err
		}
		if !terminates(n.BodyIfTrue()) {
			branches = append(branches, snapshot(q.facts))
		}

		// Check the if-false branch, assuming the inverted if condition.
		q.facts = append(q.facts[:0], snap...)
		if n.Condition().ConstValue() == nil {
			if inverse, err := invert(q.tm, n.Condition()); err != nil {
				return err
			} else {
				q.facts.appendFact(inverse)
			}
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

func (q *checker) bcheckVar(n *a.Var) error {
	if _, err := q.bcheckTypeExpr(n.XType()); err != nil {
		return err
	}

	lhs := a.NewExpr(0, 0, 0, n.Name(), nil, nil, nil, nil)
	lhs.SetMType(n.XType())
	// "var x T" has an implicit "= 0".
	//
	// TODO: check that T is an integer type.
	rhs := zeroExpr
	return q.bcheckAssignment(lhs, t.IDEq, rhs)
}

func (q *checker) bcheckExpr(n *a.Expr, depth uint32) (bounds, error) {
	if depth > a.MaxExprDepth {
		return bounds{}, fmt.Errorf("check: expression recursion depth too large")
	}
	depth++

	if b := n.MBounds(); b[0] != nil {
		return b, nil
	}
	if n.ConstValue() != nil {
		return bcheckExprConstValue(n), nil
	}

	nb, err := q.bcheckExpr1(n, depth)
	if err != nil {
		return bounds{}, err
	}
	nb, err = q.facts.refine(n, nb, q.tm)
	if err != nil {
		return bounds{}, err
	}
	tb, err := q.bcheckTypeExpr(n.MType())
	if err != nil {
		return bounds{}, err
	}

	if (nb[0].Cmp(tb[0]) < 0) || (nb[1].Cmp(tb[1]) > 0) {
		return bounds{}, fmt.Errorf("check: expression %q bounds %v is not within bounds %v",
			n.Str(q.tm), nb, tb)
	}

	n.SetMBounds(nb)
	return nb, nil
}

func bcheckExprConstValue(n *a.Expr) bounds {
	if o := n.LHS(); o != nil {
		bcheckExprConstValue(o.AsExpr())
	}
	if o := n.MHS(); o != nil {
		bcheckExprConstValue(o.AsExpr())
	}
	if o := n.RHS(); o != nil && n.Operator() != t.IDXBinaryAs {
		bcheckExprConstValue(o.AsExpr())
	}
	cv := n.ConstValue()
	b := bounds{cv, cv}
	n.SetMBounds(b)
	return b
}

func (q *checker) bcheckExpr1(n *a.Expr, depth uint32) (bounds, error) {
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

func (q *checker) bcheckExprOther(n *a.Expr, depth uint32) (bounds, error) {
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
				return bounds{cv, cv}, nil
			}
		}

	case t.IDOpenParen:
		lhs := n.LHS().AsExpr()
		if _, err := q.bcheckExpr(lhs, depth); err != nil {
			return bounds{}, err
		}
		if err := q.bcheckExprCall(n, depth); err != nil {
			return bounds{}, err
		}
		if nb, err := q.bcheckExprCallSpecialCases(n, depth); err == nil {
			return nb, nil
		} else if err != errNotASpecialCase {
			return bounds{}, err
		}

	case t.IDOpenBracket:
		lhs := n.LHS().AsExpr()
		if _, err := q.bcheckExpr(lhs, depth); err != nil {
			return bounds{}, err
		}
		rhs := n.RHS().AsExpr()
		if _, err := q.bcheckExpr(rhs, depth); err != nil {
			return bounds{}, err
		}

		lengthExpr := (*a.Expr)(nil)
		if lTyp := lhs.MType(); lTyp.IsArrayType() {
			lengthExpr = lTyp.ArrayLength()
		} else {
			lengthExpr = makeSliceLength(lhs)
		}

		if err := proveReasonRequirement(q, t.IDXBinaryLessEq, zeroExpr, rhs); err != nil {
			return bounds{}, err
		}
		if err := proveReasonRequirementForRHSLength(q, t.IDXBinaryLessThan, rhs, lengthExpr); err != nil {
			return bounds{}, err
		}

	case t.IDDotDot:
		lhs := n.LHS().AsExpr()
		if _, err := q.bcheckExpr(lhs, depth); err != nil {
			return bounds{}, err
		}
		mhs := n.MHS().AsExpr()
		if mhs != nil {
			if _, err := q.bcheckExpr(mhs, depth); err != nil {
				return bounds{}, err
			}
		}
		rhs := n.RHS().AsExpr()
		if rhs != nil {
			if _, err := q.bcheckExpr(rhs, depth); err != nil {
				return bounds{}, err
			}
		}

		if mhs == nil && rhs == nil {
			return bounds{zero, zero}, nil
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
				return bounds{}, err
			}
		}
		if err := proveReasonRequirement(q, t.IDXBinaryLessEq, mhs, rhs); err != nil {
			return bounds{}, err
		}
		if rhs != lengthExpr {
			if err := proveReasonRequirementForRHSLength(q, t.IDXBinaryLessEq, rhs, lengthExpr); err != nil {
				return bounds{}, err
			}
		}

	case t.IDDot:
		if _, err := q.bcheckExpr(n.LHS().AsExpr(), depth); err != nil {
			return bounds{}, err
		}

		// TODO: delete this hack that only matches "args".
		if n.LHS().AsExpr().Ident() == t.IDArgs {
			for _, o := range q.astFunc.In().Fields() {
				o := o.AsField()
				if o.Name() == n.Ident() {
					return q.bcheckTypeExpr(o.XType())
				}
			}
			lTyp := n.LHS().AsExpr().MType()
			return bounds{}, fmt.Errorf("check: no field named %q found in struct type %q for expression %q",
				n.Ident().Str(q.tm), lTyp.QID().Str(q.tm), n.Str(q.tm))
		}

	case t.IDComma:
		for _, o := range n.Args() {
			if _, err := q.bcheckExpr(o.AsExpr(), depth); err != nil {
				return bounds{}, err
			}
		}

	default:
		return bounds{}, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprOther", n.Operator())
	}
	return q.bcheckTypeExpr(n.MType())
}

func (q *checker) bcheckExprCall(n *a.Expr, depth uint32) error {
	// TODO: handle func pre/post conditions.
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
		if _, err := q.bcheckAssignment1(nil, inFields[i].AsField().XType(), t.IDEq, o.AsArg().Value()); err != nil {
			return err
		}
	}

	recv := lhs.LHS().AsExpr()
	if recv.MType().Decorator() != t.IDNptr {
		return nil
	}
	// Check that q.facts contain "recv != nullptr".
	for _, x := range q.facts {
		if x.Operator() != t.IDXBinaryNotEq {
			continue
		}
		xLHS := x.LHS().AsExpr()
		xRHS := x.RHS().AsExpr()
		if (xLHS.Eq(exprNullptr) && xRHS.Eq(recv)) ||
			(xRHS.Eq(exprNullptr) && xLHS.Eq(recv)) {
			return nil
		}
	}
	return fmt.Errorf("check: cannot prove %q", recv.Str(q.tm)+" != nullptr")
}

var errNotASpecialCase = errors.New("not a special case")

func (q *checker) bcheckExprCallSpecialCases(n *a.Expr, depth uint32) (bounds, error) {
	lhs := n.LHS().AsExpr()
	recv := lhs.LHS().AsExpr()
	method := lhs.Ident()

	if recvTyp := recv.MType(); recvTyp == nil {
		return bounds{}, errNotASpecialCase

	} else if recvTyp.IsNumType() {
		// For a numeric type's low_bits, etc. methods. The bound on the output
		// is dependent on bound on the input, similar to dependent types, and
		// isn't expressible in Wuffs' function syntax and type system.
		switch method {
		case t.IDLowBits, t.IDHighBits:
			ab, err := q.bcheckExpr(n.Args()[0].AsArg().Value(), depth)
			if err != nil {
				return bounds{}, err
			}
			return bounds{
				zero,
				bitMask(int(ab[1].Int64())),
			}, nil

		case t.IDMin, t.IDMax:
			// TODO: lhs has already been bcheck'ed. There should be no
			// need to bcheck lhs.LHS().Expr() twice.
			lb, err := q.bcheckExpr(lhs.LHS().AsExpr(), depth)
			if err != nil {
				return bounds{}, err
			}
			ab, err := q.bcheckExpr(n.Args()[0].AsArg().Value(), depth)
			if err != nil {
				return bounds{}, err
			}
			if method == t.IDMin {
				return bounds{
					min(lb[0], ab[0]),
					min(lb[1], ab[1]),
				}, nil
			} else {
				return bounds{
					max(lb[0], ab[0]),
					max(lb[1], ab[1]),
				}, nil
			}
		}

	} else if recvTyp.IsIOType() {
		advance, update := (*big.Int)(nil), false

		if method == t.IDUndoByte {
			if err := q.canUndoByte(recv); err != nil {
				return bounds{}, err
			}

		} else if method == t.IDCopyNFromHistoryFast {
			if err := q.canCopyNFromHistoryFast(recv, n.Args()); err != nil {
				return bounds{}, err
			}

		} else if method == t.IDSkipFast {
			args := n.Args()
			if len(args) != 2 {
				return bounds{}, fmt.Errorf("check: internal error: bad skip_fast arguments")
			}
			actual := args[0].AsArg().Value()
			worstCase := args[1].AsArg().Value()
			if err := q.proveBinaryOp(t.IDXBinaryLessEq, actual, worstCase); err == errFailed {
				return bounds{}, fmt.Errorf("check: could not prove skip_fast pre-condition: %s <= %s",
					actual.Str(q.tm), worstCase.Str(q.tm))
			} else if err != nil {
				return bounds{}, err
			}
			advance, update = worstCase.ConstValue(), true
			if advance == nil {
				return bounds{}, fmt.Errorf("check: skip_fast worst_case is not a constant value")
			}

		} else if method >= t.IDPeekU8 {
			if m := method - t.IDPeekU8; m < t.ID(len(ioMethodAdvances)) {
				au := ioMethodAdvances[m]
				advance, update = au.advance, au.update
			}
		}

		if advance != nil {
			if ok, err := q.optimizeIOMethodAdvance(recv, advance, update); err != nil {
				return bounds{}, err
			} else if !ok {
				return bounds{}, fmt.Errorf("check: could not prove %s pre-condition: %s.available() >= %v",
					method.Str(q.tm), recv.Str(q.tm), advance)
			}
			// TODO: drop other recv-related facts?
		}
	}

	return bounds{}, errNotASpecialCase
}

func (q *checker) canUndoByte(recv *a.Expr) error {
	for _, x := range q.facts {
		if x.Operator() != t.IDOpenParen || len(x.Args()) != 0 {
			continue
		}
		x = x.LHS().AsExpr()
		if x.Operator() != t.IDDot || x.Ident() != t.IDCanUndoByte {
			continue
		}
		x = x.LHS().AsExpr()
		if !x.Eq(recv) {
			continue
		}
		return q.facts.update(func(o *a.Expr) (*a.Expr, error) {
			if o.Mentions(recv) {
				return nil, nil
			}
			return o, nil
		})
	}
	return fmt.Errorf("check: could not prove %s.can_undo_byte()", recv.Str(q.tm))
}

func (q *checker) canCopyNFromHistoryFast(recv *a.Expr, args []*a.Node) error {
	// As per cgen's io-private.h, there are three pre-conditions:
	//  - n <= this.available()
	//  - distance > 0
	//  - distance <= this.history_available()

	if len(args) != 2 {
		return fmt.Errorf("check: internal error: inconsistent copy_n_from_history_fast arguments")
	}
	n := args[0].AsArg().Value()
	distance := args[1].AsArg().Value()

	// Check "n <= this.available()".
check0:
	for {
		for _, x := range q.facts {
			if x.Operator() != t.IDXBinaryLessEq {
				continue
			}

			// Check that the LHS is "n as base.u64".
			lhs := x.LHS().AsExpr()
			if lhs.Operator() != t.IDXBinaryAs {
				continue
			}
			llhs, lrhs := lhs.LHS().AsExpr(), lhs.RHS().AsTypeExpr()
			if !llhs.Eq(n) || !lrhs.Eq(typeExprU64) {
				continue
			}

			// Check that the RHS is "recv.available()".
			y, method, yArgs := splitReceiverMethodArgs(x.RHS().AsExpr())
			if method != t.IDAvailable || len(yArgs) != 0 {
				continue
			}
			if !y.Eq(recv) {
				continue
			}

			break check0
		}
		return fmt.Errorf("check: could not prove n <= %s.available()", recv.Str(q.tm))
	}

	// Check "distance > 0".
check1:
	for {
		for _, x := range q.facts {
			if x.Operator() != t.IDXBinaryGreaterThan {
				continue
			}
			if lhs := x.LHS().AsExpr(); !lhs.Eq(distance) {
				continue
			}
			if rcv := x.RHS().AsExpr().ConstValue(); rcv == nil || rcv.Sign() != 0 {
				continue
			}
			break check1
		}
		return fmt.Errorf("check: could not prove distance > 0")
	}

	// Check "distance <= this.history_available()".
check2:
	for {
		for _, x := range q.facts {
			if x.Operator() != t.IDXBinaryLessEq {
				continue
			}

			// Check that the LHS is "distance as base.u64".
			lhs := x.LHS().AsExpr()
			if lhs.Operator() != t.IDXBinaryAs {
				continue
			}
			llhs, lrhs := lhs.LHS().AsExpr(), lhs.RHS().AsTypeExpr()
			if !llhs.Eq(distance) || !lrhs.Eq(typeExprU64) {
				continue
			}

			// Check that the RHS is "recv.history_available()".
			y, method, yArgs := splitReceiverMethodArgs(x.RHS().AsExpr())
			if method != t.IDHistoryAvailable || len(yArgs) != 0 {
				continue
			}
			if !y.Eq(recv) {
				continue
			}

			break check2
		}
		return fmt.Errorf("check: could not prove distance <= %s.history_available()", recv.Str(q.tm))
	}

	return nil
}

var ioMethodAdvances = [...]struct {
	advance *big.Int
	update  bool
}{
	t.IDPeekU8 - t.IDPeekU8: {one, false},

	t.IDPeekU16BE - t.IDPeekU8: {two, false},
	t.IDPeekU16LE - t.IDPeekU8: {two, false},

	t.IDPeekU8AsU32 - t.IDPeekU8:    {one, false},
	t.IDPeekU16BEAsU32 - t.IDPeekU8: {two, false},
	t.IDPeekU16LEAsU32 - t.IDPeekU8: {two, false},
	t.IDPeekU24BEAsU32 - t.IDPeekU8: {three, false},
	t.IDPeekU24LEAsU32 - t.IDPeekU8: {three, false},
	t.IDPeekU32BE - t.IDPeekU8:      {four, false},
	t.IDPeekU32LE - t.IDPeekU8:      {four, false},

	t.IDPeekU8AsU64 - t.IDPeekU8:    {one, false},
	t.IDPeekU16BEAsU64 - t.IDPeekU8: {two, false},
	t.IDPeekU16LEAsU64 - t.IDPeekU8: {two, false},
	t.IDPeekU24BEAsU64 - t.IDPeekU8: {three, false},
	t.IDPeekU24LEAsU64 - t.IDPeekU8: {three, false},
	t.IDPeekU32BEAsU64 - t.IDPeekU8: {four, false},
	t.IDPeekU32LEAsU64 - t.IDPeekU8: {four, false},
	t.IDPeekU40BEAsU64 - t.IDPeekU8: {five, false},
	t.IDPeekU40LEAsU64 - t.IDPeekU8: {five, false},
	t.IDPeekU48BEAsU64 - t.IDPeekU8: {six, false},
	t.IDPeekU48LEAsU64 - t.IDPeekU8: {six, false},
	t.IDPeekU56BEAsU64 - t.IDPeekU8: {seven, false},
	t.IDPeekU56LEAsU64 - t.IDPeekU8: {seven, false},
	t.IDPeekU64BE - t.IDPeekU8:      {eight, false},
	t.IDPeekU64LE - t.IDPeekU8:      {eight, false},

	t.IDWriteFastU8 - t.IDPeekU8:    {one, true},
	t.IDWriteFastU16BE - t.IDPeekU8: {two, true},
	t.IDWriteFastU16LE - t.IDPeekU8: {two, true},
	t.IDWriteFastU24BE - t.IDPeekU8: {three, true},
	t.IDWriteFastU24LE - t.IDPeekU8: {three, true},
	t.IDWriteFastU32BE - t.IDPeekU8: {four, true},
	t.IDWriteFastU32LE - t.IDPeekU8: {four, true},
	t.IDWriteFastU40BE - t.IDPeekU8: {five, true},
	t.IDWriteFastU40LE - t.IDPeekU8: {five, true},
	t.IDWriteFastU48BE - t.IDPeekU8: {six, true},
	t.IDWriteFastU48LE - t.IDPeekU8: {six, true},
	t.IDWriteFastU56BE - t.IDPeekU8: {seven, true},
	t.IDWriteFastU56LE - t.IDPeekU8: {seven, true},
	t.IDWriteFastU64BE - t.IDPeekU8: {eight, true},
	t.IDWriteFastU64LE - t.IDPeekU8: {eight, true},
}

func makeConstValueExpr(tm *t.Map, cv *big.Int) (*a.Expr, error) {
	id, err := tm.Insert(cv.String())
	if err != nil {
		return nil, err
	}
	o := a.NewExpr(0, 0, 0, id, nil, nil, nil, nil)
	o.SetConstValue(cv)
	o.SetMBounds(bounds{cv, cv})
	o.SetMType(typeExprIdeal)
	return o, nil
}

// makeSliceLength returns "x.length()".
func makeSliceLength(slice *a.Expr) *a.Expr {
	x := a.NewExpr(0, t.IDDot, 0, t.IDLength, slice.AsNode(), nil, nil, nil)
	x.SetMBounds(bounds{one, one})
	x.SetMType(a.NewTypeExpr(t.IDFunc, 0, t.IDLength, slice.MType().AsNode(), nil, nil))
	x = a.NewExpr(0, t.IDOpenParen, 0, 0, x.AsNode(), nil, nil, nil)
	// TODO: call SetMBounds?
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
	cv := big.NewInt(int64(nValue))

	rhs := a.NewExpr(0, 0, 0, n, nil, nil, nil, nil)
	rhs.SetConstValue(cv)
	rhs.SetMBounds(bounds{cv, cv})
	rhs.SetMType(typeExprIdeal)

	ret := a.NewExpr(0, t.IDXBinaryEqEq, 0, 0, lhs.AsNode(), nil, rhs.AsNode(), nil)
	ret.SetMBounds(bounds{zero, one})
	ret.SetMType(typeExprBool)
	return ret
}

func (q *checker) bcheckExprUnaryOp(n *a.Expr, depth uint32) (bounds, error) {
	rb, err := q.bcheckExpr(n.RHS().AsExpr(), depth)
	if err != nil {
		return bounds{}, err
	}

	switch n.Operator() {
	case t.IDXUnaryPlus:
		return rb, nil
	case t.IDXUnaryMinus:
		return bounds{neg(rb[1]), neg(rb[0])}, nil
	case t.IDXUnaryNot:
		return bounds{zero, one}, nil
	}

	return bounds{}, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprUnaryOp", n.Operator())
}

func (q *checker) bcheckExprXBinaryPlus(lhs *a.Expr, lb bounds, rhs *a.Expr, rb bounds) (bounds, error) {
	return lb.Add(rb), nil
}

func (q *checker) bcheckExprXBinaryMinus(lhs *a.Expr, lb bounds, rhs *a.Expr, rb bounds) (bounds, error) {
	nb := lb.Sub(rb)
	for _, x := range q.facts {
		xOp, xLHS, xRHS := parseBinaryOp(x)
		if !lhs.Eq(xLHS) || !rhs.Eq(xRHS) {
			continue
		}
		switch xOp {
		case t.IDXBinaryLessThan:
			nb[1] = min(nb[1], minusOne)
		case t.IDXBinaryLessEq:
			nb[1] = min(nb[1], zero)
		case t.IDXBinaryGreaterEq:
			nb[0] = max(nb[0], zero)
		case t.IDXBinaryGreaterThan:
			nb[0] = max(nb[0], one)
		}
	}
	return nb, nil
}

func (q *checker) bcheckExprBinaryOp(op t.ID, lhs *a.Expr, rhs *a.Expr, depth uint32) (bounds, error) {
	lb, err := q.bcheckExpr(lhs, depth)
	if err != nil {
		return bounds{}, err
	}
	return q.bcheckExprBinaryOp1(op, lhs, lb, rhs, depth)
}

func (q *checker) bcheckExprBinaryOp1(op t.ID, lhs *a.Expr, lb bounds, rhs *a.Expr, depth uint32) (bounds, error) {
	rb, err := q.bcheckExpr(rhs, depth)
	if err != nil {
		return bounds{}, err
	}

	switch op {
	case t.IDXBinaryPlus:
		return q.bcheckExprXBinaryPlus(lhs, lb, rhs, rb)

	case t.IDXBinaryMinus:
		return q.bcheckExprXBinaryMinus(lhs, lb, rhs, rb)

	case t.IDXBinaryStar:
		return lb.Mul(rb), nil

	case t.IDXBinarySlash, t.IDXBinaryPercent:
		// Prohibit division by zero.
		if lb[0].Sign() < 0 {
			return bounds{}, fmt.Errorf("check: divide/modulus op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rb[0].Sign() <= 0 {
			return bounds{}, fmt.Errorf("check: divide/modulus op argument %q is possibly non-positive", rhs.Str(q.tm))
		}
		if op == t.IDXBinarySlash {
			nb, _ := lb.Quo(rb)
			return nb, nil
		}
		return bounds{
			zero,
			big.NewInt(0).Sub(rb[1], one),
		}, nil

	case t.IDXBinaryShiftL, t.IDXBinaryTildeModShiftL, t.IDXBinaryShiftR:
		shiftBounds := bounds{}
		typeBounds := bounds{}
		if lTyp := lhs.MType(); lTyp.IsNumType() {
			id := int(lTyp.QID()[1])
			if id < len(numShiftBounds) {
				shiftBounds = numShiftBounds[id]
			}
			if id < len(numTypeBounds) {
				typeBounds = numTypeBounds[id]
			}
		}
		if shiftBounds[0] == nil {
			return bounds{}, fmt.Errorf("check: shift op argument %q of type %q does not have unsigned integer type",
				lhs.Str(q.tm), lhs.MType().Str(q.tm))
		} else if !shiftBounds.ContainsIntRange(rb) {
			return bounds{}, fmt.Errorf("check: shift op argument %q is outside the range %s", rhs.Str(q.tm), shiftBounds)
		}

		switch op {
		case t.IDXBinaryShiftL:
			nb, _ := lb.Lsh(rb)
			return nb, nil
		case t.IDXBinaryTildeModShiftL:
			nb, _ := lb.Lsh(rb)
			nb[1] = min(nb[1], typeBounds[1])
			return nb, nil
		case t.IDXBinaryShiftR:
			nb, _ := lb.Rsh(rb)
			return nb, nil
		}

	case t.IDXBinaryAmp, t.IDXBinaryPipe, t.IDXBinaryHat:
		// TODO: should type-checking ensure that bitwise ops only apply to
		// *unsigned* integer types?
		if lb[0].Sign() < 0 {
			return bounds{}, fmt.Errorf("check: bitwise op argument %q is possibly negative", lhs.Str(q.tm))
		}
		if rb[0].Sign() < 0 {
			return bounds{}, fmt.Errorf("check: bitwise op argument %q is possibly negative", rhs.Str(q.tm))
		}
		switch op {
		case t.IDXBinaryAmp:
			nb, _ := lb.And(rb)
			return nb, nil
		case t.IDXBinaryPipe:
			nb, _ := lb.Or(rb)
			return nb, nil
		case t.IDXBinaryHat:
			z := max(lb[1], rb[1])
			// Return [0, z rounded up to the next power-of-2-minus-1]. This is
			// conservative, but works fine in practice.
			return bounds{
				zero,
				bitMask(z.BitLen()),
			}, nil
		}

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
				return bounds{}, err
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
		return bounds{zero, one}, nil

	case t.IDXBinaryAs:
		// Unreachable, as this is checked by the caller.
	}
	return bounds{}, fmt.Errorf("check: unrecognized token (0x%X) for bcheckExprBinaryOp", op)
}

func (q *checker) bcheckExprAssociativeOp(n *a.Expr, depth uint32) (bounds, error) {
	op := n.Operator().AmbiguousForm().BinaryForm()
	if op == 0 {
		return bounds{}, fmt.Errorf(
			"check: unrecognized token (0x%X) for bcheckExprAssociativeOp", n.Operator())
	}
	args := n.Args()
	if len(args) < 1 {
		return bounds{}, fmt.Errorf("check: associative op has no arguments")
	}
	lb, err := q.bcheckExpr(args[0].AsExpr(), depth)
	if err != nil {
		return bounds{}, err
	}
	for i, o := range args {
		if i == 0 {
			continue
		}
		lhs := a.NewExpr(n.AsNode().AsRaw().Flags(),
			n.Operator(), 0, n.Ident(), n.LHS(), n.MHS(), n.RHS(), args[:i])
		lb, err = q.bcheckExprBinaryOp1(op, lhs, lb, o.AsExpr(), depth)
		if err != nil {
			return bounds{}, err
		}
	}
	return lb, nil
}

func (q *checker) bcheckTypeExpr(typ *a.TypeExpr) (bounds, error) {
	if b := typ.AsNode().MBounds(); b[0] != nil {
		return b, nil
	}
	b, err := q.bcheckTypeExpr1(typ)
	if err != nil {
		return bounds{}, err
	}
	typ.AsNode().SetMBounds(b)
	return b, nil
}

func (q *checker) bcheckTypeExpr1(typ *a.TypeExpr) (bounds, error) {
	if typ.IsIdeal() {
		return bounds{minIdeal, maxIdeal}, nil
	}

	if innTyp := typ.Inner(); innTyp != nil {
		if _, err := q.bcheckTypeExpr(innTyp); err != nil {
			return bounds{}, err
		}
	}

	switch typ.Decorator() {
	case 0:
		// No-op.
	case t.IDArray:
		if _, err := q.bcheckExpr(typ.ArrayLength(), 0); err != nil {
			return bounds{}, err
		}
		return bounds{zero, zero}, nil
	case t.IDFunc:
		if _, err := q.bcheckTypeExpr(typ.Receiver()); err != nil {
			return bounds{}, err
		}
		return bounds{one, one}, nil
	case t.IDNptr:
		return bounds{zero, one}, nil
	case t.IDPtr:
		return bounds{one, one}, nil
	case t.IDSlice, t.IDTable:
		return bounds{zero, zero}, nil
	default:
		return bounds{}, fmt.Errorf("check: internal error: unrecognized decorator")
	}

	b := bounds{zero, zero}

	if qid := typ.QID(); qid[0] == t.IDBase {
		if qid[1] == t.IDDagger1 || qid[1] == t.IDDagger2 {
			return bounds{zero, zero}, nil
		} else if qid[1] < t.ID(len(numTypeBounds)) {
			if x := numTypeBounds[qid[1]]; x[0] != nil {
				b = x
			}
		}
	}

	if typ.IsRefined() {
		if x := typ.Min(); x != nil {
			if _, err := q.bcheckExpr(x, 0); err != nil {
				return bounds{}, err
			}
			if cv := x.ConstValue(); cv == nil {
				return bounds{}, fmt.Errorf("check: internal error: refinement has no const-value")
			} else if cv.Cmp(b[0]) < 0 {
				return bounds{}, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.Str(q.tm))
			} else {
				b[0] = cv
			}
		}

		if x := typ.Max(); x != nil {
			if _, err := q.bcheckExpr(x, 0); err != nil {
				return bounds{}, err
			}
			if cv := x.ConstValue(); cv == nil {
				return bounds{}, fmt.Errorf("check: internal error: refinement has no const-value")
			} else if cv.Cmp(b[1]) > 0 {
				return bounds{}, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.Str(q.tm))
			} else {
				b[1] = cv
			}
		}
	}

	return b, nil
}
