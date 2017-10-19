// Copyright 2017 The Puffs Authors.
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
	t.KeyBool:  {zero, one},
}

var (
	minusOne  = big.NewInt(-1)
	zero      = big.NewInt(+0)
	one       = big.NewInt(+1)
	sixtyFour = big.NewInt(+64)
	ffff      = big.NewInt(0xFFFF)

	maxIntBits = big.NewInt(t.MaxIntBits)

	zeroExpr = a.NewExpr(a.FlagsTypeChecked, 0, t.IDZero, nil, nil, nil, nil)
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
		return numTypeBounds[t.KeyU8][1]
	case 16:
		return numTypeBounds[t.KeyU16][1]
	case 32:
		return numTypeBounds[t.KeyU32][1]
	case 64:
		return numTypeBounds[t.KeyU64][1]
	}
	z := big.NewInt(0).Lsh(one, uint(nBits))
	return z.Sub(z, one)
}

func invert(tm *t.Map, n *a.Expr) (*a.Expr, error) {
	if !n.MType().IsBool() {
		return nil, fmt.Errorf("check: invert(%q) called on non-bool-typed expression", n.String(tm))
	}
	if cv := n.ConstValue(); cv != nil {
		return nil, fmt.Errorf("check: invert(%q) called on constant expression", n.String(tm))
	}
	id0, lhs, rhs, args := n.ID0(), n.LHS().Expr(), n.RHS().Expr(), []*a.Node(nil)
	switch id0.Key() {
	case t.KeyXUnaryNot:
		return rhs, nil
	case t.KeyXBinaryNotEq:
		id0 = t.IDXBinaryEqEq
	case t.KeyXBinaryLessThan:
		id0 = t.IDXBinaryGreaterEq
	case t.KeyXBinaryLessEq:
		id0 = t.IDXBinaryGreaterThan
	case t.KeyXBinaryEqEq:
		id0 = t.IDXBinaryNotEq
	case t.KeyXBinaryGreaterEq:
		id0 = t.IDXBinaryLessThan
	case t.KeyXBinaryGreaterThan:
		id0 = t.IDXBinaryLessEq
	case t.KeyXBinaryAnd, t.KeyXBinaryOr:
		var err error
		lhs, err = invert(tm, lhs)
		if err != nil {
			return nil, err
		}
		rhs, err = invert(tm, rhs)
		if err != nil {
			return nil, err
		}
		if id0.Key() == t.KeyXBinaryAnd {
			id0 = t.IDXBinaryOr
		} else {
			id0 = t.IDXBinaryAnd
		}
	case t.KeyXAssociativeAnd, t.KeyXAssociativeOr:
		args = make([]*a.Node, 0, len(n.Args()))
		for _, a := range n.Args() {
			v, err := invert(tm, a.Expr())
			if err != nil {
				return nil, err
			}
			args = append(args, v.Node())
		}
		if id0.Key() == t.KeyXAssociativeAnd {
			id0 = t.IDXAssociativeOr
		} else {
			id0 = t.IDXAssociativeAnd
		}
	default:
		id0, lhs, rhs = t.IDXUnaryNot, nil, n
	}
	o := a.NewExpr(n.Node().Raw().Flags(), id0, 0, lhs.Node(), nil, rhs.Node(), args)
	o.SetMType(n.MType())
	return o, nil
}

