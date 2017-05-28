// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

func (q *checker) tcheckVars(n *a.Node) error {
	q.errFilename, q.errLine = n.Raw().FilenameLine()

	if n.Kind() == a.KVar {
		n := n.Var()
		name := n.Name()
		if _, ok := q.f.LocalVars[name]; ok {
			return fmt.Errorf("check: duplicate var %q", name.String(q.idMap))
		}
		if err := q.tcheckTypeExpr(n.XType(), 0); err != nil {
			return err
		}
		q.f.LocalVars[name] = n.XType()
		return nil
	}
	for _, l := range n.Raw().SubLists() {
		for _, o := range l {
			if err := q.tcheckVars(o); err != nil {
				return err
			}
		}
	}
	return nil
}

func (q *checker) tcheckStatement(n *a.Node) error {
	q.errFilename, q.errLine = n.Raw().FilenameLine()

	switch n.Kind() {
	case a.KAssert:
		if err := q.tcheckAssert(n.Assert()); err != nil {
			return err
		}

	case a.KAssign:
		if err := q.tcheckAssign(n.Assign()); err != nil {
			return err
		}

	case a.KIf:
		for n := n.If(); n != nil; n = n.ElseIf() {
			cond := n.Condition()
			if err := q.tcheckExpr(cond, 0); err != nil {
				return err
			}
			if !cond.MType().Eq(TypeExprBoolean) {
				return fmt.Errorf("check: if condition %q, of type %q, does not have a boolean type",
					cond.String(q.idMap), cond.MType().String(q.idMap))
			}
			for _, o := range n.BodyIfTrue() {
				if err := q.tcheckStatement(o); err != nil {
					return err
				}
			}
			for _, o := range n.BodyIfFalse() {
				if err := q.tcheckStatement(o); err != nil {
					return err
				}
			}
		}

	case a.KJump:
		// TODO: check that we're in a for loop.

	case a.KReturn:
		n := n.Return()
		if value := n.Value(); value != nil {
			if err := q.tcheckExpr(value, 0); err != nil {
				return err
			}
			// TODO: type-check that value is assignable to the return value.
			// This needs the context of what func we're in.
		}

	case a.KVar:
		n := n.Var()
		if !n.XType().Node().TypeChecked() {
			return fmt.Errorf("check: internal error: unchecked type expression %q", n.XType().String(q.idMap))
		}
		if value := n.Value(); value != nil {
			if err := q.tcheckExpr(value, 0); err != nil {
				return err
			}
			// TODO: check that value.Type is assignable to n.TypeExpr().
		}

	case a.KWhile:
		n := n.While()
		cond := n.Condition()
		if err := q.tcheckExpr(cond, 0); err != nil {
			return err
		}
		if !cond.MType().Eq(TypeExprBoolean) {
			return fmt.Errorf("check: for-loop condition %q, of type %q, does not have a boolean type",
				cond.String(q.idMap), cond.MType().String(q.idMap))
		}
		for _, o := range n.Asserts() {
			if err := q.tcheckAssert(o.Assert()); err != nil {
				return err
			}
			o.SetTypeChecked()
		}
		for _, o := range n.Body() {
			if err := q.tcheckStatement(o); err != nil {
				return err
			}
		}

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%s) for checkStatement", n.Kind())
	}

	n.SetTypeChecked()
	return nil
}

func (q *checker) tcheckAssert(n *a.Assert) error {
	cond := n.Condition()
	if err := q.tcheckExpr(cond, 0); err != nil {
		return err
	}
	if !cond.MType().Eq(TypeExprBoolean) {
		return fmt.Errorf("check: assert condition %q, of type %q, does not have a boolean type",
			cond.String(q.idMap), cond.MType().String(q.idMap))
	}
	for _, o := range n.Args() {
		if err := q.tcheckExpr(o.Arg().Value(), 0); err != nil {
			return err
		}
		o.SetTypeChecked()
	}
	// TODO: check that there are no side effects.
	return nil
}

