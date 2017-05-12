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
	c       *Checker
	idMap   *t.IDMap
	typeMap TypeMap
}

func (c *typeChecker) checkVars(n *a.Node) error {
	if n.Kind() == a.KVar {
		v := n.Var()
		name := v.Name()
		if _, ok := c.typeMap[name]; ok {
			return fmt.Errorf("check: duplicate var %q", c.idMap.ByID(name))
		}
		if err := c.checkTypeExpr(v.XType()); err != nil {
			return err
		}
		if value := v.Value(); value != nil {
			// TODO: check that value doesn't mention the variable itself.
		}
		c.typeMap[name] = v.XType()
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
	switch n.Kind() {
	case a.KAssert:
		o := n.Assert()
		if err := c.checkExpr(o.Condition()); err != nil {
			return err
		}
		// TODO: check that o.Condition() has type bool.
		// TODO: check the actual assertion.
		return nil

	case a.KAssign:
		o := n.Assign()
		l := o.LHS()
		r := o.RHS()
		if err := c.checkExpr(l); err != nil {
			return err
		}
		if err := c.checkExpr(r); err != nil {
			return err
		}
		lTyp := l.MType()
		rTyp := r.MType()
		// TODO, look at o.Operator().
		if !lTyp.Eq(rTyp) {
			return fmt.Errorf("check: cannot assign %q of type %q to %q of type %q",
				r.String(c.idMap), rTyp.String(c.idMap), l.String(c.idMap), lTyp.String(c.idMap))
		}
		return nil

	case a.KVar:
		o := n.Var()
		if value := o.Value(); value != nil {
			if err := c.checkExpr(value); err != nil {
				return err
			}
			// TODO: check that value.Type is assignable to o.TypeExpr().
		}
		return nil

	case a.KFor:
		o := n.For()
		if cond := o.Condition(); cond != nil {
			if err := c.checkExpr(cond); err != nil {
				return err
			}
			// TODO: check cond has type bool.
		}
		for _, m := range o.Body() {
			if err := c.checkStatement(m); err != nil {
				return err
			}
		}
		return nil

	case a.KIf:
		for o := n.If(); o != nil; o = o.ElseIf() {
			if err := c.checkExpr(o.Condition()); err != nil {
				return err
			}
			// TODO: check o.Condition() has type bool.
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
		return nil

	case a.KReturn:
		o := n.Return()
		if value := o.Value(); value != nil {
			if err := c.checkExpr(value); err != nil {
				return err
			}
			// TODO: type-check that value is assignable to the return value.
			// This needs the context of what func we're in.
		}
		return nil

	case a.KBreak:
		// TODO: check that we're in a for loop.
		return nil

	case a.KContinue:
		// TODO: check that we're in a for loop.
		return nil
	}

	return fmt.Errorf("check: unrecognized ast.Kind (%d) for checkStatement", n.Kind())
}

func (c *typeChecker) checkExpr(n *a.Expr) error {
	switch n.ID0().Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		return c.checkExprOther(n)
	case t.FlagsUnaryOp:
		return c.checkExprUnaryOp(n)
	case t.FlagsBinaryOp:
		return c.checkExprBinaryOp(n)
	case t.FlagsAssociativeOp:
		return c.checkExprAssociativeOp(n)
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExpr", n.ID0().Key())
}

func (c *typeChecker) checkExprOther(n *a.Expr) error {
	switch n.ID0() {
	case 0:
		if n.ID1().IsNumLiteral() {
			z := big.NewInt(0)
			s := c.idMap.ByID(n.ID1())
			if _, ok := z.SetString(s, 0); !ok {
				return fmt.Errorf("check: invalid numeric literal %q", s)
			}
			n.SetConstValue(z)
			n.SetMType(DummyTypeIdealNumber)
			return nil
		}

		if n.ID1().IsIdent() {
			if typ, ok := c.typeMap[n.ID1()]; ok {
				n.SetMType(typ)
				return nil
			}
			// TODO: look for (global) names (constants, funcs, structs).
			return fmt.Errorf("check: unrecognized identifier %q", c.idMap.ByID(n.ID1()))
		}

	case t.IDOpenParen:
		// n is a function call.
		// TODO.

	case t.IDOpenBracket:
		// n is an index.
		// TODO.

	case t.IDColon:
		// n is a slice.
		// TODO.

	case t.IDDot:
		// n is a selector.
		// TODO.
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprOther", n.ID0().Key())
}

func (c *typeChecker) checkExprUnaryOp(n *a.Expr) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprUnaryOp", n.ID0().Key())
}

func (c *typeChecker) checkExprBinaryOp(n *a.Expr) error {
	if err := c.checkExpr(n.LHS().Expr()); err != nil {
		return err
	}
	if n.ID0() == t.IDXBinaryAs {
		lhs := n.LHS().Expr()
		rhs := n.RHS().TypeExpr()
		if err := c.checkTypeExpr(rhs); err != nil {
			return err
		}
		if err := c.typeConvertible(lhs, rhs); err != nil {
			return err
		}
		n.SetMType(rhs)
		return nil
	}
	if err := c.checkExpr(n.RHS().Expr()); err != nil {
		return err
	}
	// TODO: check lhs and rhs have compatible types, then call n.SetMType.
	// TODO: other checks.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprBinaryOp", n.ID0().Key())
}

func (c *typeChecker) checkExprAssociativeOp(n *a.Expr) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprAssociativeOp", n.ID0().Key())
}

func (c *typeChecker) checkTypeExpr(n *a.TypeExpr) error {
	for ; n != nil; n = n.Inner() {
		switch n.PackageOrDecorator() {
		case 0:
			name := n.Name()
			if name.IsNumType() || name == t.IDBool {
				for _, bound := range n.Bounds() {
					if bound == nil {
						continue
					}
					if err := c.checkExpr(bound); err != nil {
						return err
					}
					if bound.ConstValue() == nil {
						return fmt.Errorf("check: %q is not constant", bound.String(c.idMap))
					}
				}
				return nil
			}
			if n.InclMin() != nil || n.ExclMax() != nil {
				// TODO: reject.
			}
			// TODO: see if name refers to a struct type.
			return fmt.Errorf("check: %q is not a type", c.idMap.ByID(name))

		case t.IDPtr:
			// No-op.

		case t.IDOpenBracket:
			aLen := n.ArrayLength()
			if err := c.checkExpr(aLen); err != nil {
				return err
			}
			if aLen.ConstValue() == nil {
				return fmt.Errorf("check: %q is not constant", aLen.String(c.idMap))
			}

		default:
			return fmt.Errorf("check: unrecognized node for checkTypeExpr")
		}
	}
	return nil
}

func (c *typeChecker) typeConvertible(e *a.Expr, typ *a.TypeExpr) error {
	eTyp := e.MType()
	if eTyp == nil {
		return fmt.Errorf("check: expression %q has no inferred type", e.String(c.idMap))
	}

	if typ.PackageOrDecorator() == 0 {
		if name := typ.Name(); name.IsBuiltIn() && name.IsNumType() {
			if eTyp == DummyTypeIdealNumber {
				minMax := numTypeRanges[0xFF&(name>>t.KeyShift)]
				if minMax[0] == nil {
					return fmt.Errorf("check: unknown range for built-in numeric type %q", typ.String(c.idMap))
				}
				// TODO: update minMax for typ.Bounds().
				//
				// TODO: compare val to minMax. Or does that belong in a
				// separate bounds checking phase instead of type checking?
				return nil
			} else {
				// TODO: check that eTyp is a numeric type, not e.g. a struct.
				return nil
			}
		}
	}

	return fmt.Errorf("check: cannot convert expression %q of type %q to type %q",
		e.String(c.idMap), eTyp.String(c.idMap), typ.String(c.idMap))
}
