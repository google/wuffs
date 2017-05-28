// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

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
}

var (
	zero = big.NewInt(0)
	one  = big.NewInt(1)
	ffff = big.NewInt(0xFFFF)
)

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

func invert(m *t.IDMap, n *a.Expr) (*a.Expr, error) {
	if !n.MType().IsBool() {
		return nil, fmt.Errorf("check: invert(%q) called on non-bool-typed expression", n.String(m))
	}
	if cv := n.ConstValue(); cv != nil {
		return nil, fmt.Errorf("check: invert(%q) called on constant expression", n.String(m))
	}
	id0, lhs, rhs := n.ID0(), n.LHS().Expr(), n.RHS().Expr()
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
	default:
		// TODO: De Morgan's law for t.IDXBinaryAnd, t.IDXBinaryOr?
		id0, lhs, rhs = t.IDXUnaryNot, nil, n
	}
	o := a.NewExpr(n.Node().Raw().Flags(), id0, 0, lhs.Node(), nil, rhs.Node(), nil)
	o.SetMType(n.MType())
	return o, nil
}

func bcheckField(idMap *t.IDMap, n *a.Field) error {
	x := n.XType()
	for ; x.Inner() != nil; x = x.Inner() {
	}
	dv, nMin, nMax := zero, zero, zero
	if o := n.DefaultValue(); o != nil {
		dv = o.ConstValue()
	}
	if o := x.Min(); o != nil {
		nMin = o.ConstValue()
	}
	if o := x.Max(); o != nil {
		nMax = o.ConstValue()
	}
	if dv.Cmp(nMin) < 0 || dv.Cmp(nMax) > 0 {
		return fmt.Errorf("check: default value %v is not within bounds [%v..%v] for field %q",
			dv, nMin, nMax, n.Name().String(idMap))
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
		return q.bcheckAssignment(n.LHS(), n.LHS().MType(), n.Operator(), n.RHS())

	case a.KIf:
		// TODO.

	case a.KJump:
		// No-op.

	case a.KReturn:
		// TODO.

	case a.KVar:
		n := n.Var()
		return q.bcheckAssignment(nil, n.XType(), t.IDEq, n.Value())

	case a.KWhile:
		return q.bcheckWhile(n.While())

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%s) for checkStatement", n.Kind())
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
			err = fmt.Errorf("no such reason %s", reasonID.String(q.idMap))
		}
	} else if condition.ID0().IsBinaryOp() && condition.ID0().Key() != t.KeyAs {
		if q.proveBinaryOp(condition.ID0().Key(), condition.LHS().Expr(), condition.RHS().Expr()) {
			err = nil
		}
	}
	if err != nil {
		if err == errFailed {
			return fmt.Errorf("check: cannot prove %q", condition.String(q.idMap))
		}
		return fmt.Errorf("check: cannot prove %q: %v", condition.String(q.idMap), err)
	}
	q.facts.appendFact(condition)
	return nil
}

func (q *checker) bcheckAssignment(lhs *a.Expr, lTyp *a.TypeExpr, op t.ID, rhs *a.Expr) error {
	if err := q.bcheckAssignment1(lhs, lTyp, op, rhs); err != nil {
		return err
	}
	// TODO: drop any facts involving lhs.
	return nil
}

func (q *checker) bcheckAssignment1(lhs *a.Expr, lTyp *a.TypeExpr, op t.ID, rhs *a.Expr) error {
	switch lTyp.PackageOrDecorator().Key() {
	case t.KeyPtr:
		// TODO: handle.
		return nil
	case t.KeyOpenBracket:
		// TODO: handle.
		return nil
	}
	if lTyp.Name().Key() == t.KeyBool {
		return nil
	}

	lMin, lMax, err := q.bcheckTypeExpr(lTyp)
	if err != nil {
		return err
	}

	rMin, rMax := (*big.Int)(nil), (*big.Int)(nil)
	if op == t.IDEq {
		cv := zero
		// rhs might be nil because "var x T" has an implicit "= 0".
		if rhs != nil {
			cv = rhs.ConstValue()
		}
		if cv != nil {
			if cv.Cmp(lMin) < 0 || cv.Cmp(lMax) > 0 {
				return fmt.Errorf("check: constant %v is not within bounds [%v..%v]", cv, lMin, lMax)
			}
			return nil
		}
		rMin, rMax, err = q.bcheckExpr(rhs, 0)
	} else {
		rMin, rMax, err = q.bcheckExprBinaryOp(lhs, op.BinaryForm().Key(), rhs, 0)
	}
	if err != nil {
		return err
	}
	if rMin.Cmp(lMin) < 0 || rMax.Cmp(lMax) > 0 {
		if op == t.IDEq {
			return fmt.Errorf("check: expression %q bounds [%v..%v] is not within bounds [%v..%v]",
				rhs.String(q.idMap), rMin, rMax, lMin, lMax)
		} else {
			return fmt.Errorf("check: assignment %q bounds [%v..%v] is not within bounds [%v..%v]",
				lhs.String(q.idMap)+" "+op.String(q.idMap)+" "+rhs.String(q.idMap),
				rMin, rMax, lMin, lMax)
		}
	}
	return nil
}