func (q *checker) tcheckAssign(n *a.Assign) error {
	lhs := n.LHS()
	rhs := n.RHS()
	if err := q.tcheckExpr(lhs, 0); err != nil {
		return err
	}
	if err := q.tcheckExpr(rhs, 0); err != nil {
		return err
	}
	lTyp := lhs.MType()
	rTyp := rhs.MType()

	if n.Operator().Key() == t.KeyEq {
		if (rTyp == TypeExprIdealNumber && lTyp.IsNumType()) || lTyp.EqIgnoringRefinements(rTyp) {
			return nil
		}
		return fmt.Errorf("check: cannot assign %q of type %q to %q of type %q",
			rhs.String(q.idMap), rTyp.String(q.idMap), lhs.String(q.idMap), lTyp.String(q.idMap))
	}

	if !lTyp.IsNumType() {
		return fmt.Errorf("check: assignment %q: assignee %q, of type %q, does not have numeric type",
			n.Operator().String(q.idMap), lhs.String(q.idMap), lTyp.String(q.idMap))
	}

	switch n.Operator().Key() {
	case t.KeyShiftLEq, t.KeyShiftREq:
		if rTyp.IsNumType() {
			return nil
		}
		return fmt.Errorf("check: assignment %q: shift %q, of type %q, does not have numeric type",
			n.Operator().String(q.idMap), rhs.String(q.idMap), rTyp.String(q.idMap))
	}

	if rTyp == TypeExprIdealNumber || lTyp.EqIgnoringRefinements(rTyp) {
		return nil
	}
	return fmt.Errorf("check: assignment %q: %q and %q, of types %q and %q, do not have compatible types",
		n.Operator().String(q.idMap),
		lhs.String(q.idMap), rhs.String(q.idMap),
		lTyp.String(q.idMap), rTyp.String(q.idMap),
	)
}

func (q *checker) tcheckExpr(n *a.Expr, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("check: expression recursion depth too large")
	}
	depth++

	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		if err := q.tcheckExprOther(n, depth); err != nil {
			return err
		}
	case t.FlagsUnaryOp:
		if err := q.tcheckExprUnaryOp(n, depth); err != nil {
			return err
		}
	case t.FlagsBinaryOp:
		if err := q.tcheckExprBinaryOp(n, depth); err != nil {
			return err
		}
	case t.FlagsAssociativeOp:
		if err := q.tcheckExprAssociativeOp(n, depth); err != nil {
			return err
		}
	default:
		return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExpr", n.ID0().Key())
	}
	n.Node().SetTypeChecked()
	return nil
}

