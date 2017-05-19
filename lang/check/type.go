// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

type typeChecker struct {
	c     *Checker
	idMap *t.IDMap
	f     Func

	errFilename string
	errLine     uint32
}

func (c *typeChecker) checkVars(n *a.Node) error {
	c.errFilename, c.errLine = n.Raw().FilenameLine()

	if n.Kind() == a.KVar {
		v := n.Var()
		name := v.Name()
		if _, ok := c.f.LocalVars[name]; ok {
			return fmt.Errorf("check: duplicate var %q", c.idMap.ByID(name))
		}
		if err := c.checkTypeExpr(v.XType(), 0); err != nil {
			return err
		}
		c.f.LocalVars[name] = v.XType()
		return nil
	}
	for _, l := range n.Raw().SubLists() {
		for _, m := range l {
			if err := c.checkVars(m); err != nil {
				return err
			}
		}
	}
	return nil
}

func (c *typeChecker) checkStatement(n *a.Node) error {
	c.errFilename, c.errLine = n.Raw().FilenameLine()

	switch n.Kind() {
	case a.KAssert:
		if err := c.checkAssert(n.Assert()); err != nil {
			return err
		}

	case a.KAssign:
		if err := c.checkAssign(n.Assign()); err != nil {
			return err
		}

	case a.KVar:
		o := n.Var()
		if !o.XType().Node().TypeChecked() {
			return fmt.Errorf("check: internal error: unchecked type expression %q", o.XType().String(c.idMap))
		}
		if value := o.Value(); value != nil {
			if err := c.checkExpr(value, 0); err != nil {
				return err
			}
			// TODO: check that value.Type is assignable to o.TypeExpr().
		}

	case a.KWhile:
		o := n.While()
		cond := o.Condition()
		if err := c.checkExpr(cond, 0); err != nil {
			return err
		}
		if !cond.MType().Eq(TypeExprBoolean) {
			return fmt.Errorf("check: for-loop condition %q, of type %q, does not have a boolean type",
				cond.String(c.idMap), cond.MType().String(c.idMap))
		}
		for _, m := range o.Asserts() {
			if err := c.checkAssert(m.Assert()); err != nil {
				return err
			}
			m.SetTypeChecked()
		}
		for _, m := range o.Body() {
			if err := c.checkStatement(m); err != nil {
				return err
			}
		}

	case a.KIf:
		for o := n.If(); o != nil; o = o.ElseIf() {
			cond := o.Condition()
			if err := c.checkExpr(cond, 0); err != nil {
				return err
			}
			if !cond.MType().Eq(TypeExprBoolean) {
				return fmt.Errorf("check: if condition %q, of type %q, does not have a boolean type",
					cond.String(c.idMap), cond.MType().String(c.idMap))
			}
			for _, m := range o.BodyIfTrue() {
				if err := c.checkStatement(m); err != nil {
					return err
				}
			}
			for _, m := range o.BodyIfFalse() {
				if err := c.checkStatement(m); err != nil {
					return err
				}
			}
		}

	case a.KReturn:
		o := n.Return()
		if value := o.Value(); value != nil {
			if err := c.checkExpr(value, 0); err != nil {
				return err
			}
			// TODO: type-check that value is assignable to the return value.
			// This needs the context of what func we're in.
		}

	case a.KJump:
		// TODO: check that we're in a for loop.

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%d) for checkStatement", n.Kind())
	}

	n.SetTypeChecked()
	return nil
}

func (c *typeChecker) checkAssert(n *a.Assert) error {
	cond := n.Condition()
	if err := c.checkExpr(cond, 0); err != nil {
		return err
	}
	if !cond.MType().Eq(TypeExprBoolean) {
		return fmt.Errorf("check: assert condition %q, of type %q, does not have a boolean type",
			cond.String(c.idMap), cond.MType().String(c.idMap))
	}
	// TODO: check that there are no side effects.
	// TODO: check the actual assertion.
	return nil
}

func (c *typeChecker) checkAssign(n *a.Assign) error {
	lhs := n.LHS()
	rhs := n.RHS()
	if err := c.checkExpr(lhs, 0); err != nil {
		return err
	}
	if err := c.checkExpr(rhs, 0); err != nil {
		return err
	}
	lTyp := lhs.MType()
	rTyp := rhs.MType()

	if n.Operator().Key() == t.KeyEq {
		if (rTyp == TypeExprIdealNumber && lTyp.IsNumType()) || lTyp.EqIgnoringRefinements(rTyp) {
			return nil
		}
		return fmt.Errorf("check: cannot assign %q of type %q to %q of type %q",
			rhs.String(c.idMap), rTyp.String(c.idMap), lhs.String(c.idMap), lTyp.String(c.idMap))
	}

	if !lTyp.IsNumType() {
		return fmt.Errorf("check: assignment %q: assignee %q, of type %q, does not have numeric type",
			c.idMap.ByID(n.Operator()), lhs.String(c.idMap), lTyp.String(c.idMap))
	}

	switch n.Operator().Key() {
	case t.KeyShiftLEq, t.KeyShiftREq:
		if rTyp.IsNumType() {
			return nil
		}
		return fmt.Errorf("check: assignment %q: shift %q, of type %q, does not have numeric type",
			c.idMap.ByID(n.Operator()), rhs.String(c.idMap), rTyp.String(c.idMap))
	}

	if rTyp == TypeExprIdealNumber || lTyp.EqIgnoringRefinements(rTyp) {
		return nil
	}
	return fmt.Errorf("check: assignment %q: %q and %q, of types %q and %q, do not have compatible types",
		c.idMap.ByID(n.Operator()),
		lhs.String(c.idMap), rhs.String(c.idMap),
		lTyp.String(c.idMap), rTyp.String(c.idMap),
	)
}