func (q *checker) bcheckWhile(n *a.While) error {
	// TODO: check that n.Condition() has no side effects.

	if _, _, err := q.bcheckExpr(n.Condition(), 0); err != nil {
		return err
	}

	// Check the pre and assert conditions on entry.
	for _, o := range n.Asserts() {
		if o.Assert().Keyword().Key() == t.KeyPost {
			continue
		}
		// TODO
	}

	// Check the post conditions on exit, assuming only the pre and assert
	// conditions and the inverted while condition.
	//
	// We don't need to check the assert conditions, even though we add them to
	// the facts after the while loop, since we have already proven each assert
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
		if inverse, err := invert(q.idMap, n.Condition()); err != nil {
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
		// Assume the pre and assert conditions, and the while condition. Check
		// the body.
		q.facts = q.facts[:0]
		for _, o := range n.Asserts() {
			if o.Assert().Keyword().Key() == t.KeyPost {
				continue
			}
			q.facts.appendFact(o.Assert().Condition())
		}
		q.facts.appendFact(n.Condition())
		for _, o := range n.Body() {
			if err := q.bcheckStatement(o); err != nil {
				return err
			}
		}
		// Check the pre and assert conditions on the implicit continue after the
		// body.
		for _, o := range n.Asserts() {
			if o.Assert().Keyword().Key() == t.KeyPost {
				continue
			}
			// TODO
		}

		// TODO: check the pre and assert conditions for each continue.

		// TODO: check the assert and post conditions for each break.
	}

	// Assume the assert and post conditions.
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
			lMin, lMax, err := q.bcheckExpr(n.LHS().Expr(), depth)
			if err != nil {
				return nil, nil, err
			}
			// TODO.
			_, _ = lMin, lMax
			return zero, zero, nil
		}
		return q.bcheckExprBinaryOp(n.LHS().Expr(), n.ID0().Key(), n.RHS().Expr(), depth)
	case t.FlagsAssociativeOp:
		return q.bcheckExprAssociativeOp(n, depth)
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExpr", n.ID0().Key())
}

func (q *checker) bcheckExprOther(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	switch n.ID0().Key() {
	case 0:
		// No-op.

	case t.KeyOpenParen:
		// TODO.
		return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprOther", n.ID0().Key())

	case t.KeyOpenBracket:
		// TODO.
		return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprOther", n.ID0().Key())

	case t.KeyColon:
		// TODO.
		return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprOther", n.ID0().Key())

	case t.KeyDot:
		if _, _, err := q.bcheckExpr(n.LHS().Expr(), depth); err != nil {
			return nil, nil, err
		}

	default:
		return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprOther", n.ID0().Key())
	}

	nMin, nMax, err := q.bcheckTypeExpr(n.MType())
	if err != nil {
		return nil, nil, err
	}
	nMin, nMax = q.facts.refine(n, nMin, nMax)
	return nMin, nMax, nil
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

func (q *checker) bcheckExprBinaryOp(lhs *a.Expr, op t.Key, rhs *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	lMin, lMax, err := q.bcheckExpr(lhs, depth)
	if err != nil {
		return nil, nil, err
	}
	rMin, rMax, err := q.bcheckExpr(rhs, depth)
	if err != nil {
		return nil, nil, err
	}

	switch op {
	case t.KeyXBinaryPlus:
		return big.NewInt(0).Add(lMin, rMin), big.NewInt(0).Add(lMax, rMax), nil
	case t.KeyXBinaryMinus:
		return big.NewInt(0).Sub(lMin, rMin), big.NewInt(0).Sub(lMax, rMax), nil
	case t.KeyXBinaryStar:
		// TODO.
		if cv := lhs.ConstValue(); cv != nil && cv.Cmp(zero) == 0 {
			return zero, zero, nil
		}
		if cv := rhs.ConstValue(); cv != nil && cv.Cmp(zero) == 0 {
			return zero, zero, nil
		}
	case t.KeyXBinarySlash:
	case t.KeyXBinaryShiftL:
	case t.KeyXBinaryShiftR:
	case t.KeyXBinaryAmp, t.KeyXBinaryPipe:
		// TODO: should type-checking ensure that bitwise ops only apply to
		// *unsigned* integer types?
		if lMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly negative", lhs.String(q.idMap))
		}
		if lMax.Cmp(numTypeBounds[t.KeyU64][1]) > 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly too large", lhs.String(q.idMap))
		}
		if rMin.Cmp(zero) < 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly negative", rhs.String(q.idMap))
		}
		if rMax.Cmp(numTypeBounds[t.KeyU64][1]) > 0 {
			return nil, nil, fmt.Errorf("check: bitwise op argument %q is possibly too large", rhs.String(q.idMap))
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
	case t.KeyXBinaryHat:

	case t.KeyXBinaryNotEq, t.KeyXBinaryLessThan, t.KeyXBinaryLessEq, t.KeyXBinaryEqEq,
		t.KeyXBinaryGreaterEq, t.KeyXBinaryGreaterThan, t.KeyXBinaryAnd, t.KeyXBinaryOr:
		return zero, one, nil

	case t.KeyXBinaryAs:
	}
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprBinaryOp", op)
}

func (q *checker) bcheckExprAssociativeOp(n *a.Expr, depth uint32) (*big.Int, *big.Int, error) {
	return nil, nil, fmt.Errorf("check: unrecognized token.Key (0x%X) for bcheckExprAssociativeOp", n.ID0().Key())
}

func (q *checker) bcheckTypeExpr(n *a.TypeExpr) (*big.Int, *big.Int, error) {
	switch n.PackageOrDecorator().Key() {
	case t.KeyPtr, t.KeyOpenBracket:
		return nil, nil, nil
	}

	b := [2]*big.Int{}
	if key := n.Name().Key(); key < t.Key(len(numTypeBounds)) {
		b = numTypeBounds[key]
	}
	if b[0] == nil || b[1] == nil {
		return nil, nil, fmt.Errorf("check: internal error: unknown bounds for %q", n.String(q.idMap))
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