func (q *checker) tcheckExprOther(n *a.Expr, depth uint32) error {
	switch n.ID0().Key() {
	case 0:
		id1 := n.ID1()
		if id1.IsNumLiteral() {
			z := big.NewInt(0)
			s := id1.String(q.idMap)
			if _, ok := z.SetString(s, 0); !ok {
				return fmt.Errorf("check: invalid numeric literal %q", s)
			}
			n.SetConstValue(z)
			n.SetMType(TypeExprIdealNumber)
			return nil

		} else if id1.IsIdent() {
			if q.f.LocalVars != nil {
				if typ, ok := q.f.LocalVars[id1]; ok {
					n.SetMType(typ)
					return nil
				}
			}
			// TODO: look for (global) names (constants, funcs, structs).
			return fmt.Errorf("check: unrecognized identifier %q", id1.String(q.idMap))
		}
		switch id1.Key() {
		case t.KeyFalse:
			n.SetConstValue(zero)
			n.SetMType(TypeExprBoolean)
			return nil

		case t.KeyTrue:
			n.SetConstValue(one)
			n.SetMType(TypeExprBoolean)
			return nil

		case t.KeyUnderscore:
			// TODO.

		case t.KeyThis:
			// TODO.
		}

	case t.KeyOpenParen:
		// n is a function call.
		// TODO.

	case t.KeyOpenBracket:
		// n is an index.
		// TODO.

	case t.KeyColon:
		// n is a slice.
		// TODO.

	case t.KeyDot:
		return q.tcheckDot(n, depth)
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprOther", n.ID0().Key())
}

func (q *checker) tcheckDot(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := q.tcheckExpr(lhs, depth); err != nil {
		return err
	}
	lTyp := lhs.MType()
	for ; lTyp.PackageOrDecorator().Key() == t.KeyPtr; lTyp = lTyp.Inner() {
	}

	if lTyp.PackageOrDecorator() != 0 {
		// TODO.
		return fmt.Errorf("check: unsupported package-or-decorator for checkDot")
	}

	s := (*a.Struct)(nil)
	if q.f.Func != nil {
		switch name := lTyp.Name(); name.Key() {
		case t.KeyIn:
			s = q.f.Func.In()
		case t.KeyOut:
			s = q.f.Func.Out()
		default:
			s = q.c.structs[name].Struct
		}
	}
	if s == nil {
		return fmt.Errorf("check: no struct type %q found for expression %q",
			lTyp.Name().String(q.idMap), lhs.String(q.idMap))
	}

	for _, field := range s.Fields() {
		f := field.Field()
		if f.Name() == n.ID1() {
			n.SetMType(f.XType())
			return nil
		}
	}
	return fmt.Errorf("check: no field named %q found in struct type %q for expression %q",
		n.ID1().String(q.idMap), lTyp.Name().String(q.idMap), n.String(q.idMap))
}

func (q *checker) tcheckExprUnaryOp(n *a.Expr, depth uint32) error {
	rhs := n.RHS().Expr()
	if err := q.tcheckExpr(rhs, depth); err != nil {
		return err
	}
	rTyp := rhs.MType()

	switch n.ID0().Key() {
	case t.KeyXUnaryPlus, t.KeyXUnaryMinus:
		if !numeric(rTyp) {
			return fmt.Errorf("check: unary %q: %q, of type %q, does not have a numeric type",
				n.ID0().AmbiguousForm().String(q.idMap), rhs.String(q.idMap), rTyp.String(q.idMap))
		}
		if cv := rhs.ConstValue(); cv != nil {
			if n.ID0().Key() == t.KeyXUnaryMinus {
				cv = neg(cv)
			}
			n.SetConstValue(cv)
		}
		n.SetMType(rTyp)
		return nil

	case t.KeyXUnaryNot:
		if !rTyp.Eq(TypeExprBoolean) {
			return fmt.Errorf("check: unary %q: %q, of type %q, does not have a boolean type",
				n.ID0().AmbiguousForm().String(q.idMap), rhs.String(q.idMap), rTyp.String(q.idMap))
		}
		if cv := rhs.ConstValue(); cv != nil {
			n.SetConstValue(btoi(cv.Cmp(zero) == 0))
		}
		n.SetMType(TypeExprBoolean)
		return nil
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprUnaryOp", n.ID0().Key())
}

func (q *checker) tcheckExprBinaryOp(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := q.tcheckExpr(lhs, depth); err != nil {
		return err
	}
	lTyp := lhs.MType()
	op := n.ID0()
	if op.Key() == t.KeyXBinaryAs {
		rhs := n.RHS().TypeExpr()
		if err := q.tcheckTypeExpr(rhs, 0); err != nil {
			return err
		}
		if numeric(lTyp) && rhs.IsNumType() {
			n.SetMType(rhs)
			return nil
		}
		return fmt.Errorf("check: cannot convert expression %q, of type %q, as type %q",
			lhs.String(q.idMap), lTyp.String(q.idMap), rhs.String(q.idMap))
	}
	rhs := n.RHS().Expr()
	if err := q.tcheckExpr(rhs, depth); err != nil {
		return err
	}
	rTyp := rhs.MType()

	switch op.Key() {
	default:
		if !numeric(lTyp) {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				op.AmbiguousForm().String(q.idMap), lhs.String(q.idMap), lTyp.String(q.idMap))
		}
		if !numeric(rTyp) {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				op.AmbiguousForm().String(q.idMap), rhs.String(q.idMap), rTyp.String(q.idMap))
		}
	case t.KeyXBinaryNotEq, t.KeyXBinaryEqEq:
		// No-op.
	case t.KeyXBinaryAnd, t.KeyXBinaryOr:
		if lTyp != TypeExprBoolean {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().String(q.idMap), lhs.String(q.idMap), lTyp.String(q.idMap))
		}
		if rTyp != TypeExprBoolean {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().String(q.idMap), rhs.String(q.idMap), rTyp.String(q.idMap))
		}
	}

	switch op.Key() {
	default:
		if !lTyp.EqIgnoringRefinements(rTyp) && lTyp != TypeExprIdealNumber && rTyp != TypeExprIdealNumber {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q, do not have compatible types",
				op.AmbiguousForm().String(q.idMap),
				lhs.String(q.idMap), rhs.String(q.idMap),
				lTyp.String(q.idMap), rTyp.String(q.idMap),
			)
		}
	case t.KeyXBinaryShiftL, t.KeyXBinaryShiftR:
		if (lTyp == TypeExprIdealNumber) && (rTyp != TypeExprIdealNumber) {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q; "+
				"cannot shift an ideal number by a non-ideal number",
				op.AmbiguousForm().String(q.idMap),
				lhs.String(q.idMap), rhs.String(q.idMap),
				lTyp.String(q.idMap), rTyp.String(q.idMap),
			)
		}
	}

	if l, r := lhs.ConstValue(), rhs.ConstValue(); l != nil && r != nil {
		if err := q.setConstValueBinaryOp(n, l, r); err != nil {
			return err
		}
	}

	if comparisonOps[0xFF&op.Key()] {
		n.SetMType(TypeExprBoolean)
	} else if lTyp != TypeExprIdealNumber {
		n.SetMType(lTyp)
	} else {
		n.SetMType(rTyp)
	}

	return nil
}