func (c *typeChecker) checkExpr(n *a.Expr, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("check: expression recursion depth too large")
	}
	depth++

	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		if err := c.checkExprOther(n, depth); err != nil {
			return err
		}
	case t.FlagsUnaryOp:
		if err := c.checkExprUnaryOp(n, depth); err != nil {
			return err
		}
	case t.FlagsBinaryOp:
		if err := c.checkExprBinaryOp(n, depth); err != nil {
			return err
		}
	case t.FlagsAssociativeOp:
		if err := c.checkExprAssociativeOp(n, depth); err != nil {
			return err
		}
	default:
		return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExpr", n.ID0().Key())
	}
	n.Node().SetTypeChecked()
	return nil
}

func (c *typeChecker) checkExprOther(n *a.Expr, depth uint32) error {
	switch n.ID0().Key() {
	case 0:
		id1 := n.ID1()
		if id1.IsNumLiteral() {
			z := big.NewInt(0)
			s := c.idMap.ByID(id1)
			if _, ok := z.SetString(s, 0); !ok {
				return fmt.Errorf("check: invalid numeric literal %q", s)
			}
			n.SetConstValue(z)
			n.SetMType(TypeExprIdealNumber)
			return nil

		} else if id1.IsIdent() {
			if c.f.LocalVars != nil {
				if typ, ok := c.f.LocalVars[id1]; ok {
					n.SetMType(typ)
					return nil
				}
			}
			// TODO: look for (global) names (constants, funcs, structs).
			return fmt.Errorf("check: unrecognized identifier %q", c.idMap.ByID(id1))
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
		return c.checkDot(n, depth)
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprOther", n.ID0().Key())
}

func (c *typeChecker) checkDot(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := c.checkExpr(lhs, depth); err != nil {
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
	if c.f.Func != nil {
		switch name := lTyp.Name(); name.Key() {
		case t.KeyIn:
			s = c.f.Func.In()
		case t.KeyOut:
			s = c.f.Func.Out()
		default:
			s = c.c.structs[name].Struct
		}
	}
	if s == nil {
		return fmt.Errorf("check: no struct type %q found for expression %q",
			lTyp.Name().String(c.idMap), lhs.String(c.idMap))
	}

	for _, field := range s.Fields() {
		f := field.Field()
		if f.Name() == n.ID1() {
			n.SetMType(f.XType())
			return nil
		}
	}
	return fmt.Errorf("check: no field named %q found in struct type %q for expression %q",
		n.ID1().String(c.idMap), lTyp.Name().String(c.idMap), n.String(c.idMap))
}

func (c *typeChecker) checkExprUnaryOp(n *a.Expr, depth uint32) error {
	rhs := n.RHS().Expr()
	if err := c.checkExpr(rhs, depth); err != nil {
		return err
	}
	rTyp := rhs.MType()

	switch n.ID0().Key() {
	case t.KeyXUnaryPlus, t.KeyXUnaryMinus:
		if !numeric(rTyp) {
			return fmt.Errorf("check: unary %q: %q, of type %q, does not have a numeric type",
				c.idMap.ByID(n.ID0().AmbiguousForm()), rhs.String(c.idMap), rTyp.String(c.idMap))
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
				c.idMap.ByID(n.ID0().AmbiguousForm()), rhs.String(c.idMap), rTyp.String(c.idMap))
		}
		if cv := rhs.ConstValue(); cv != nil {
			n.SetConstValue(btoi(cv.Cmp(zero) == 0))
		}
		n.SetMType(TypeExprBoolean)
		return nil
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprUnaryOp", n.ID0().Key())
}

func (c *typeChecker) checkExprBinaryOp(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := c.checkExpr(lhs, depth); err != nil {
		return err
	}
	lTyp := lhs.MType()
	op := n.ID0()
	if op.Key() == t.KeyXBinaryAs {
		rhs := n.RHS().TypeExpr()
		if err := c.checkTypeExpr(rhs, 0); err != nil {
			return err
		}
		if numeric(lTyp) && rhs.IsNumType() {
			n.SetMType(rhs)
			return nil
		}
		return fmt.Errorf("check: cannot convert expression %q, of type %q, as type %q",
			lhs.String(c.idMap), lTyp.String(c.idMap), rhs.String(c.idMap))
	}
	rhs := n.RHS().Expr()
	if err := c.checkExpr(rhs, depth); err != nil {
		return err
	}
	rTyp := rhs.MType()

	switch op.Key() {
	default:
		if !numeric(lTyp) {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				c.idMap.ByID(op.AmbiguousForm()), lhs.String(c.idMap), lTyp.String(c.idMap))
		}
		if !numeric(rTyp) {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				c.idMap.ByID(op.AmbiguousForm()), rhs.String(c.idMap), rTyp.String(c.idMap))
		}
	case t.KeyXBinaryNotEq, t.KeyXBinaryEqEq:
		// No-op.
	case t.KeyXBinaryAnd, t.KeyXBinaryOr:
		if lTyp != TypeExprBoolean {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				c.idMap.ByID(op.AmbiguousForm()), lhs.String(c.idMap), lTyp.String(c.idMap))
		}
		if rTyp != TypeExprBoolean {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				c.idMap.ByID(op.AmbiguousForm()), rhs.String(c.idMap), rTyp.String(c.idMap))
		}
	}

	switch op.Key() {
	default:
		if !lTyp.EqIgnoringRefinements(rTyp) && lTyp != TypeExprIdealNumber && rTyp != TypeExprIdealNumber {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q, do not have compatible types",
				c.idMap.ByID(op.AmbiguousForm()),
				lhs.String(c.idMap), rhs.String(c.idMap),
				lTyp.String(c.idMap), rTyp.String(c.idMap),
			)
		}
	case t.KeyXBinaryShiftL, t.KeyXBinaryShiftR:
		if (lTyp == TypeExprIdealNumber) && (rTyp != TypeExprIdealNumber) {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q; "+
				"cannot shift an ideal number by a non-ideal number",
				c.idMap.ByID(op.AmbiguousForm()),
				lhs.String(c.idMap), rhs.String(c.idMap),
				lTyp.String(c.idMap), rTyp.String(c.idMap),
			)
		}
	}

	if l, r := lhs.ConstValue(), rhs.ConstValue(); l != nil && r != nil {
		if err := c.setConstValueBinaryOp(n, l, r); err != nil {
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

func (c *typeChecker) setConstValueBinaryOp(n *a.Expr, l *big.Int, r *big.Int) error {
	switch n.ID0().Key() {
	case t.KeyXBinaryPlus:
		n.SetConstValue(big.NewInt(0).Add(l, r))
	case t.KeyXBinaryMinus:
		n.SetConstValue(big.NewInt(0).Sub(l, r))
	case t.KeyXBinaryStar:
		n.SetConstValue(big.NewInt(0).Mul(l, r))
	case t.KeyXBinarySlash:
		if r.Cmp(zero) == 0 {
			return fmt.Errorf("check: division by zero in const expression %q", n.String(c.idMap))
		}
		// TODO: decide on Euclidean division vs other definitions. See "go doc
		// math/big int.divmod" for details.
		n.SetConstValue(big.NewInt(0).Div(l, r))
	case t.KeyXBinaryShiftL:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().String(c.idMap), n.String(c.idMap))
		}
		n.SetConstValue(big.NewInt(0).Lsh(l, uint(r.Uint64())))
	case t.KeyXBinaryShiftR:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().String(c.idMap), n.String(c.idMap))
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

func (c *typeChecker) checkExprAssociativeOp(n *a.Expr, depth uint32) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprAssociativeOp", n.ID0().Key())
}