func typeBounds(tm *t.Map, typ *a.TypeExpr) (*big.Int, *big.Int, error) {
	b := [2]*big.Int{}
	if key := typ.Name().Key(); typ.Decorator() == 0 && key < t.Key(len(numTypeBounds)) {
		b = numTypeBounds[key]
	}
	if b[0] == nil || b[1] == nil {
		return nil, nil, nil
	}
	if o := typ.Min(); o != nil {
		cv := o.ConstValue()
		if cv.Cmp(b[0]) < 0 {
			return nil, nil, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.String(tm))
		}
		b[0] = cv
	}
	if o := typ.Max(); o != nil {
		cv := o.ConstValue()
		if cv.Cmp(b[1]) > 0 {
			return nil, nil, fmt.Errorf("check: type refinement %v for %q is out of bounds", cv, typ.String(tm))
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
				n.DefaultValue().ConstValue(), n.Name().String(tm), innTyp.String(tm))
		}
		return nil
	}
	dv := zero
	if o := n.DefaultValue(); o != nil {
		dv = o.ConstValue()
	}
	if dv.Cmp(nMin) < 0 || dv.Cmp(nMax) > 0 {
		return fmt.Errorf("check: default value %v is not within bounds [%v..%v] for field %q",
			dv, nMin, nMax, n.Name().String(tm))
	}
	return nil
}

func (q *checker) bcheckBlock(block []*a.Node) error {
	for _, o := range block {
		if err := q.bcheckStatement(o); err != nil {
			return err
		}
		if k := o.Kind(); k == a.KJump || k == a.KReturn {
			break
		}
	}
	return nil
}

func (q *checker) bcheckStatement(n *a.Node) error {
	q.errFilename, q.errLine = n.Raw().FilenameLine()

	switch n.Kind() {
	case a.KAssert:
		return q.bcheckAssert(n.Assert())

	case a.KAssign:
		n := n.Assign()
		return q.bcheckAssignment(n.LHS(), n.Operator(), n.RHS())

	case a.KExpr:
		_, _, err := q.bcheckExpr(n.Expr(), 0)
		return err

	case a.KIf:
		return q.bcheckIf(n.If())

	case a.KJump:
		n := n.Jump()
		skip := t.KeyPost
		if n.Keyword().Key() == t.KeyBreak {
			skip = t.KeyPre
		}
		for _, o := range n.JumpTarget().Asserts() {
			if o.Assert().Keyword().Key() == skip {
				continue
			}
			if err := q.bcheckAssert(o.Assert()); err != nil {
				return err
			}
		}
		q.facts = q.facts[:0]
		return nil

	case a.KReturn:
		// TODO.

	case a.KVar:
		n := n.Var()
		if innTyp := n.XType().Innermost(); innTyp.IsRefined() {
			if _, _, err := typeBounds(q.tm, innTyp); err != nil {
				return err
			}
		}
		lhs := a.NewExpr(a.FlagsTypeChecked, 0, n.Name(), nil, nil, nil, nil)
		lhs.SetMType(n.XType())
		rhs := n.Value()
		if rhs == nil {
			// "var x T" has an implicit "= 0".
			//
			// TODO: check that T is an integer type.
			rhs = zeroExpr
		}
		return q.bcheckAssignment(lhs, t.IDEq, rhs)

	case a.KWhile:
		return q.bcheckWhile(n.While())

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%s) for bcheckStatement", n.Kind())
	}

	return nil
}

func (q *checker) bcheckAssert(n *a.Assert) error {
	condition := n.Condition()
	err := errFailed
	if cv := condition.ConstValue(); cv != nil {
		if cv.Cmp(one) == 0 {
			err = nil
		}
	} else if reasonID := n.Reason(); reasonID != 0 {
		if reasonFunc := q.reasonMap[reasonID.Key()]; reasonFunc != nil {
			err = reasonFunc(q, n)
		} else {
			err = fmt.Errorf("no such reason %s", reasonID.String(q.tm))
		}
	} else if condition.ID0().IsBinaryOp() && condition.ID0().Key() != t.KeyAs {
		err = q.proveBinaryOp(condition.ID0().Key(), condition.LHS().Expr(), condition.RHS().Expr())
	}
	if err != nil {
		if err == errFailed {
			return fmt.Errorf("check: cannot prove %q", condition.String(q.tm))
		}
		return fmt.Errorf("check: cannot prove %q: %v", condition.String(q.tm), err)
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
			o := a.NewExpr(a.FlagsTypeChecked, t.IDXBinaryEqEq, 0, lhs.Node(), nil, rhs.Node(), nil)
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
			switch op.Key() {
			case t.KeyPlusEq, t.KeyMinusEq:
				oRHS := a.NewExpr(a.FlagsTypeChecked, op.BinaryForm(), 0, xRHS.Node(), nil, rhs.Node(), nil)
				oRHS.SetMType(xRHS.MType())
				oRHS, err := simplify(q.tm, oRHS)
				if err != nil {
					return nil, err
				}
				o := a.NewExpr(a.FlagsTypeChecked, xOp, 0, xLHS.Node(), nil, oRHS.Node(), nil)
				o.SetMType(x.MType())
				return o, nil
			}
			return nil, nil
		}); err != nil {
			return err
		}
	}
	return nil
}