func (q *checker) setConstValueBinaryOp(n *a.Expr, l *big.Int, r *big.Int) error {
	switch n.ID0().Key() {
	case t.KeyXBinaryPlus:
		n.SetConstValue(big.NewInt(0).Add(l, r))
	case t.KeyXBinaryMinus:
		n.SetConstValue(big.NewInt(0).Sub(l, r))
	case t.KeyXBinaryStar:
		n.SetConstValue(big.NewInt(0).Mul(l, r))
	case t.KeyXBinarySlash:
		if r.Cmp(zero) == 0 {
			return fmt.Errorf("check: division by zero in const expression %q", n.String(q.idMap))
		}
		// TODO: decide on Euclidean division vs other definitions. See "go doc
		// math/big int.divmod" for details.
		n.SetConstValue(big.NewInt(0).Div(l, r))
	case t.KeyXBinaryShiftL:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().String(q.idMap), n.String(q.idMap))
		}
		n.SetConstValue(big.NewInt(0).Lsh(l, uint(r.Uint64())))
	case t.KeyXBinaryShiftR:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().String(q.idMap), n.String(q.idMap))
		}
		n.SetConstValue(big.NewInt(0).Rsh(l, uint(r.Uint64())))
	case t.KeyXBinaryAmp:
		n.SetConstValue(big.NewInt(0).And(l, r))
	case t.KeyXBinaryAmpHat:
		n.SetConstValue(big.NewInt(0).AndNot(l, r))
	case t.KeyXBinaryPipe:
		n.SetConstValue(big.NewInt(0).Or(l, r))
	case t.KeyXBinaryHat:
		n.SetConstValue(big.NewInt(0).Xor(l, r))
	case t.KeyXBinaryNotEq:
		n.SetConstValue(btoi(l.Cmp(r) != 0))
	case t.KeyXBinaryLessThan:
		n.SetConstValue(btoi(l.Cmp(r) < 0))
	case t.KeyXBinaryLessEq:
		n.SetConstValue(btoi(l.Cmp(r) <= 0))
	case t.KeyXBinaryEqEq:
		n.SetConstValue(btoi(l.Cmp(r) == 0))
	case t.KeyXBinaryGreaterEq:
		n.SetConstValue(btoi(l.Cmp(r) >= 0))
	case t.KeyXBinaryGreaterThan:
		n.SetConstValue(btoi(l.Cmp(r) > 0))
	case t.KeyXBinaryAnd:
		n.SetConstValue(btoi((l.Cmp(zero) != 0) && (r.Cmp(zero) != 0)))
	case t.KeyXBinaryOr:
		n.SetConstValue(btoi((l.Cmp(zero) != 0) || (r.Cmp(zero) != 0)))
	}
	return nil
}

func (q *checker) tcheckExprAssociativeOp(n *a.Expr, depth uint32) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprAssociativeOp", n.ID0().Key())
}

func (q *checker) tcheckTypeExpr(n *a.TypeExpr, depth uint32) error {
	if depth > a.MaxTypeExprDepth {
		return fmt.Errorf("check: type expression recursion depth too large")
	}
	depth++

	switch n.PackageOrDecorator().Key() {
	case 0:
		if name := n.Name(); name.IsNumType() || name.Key() == t.KeyBool {
			for _, b := range n.Bounds() {
				if b == nil {
					continue
				}
				if err := q.tcheckExpr(b, 0); err != nil {
					return err
				}
				if b.ConstValue() == nil {
					return fmt.Errorf("check: %q is not constant", b.String(q.idMap))
				}
			}
			break
		}
		if n.Min() != nil || n.Max() != nil {
			// TODO: reject.
		}
		// TODO: see if name refers to a struct type.
		return fmt.Errorf("check: %q is not a type", n.Name().String(q.idMap))

	case t.KeyPtr:
		if err := q.tcheckTypeExpr(n.Inner(), depth); err != nil {
			return err
		}

	case t.KeyOpenBracket:
		aLen := n.ArrayLength()
		if err := q.tcheckExpr(aLen, 0); err != nil {
			return err
		}
		if aLen.ConstValue() == nil {
			return fmt.Errorf("check: %q is not constant", aLen.String(q.idMap))
		}
		if err := q.tcheckTypeExpr(n.Inner(), depth); err != nil {
			return err
		}

	default:
		// TODO: delete this hack.
		if n.PackageOrDecorator() == q.idMap.ByName("io") && n.Name() == q.idMap.ByName("buf1") {
			break
		}

		return fmt.Errorf("check: unrecognized node for checkTypeExpr")
	}
	n.Node().SetTypeChecked()
	return nil
}

var comparisonOps = [256]bool{
	t.KeyXBinaryNotEq:       true,
	t.KeyXBinaryLessThan:    true,
	t.KeyXBinaryLessEq:      true,
	t.KeyXBinaryEqEq:        true,
	t.KeyXBinaryGreaterEq:   true,
	t.KeyXBinaryGreaterThan: true,
}