func (c *typeChecker) checkTypeExpr(n *a.TypeExpr, depth uint32) error {
	if depth > a.MaxTypeExprDepth {
		return fmt.Errorf("check: type expression recursion depth too large")
	}
	depth++

	switch n.PackageOrDecorator().Key() {
	case 0:
		if name := n.Name(); name.IsNumType() || name.Key() == t.KeyBool {
			for _, bound := range n.Bounds() {
				if bound == nil {
					continue
				}
				if err := c.checkExpr(bound, 0); err != nil {
					return err
				}
				if bound.ConstValue() == nil {
					return fmt.Errorf("check: %q is not constant", bound.String(c.idMap))
				}
			}
			break
		}
		if n.InclMin() != nil || n.ExclMax() != nil {
			// TODO: reject.
		}
		// TODO: see if name refers to a struct type.
		return fmt.Errorf("check: %q is not a type", c.idMap.ByID(n.Name()))

	case t.KeyPtr:
		if err := c.checkTypeExpr(n.Inner(), depth); err != nil {
			return err
		}

	case t.KeyOpenBracket:
		aLen := n.ArrayLength()
		if err := c.checkExpr(aLen, 0); err != nil {
			return err
		}
		if aLen.ConstValue() == nil {
			return fmt.Errorf("check: %q is not constant", aLen.String(c.idMap))
		}
		if err := c.checkTypeExpr(n.Inner(), depth); err != nil {
			return err
		}

	default:
		// TODO: delete this hack.
		if n.PackageOrDecorator() == c.idMap.ByName("io") && n.Name() == c.idMap.ByName("buf1") {
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