func (q *checker) bcheckAssignment1(lhs *a.Expr, op t.ID, rhs *a.Expr) error {
	switch lhs.MType().Decorator().Key() {
	case t.KeyPtr:
		// TODO: handle.
		return nil
	case t.KeyOpenBracket:
		// TODO: handle.
		return nil
	}

	_, _, err := q.bcheckExpr(lhs, 0)
	if err != nil {
		return err
	}
	lMin, lMax, err := q.bcheckTypeExpr(lhs.MType())
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
		rMin, rMax, err = q.bcheckExprBinaryOp(op.BinaryForm().Key(), lhs, rhs, 0)
	}
	if err != nil {
		return err
	}

	if (rMin != nil && lMin != nil && rMin.Cmp(lMin) < 0) ||
		(rMax != nil && lMax != nil && rMax.Cmp(lMax) > 0) {

		if op == t.IDEq {
			return fmt.Errorf("check: expression %q bounds [%v..%v] is not within bounds [%v..%v]",
				rhs.String(q.tm), rMin, rMax, lMin, lMax)
		} else {
			return fmt.Errorf("check: assignment %q bounds [%v..%v] is not within bounds [%v..%v]",
				lhs.String(q.tm)+" "+op.String(q.tm)+" "+rhs.String(q.tm),
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
		case a.KReturn, a.KJump:
			return true
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
			m[f.String(q.tm)]++
		}
	}

	return q.facts.update(func(n *a.Expr) (*a.Expr, error) {
		if m[n.String(q.tm)] == len(branches) {
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
		if o.Assert().Keyword().Key() == t.KeyPost {
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
			if o.Assert().Keyword().Key() == t.KeyPost {
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
			if o.Assert().Keyword().Key() == t.KeyPost {
				if err := q.bcheckAssert(o.Assert()); err != nil {
					return err
				}
			}
		}
	}

	if cv := n.Condition().ConstValue(); cv != nil && cv.Cmp(zero) == 0 {
		// We effectively have a "while false { etc }" loop. There's no need to
		// check the body.
	} else {
		// Assume the pre and inv conditions...
		q.facts = q.facts[:0]
		for _, o := range n.Asserts() {
			if o.Assert().Keyword().Key() == t.KeyPost {
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
				if o.Assert().Keyword().Key() == t.KeyPost {
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
		if o.Assert().Keyword().Key() == t.KeyPre {
			continue
		}
		q.facts.appendFact(o.Assert().Condition())
	}
	return nil
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
			n.String(q.tm), nMin, nMax, tMin, tMax)
	}
	return nMin, nMax, nil
}

func (q *checker) bcheckExpr1(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	if cv := n.ConstValue(); cv != nil {
		return cv, cv, nil
	}
	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		return q.bcheckExprOther(n, depth)
	case t.FlagsUnaryOp:
		return q.bcheckExprUnaryOp(n, depth)
	case t.FlagsBinaryOp:
		if n.ID0().Key() == t.KeyXBinaryAs {
			return q.bcheckExpr(n.LHS().Expr(), depth)
		}
		return q.bcheckExprBinaryOp(n.ID0().Key(), n.LHS().Expr(), n.RHS().Expr(), depth)
	case t.FlagsAssociativeOp:
		return q.bcheckExprAssociativeOp(n, depth)
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExpr", n.ID0().Key())
}

func (q *checker) bcheckExprOther(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	switch n.ID0().Key() {
	case 0:
		// No-op.

	case t.KeyOpenParen, t.KeyTry:
		_, _, err := q.bcheckExpr(n.LHS().Expr(), depth)
		if err != nil {
			return nil, nil, err
		}

		// TODO: delete this hack that only matches "in.src.read_u8?()" etc.
		if isInSrc(q.tm, n, t.KeyReadU8, 0) ||
			isInSrc(q.tm, n, t.KeyReadU16BE, 0) || isInSrc(q.tm, n, t.KeyReadU32BE, 0) ||
			isInSrc(q.tm, n, t.KeyReadU32BE, 0) || isInSrc(q.tm, n, t.KeyReadU32LE, 0) ||
			isInSrc(q.tm, n, t.KeySkip32, 1) ||
			isInDst(q.tm, n, t.KeyCopyFromSlice, 1) || isInDst(q.tm, n, t.KeyCopyFromSlice32, 2) ||
			isInDst(q.tm, n, t.KeyCopyFromReader32, 2) || isInDst(q.tm, n, t.KeyCopyFromHistory32, 2) ||
			isInDst(q.tm, n, t.KeyWriteU8, 1) || isInDst(q.tm, n, t.KeySlice, 0) ||
			isThisMethod(q.tm, n, "decode_header", 1) || isThisMethod(q.tm, n, "decode_lsd", 1) ||
			isThisMethod(q.tm, n, "decode_extension", 1) || isThisMethod(q.tm, n, "decode_id", 2) ||
			isThisMethod(q.tm, n, "decode_uncompressed", 2) || isThisMethod(q.tm, n, "decode_huffman", 2) ||
			isThisMethod(q.tm, n, "decode_blocks", 2) ||
			isThisMethod(q.tm, n, "init_fixed_huffman", 0) || isThisMethod(q.tm, n, "init_dynamic_huffman", 1) ||
			isThisMethod(q.tm, n, "init_huff", 4) {

			for _, o := range n.Args() {
				a := o.Arg().Value()
				// TODO: check that the arg range at the caller and the
				// signature are compatible.
				aMin, aMax, err := q.bcheckExpr(a, depth)
				if err != nil {
					return nil, nil, err
				}
				// TODO: check that aMin and aMax is within the function's
				// declared arg bounds.
				_, _ = aMin, aMax
			}
			break
		}
		// TODO: delete this hack that only matches "foo.bar_bits(etc)".
		if isThatMethod(q.tm, n, t.KeyLowBits, 1) || isThatMethod(q.tm, n, t.KeyHighBits, 1) {
			a := n.Args()[0].Arg().Value()
			aMin, aMax, err := q.bcheckExpr(a, depth)
			if err != nil {
				return nil, nil, err
			}
			if aMin.Cmp(zero) < 0 {
				// TODO: error, but a better check than aMin < 0 is that
				// a.MType() is u32. Checking this properly should fall out
				// when a *a.TypeExpr can express function types.
			}
			// TODO: sixtyFour should actually be 8 * sizeof(n.LHS().Expr()).
			if aMax.Cmp(sixtyFour) > 0 {
				return nil, nil, fmt.Errorf("check: low_bits argument %q is possibly too large", a.String(q.tm))
			}
			return zero, bitMask(int(aMax.Int64())), nil
		}
		// TODO: delete this hack that only matches "foo.is_suspension(etc)".
		if isThatMethod(q.tm, n, t.KeyIsSuspension, 0) || isThatMethod(q.tm, n, t.KeySuffix, 1) {
			return nil, nil, nil
		}
		// TODO: delete this hack that only matches "foo.set_literal_width(etc)".
		if isThatMethod(q.tm, n, q.tm.ByName("set_literal_width").Key(), 1) {
			a := n.Args()[0].Arg().Value()
			aMin, aMax, err := q.bcheckExpr(a, depth)
			if err != nil {
				return nil, nil, err
			}
			if aMin.Cmp(big.NewInt(2)) < 0 || aMax.Cmp(big.NewInt(8)) > 0 {
				return nil, nil, fmt.Errorf("check: %q not in range [2..8]", a.String(q.tm))
			}
			return nil, nil, nil
		}
		// TODO: delete this hack that only matches "foo.decode(etc)".
		if isThatMethod(q.tm, n, q.tm.ByName("decode").Key(), 2) {
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
		// TODO: delete this hack that only matches "foo.copy_from_slice(etc)".
		if isThatMethod(q.tm, n, t.KeyCopyFromSlice, 1) {
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
		// TODO: delete this hack that only matches "foo.length(etc)".
		if isThatMethod(q.tm, n, t.KeyLength, 0) || isThatMethod(q.tm, n, t.KeyAvailable, 0) {
			break
		}
		return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprOther", n.ID0().Key())

	case t.KeyOpenBracket:
		lhs := n.LHS().Expr()
		_, _, err := q.bcheckExpr(lhs, depth)
		if err != nil {
			return nil, nil, err
		}
		rhs := n.RHS().Expr()
		rMin, rMax, err := q.bcheckExpr(rhs, depth)
		if err != nil {
			return nil, nil, err
		}
		cv := lhs.MType().ArrayLength().ConstValue()
		if cv == nil {
			return nil, nil, fmt.Errorf("check: cannot determine constant array length for %q",
				lhs.MType().String(q.tm))
		}
		if rMin.Cmp(zero) < 0 || rMax.Cmp(cv) >= 0 {
			return nil, nil, fmt.Errorf("check: array index %q (of range %v..%v) out of range",
				rhs.String(q.tm), rMin, rMax)
		}

	case t.KeyColon:
		lhs := n.LHS().Expr()
		mhs := n.MHS().Expr()
		rhs := n.RHS().Expr()
		if mhs == nil && rhs == nil {
			return nil, nil, nil
		}

		lengthExpr := (*a.Expr)(nil)
		if lTyp := lhs.MType(); lTyp.Decorator().Key() == t.KeyOpenBracket {
			// Slice of an array.
			cv := lTyp.ArrayLength().ConstValue()
			id, err := q.tm.Insert(cv.String())
			if err != nil {
				return nil, nil, err
			}
			lengthExpr = a.NewExpr(a.FlagsTypeChecked, 0, id, nil, nil, nil, nil)
			lengthExpr.SetConstValue(cv)
			lengthExpr.SetMType(typeExprIdeal)
		} else {
			// Slice of a slice.
			lengthExpr = a.NewExpr(a.FlagsTypeChecked, t.IDDot, t.IDLength, lhs.Node(), nil, nil, nil)
			lengthExpr.SetMType(typeExprPlaceholder) // HACK.
			lengthExpr = a.NewExpr(a.FlagsTypeChecked, t.IDOpenParen, 0, lengthExpr.Node(), nil, nil, nil)
			lengthExpr.SetMType(typeExprU64)
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

	case t.KeyDot:
		// TODO: delete this hack that only matches "in".
		if n.LHS().Expr().ID1().Key() == t.KeyIn {
			for _, o := range q.f.Func.In().Fields() {
				o := o.Field()
				if o.Name() == n.ID1() {
					return q.bcheckTypeExpr(o.XType())
				}
			}
			lTyp := n.LHS().Expr().MType()
			return nil, nil, fmt.Errorf("check: no field named %q found in struct type %q for expression %q",
				n.ID1().String(q.tm), lTyp.Name().String(q.tm), n.String(q.tm))
		}

		if _, _, err := q.bcheckExpr(n.LHS().Expr(), depth); err != nil {
			return nil, nil, err
		}

	case t.KeyLimit:
		return nil, nil, nil

	default:
		return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprOther", n.ID0().Key())
	}
	return q.bcheckTypeExpr(n.MType())
}

func (q *checker) bcheckExprUnaryOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	rMin, rMax, err := q.bcheckExpr(n.RHS().Expr(), depth)
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

func (q *checker) bcheckExprBinaryOp(op t.Key, lhs *a.Expr, rhs *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	lMin, lMax, err := q.bcheckExpr(lhs, depth)
	if err != nil {
		return nil, nil, err
	}
	return q.bcheckExprBinaryOp1(op, lhs, lMin, lMax, rhs, depth)
}

func (q *checker) bcheckExprBinaryOp1(op t.Key, lhs *a.Expr, lMin *big.Int, lMax *big.Int, rhs *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	rMin, rMax, err := q.bcheckExpr(rhs, depth)
	if err != nil {
		return nil, nil, err
	}

	switch op {
	case t.KeyXBinaryPlus:
		return big.NewInt(0).Add(lMin, rMin), big.NewInt(0).Add(lMax, rMax), nil

	case t.KeyXBinaryMinus:
		nMin := big.NewInt(0).Sub(lMin, rMax)
		nMax := big.NewInt(0).Sub(lMax, rMin)
		for _, x := range q.facts {
			xOp, xLHS, xRHS := parseBinaryOp(x)
			if !lhs.Eq(xLHS) || !rhs.Eq(xRHS) {
				continue
			}
			switch xOp.Key() {
			case t.KeyXBinaryLessThan:
				nMax = min(nMax, minusOne)
			case t.KeyXBinaryLessEq:
				nMax = min(nMax, zero)
			case t.KeyXBinaryGreaterEq:
				nMin = max(nMin, zero)
			case t.KeyXBinaryGreaterThan:
				nMin = max(nMin, one)
			}
		}
		return nMin, nMax, nil

	case t.KeyXBinaryStar:
		// TODO: handle multiplication by negative numbers. Note that this
		// might reverse the inequality: if 0 < a < b but c < 0 then a*c > b*c.
		if lMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: multiply op argument %q is possibly negative", lhs.String(q.tm))
		}
		if rMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: multiply op argument %q is possibly negative", rhs.String(q.tm))
		}
		return big.NewInt(0).Mul(lMin, rMin), big.NewInt(0).Mul(lMax, rMax), nil

	case t.KeyXBinarySlash:
		// TODO.

	case t.KeyXBinaryShiftL:
		if lMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", lhs.String(q.tm))
		}
		if rMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", rhs.String(q.tm))
		}
		if rMax.Cmp(ffff) > 0 {
			return nil, nil, fmt.Errorf("check: shift %q out of range", rhs.String(q.tm))
		}
		return big.NewInt(0).Lsh(lMin, uint(rMin.Uint64())), big.NewInt(0).Lsh(lMax, uint(rMax.Uint64())), nil

	case t.KeyXBinaryShiftR:
		if lMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", lhs.String(q.tm))
		}
		if rMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: shift op argument %q is possibly negative", rhs.String(q.tm))
		}
		if rMin.Cmp(maxIntBits) >= 0 {
			return zero, zero, nil
		}
		nMax := big.NewInt(0).Rsh(lMax, uint(rMin.Uint64()))
		if rMax.Cmp(maxIntBits) >= 0 {
			return zero, nMax, nil
		}
		return big.NewInt(0).Rsh(lMin, uint(rMax.Uint64())), nMax, nil

	case t.KeyXBinaryAmp, t.KeyXBinaryPipe:
		// TODO: should type-checking ensure that bitwise ops only apply to
		// *unsigned* integer types?
		if lMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly negative", lhs.String(q.tm))
		}
		if lMax.Cmp(numTypeBounds[t.KeyU64][1]) > 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly too large", lhs.String(q.tm))
		}
		if rMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly negative", rhs.String(q.tm))
		}
		if rMax.Cmp(numTypeBounds[t.KeyU64][1]) > 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly too large", rhs.String(q.tm))
		}
		z := (*big.Int)(nil)
		if op == t.KeyXBinaryAmp {
			z = min(lMax, rMax)
		} else {
			z = max(lMax, rMax)
		}
		// Return [0, z rounded up to the next power-of-2-minus-1]. This is
		// conservative, but works fine in practice.
		return zero, bitMask(z.BitLen()), nil

	case t.KeyXBinaryAmpHat:
		// TODO.

	case t.KeyXBinaryHat:
		// TODO.

	case t.KeyXBinaryPercent:
		if lMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: modulus op argument %q is possibly negative", lhs.String(q.tm))
		}
		if rMin.Cmp(zero) <= 0 {
			return nil, nil, fmt.Errorf("check: modulus op argument %q is possibly non-positive", rhs.String(q.tm))
		}
		return zero, big.NewInt(0).Sub(rMax, one), nil

	case t.KeyXBinaryNotEq, t.KeyXBinaryLessThan, t.KeyXBinaryLessEq, t.KeyXBinaryEqEq,
		t.KeyXBinaryGreaterEq, t.KeyXBinaryGreaterThan, t.KeyXBinaryAnd, t.KeyXBinaryOr:
		return zero, one, nil

	case t.KeyXBinaryAs:
		// Unreachable, as this is checked by the caller.

	case t.KeyXBinaryTildePlus:
		typ := lhs.MType()
		if typ.IsIdeal() {
			typ = rhs.MType()
		}
		b := numTypeBounds[typ.Name().Key()]
		return b[0], b[1], nil
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprBinaryOp", op)
}

func (q *checker) bcheckExprAssociativeOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	op := n.ID0().AmbiguousForm().BinaryForm().Key()
	if op == 0 {
		return nil, nil, fmt.Errorf(
			"check: unrecognized token.Key (0x%X) for bcheckExprAssociativeOp", n.ID0().Key())
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
		lhs := a.NewExpr(n.Node().Raw().Flags(), n.ID0(), n.ID1(), n.LHS(), n.MHS(), n.RHS(), args[:i])
		lMin, lMax, err = q.bcheckExprBinaryOp1(op, lhs, lMin, lMax, o.Expr(), depth)
		if err != nil {
			return nil, nil, err
		}
	}
	return lMin, lMax, nil
}

func (q *checker) bcheckTypeExpr(n *a.TypeExpr) (*big.Int, *big.Int, error) {
	if n.IsIdeal() {
		// TODO: can an ideal type be refined??
		return nil, nil, nil
	}

	switch n.Decorator().Key() {
	case t.KeyPtr, t.KeyOpenBracket, t.KeyColon:
		return nil, nil, nil
	}
	switch n.Name().Key() {
	case t.KeyReader1, t.KeyWriter1:
		return nil, nil, nil
	}

	b := [2]*big.Int{}
	if key := n.Name().Key(); key < t.Key(len(numTypeBounds)) {
		b = numTypeBounds[key]
	}
	if b[0] == nil || b[1] == nil {
		return nil, nil, nil
	}
	if n.IsRefined() {
		if x := n.Min(); x != nil {
			if cv := x.ConstValue(); cv != nil && b[0].Cmp(cv) < 0 {
				b[0] = cv
			}
		}
		if x := n.Max(); x != nil {
			if cv := x.ConstValue(); cv != nil && b[1].Cmp(cv) > 0 {
				b[1] = cv
			}
		}
	}
	return b[0], b[1], nil
}
