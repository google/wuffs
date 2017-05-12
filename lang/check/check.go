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

// DummyTypeIdealNumber is an *ast.Node MType (implicit type) that means that n
// is an ideal numeric constant.
var DummyTypeIdealNumber = a.NewTypeExpr(0, t.IDIdeal, nil, nil, nil)

// TypeMap maps from variable names (as token IDs) to types.
type TypeMap map[t.ID]*a.TypeExpr

type Func struct {
	QID       t.QID
	Func      *a.Func
	LocalVars TypeMap
}

func Check(m *t.IDMap, files ...*a.File) (*Checker, error) {
	for _, f := range files {
		if f == nil {
			return nil, errors.New("check: Check given a nil *ast.File")
		}
	}

	if len(files) > 1 {
		m := map[string]bool{}
		for _, f := range files {
			if m[f.Filename()] {
				return nil, fmt.Errorf("check: Check given duplicate filename %q", f.Filename())
			}
			m[f.Filename()] = true
		}
	}

	c := &Checker{
		idMap: m,
		funcs: map[t.QID]Func{},
	}
	for _, phase := range phases {
		for _, f := range files {
			for _, n := range f.TopLevelDecls() {
				if n.Kind() != phase.kind {
					continue
				}
				if err := phase.check(c, n); err != nil {
					return nil, err
				}
			}
		}
	}
	return c, nil
}

var phases = [...]struct {
	kind  a.Kind
	check func(*Checker, *a.Node) error
}{
	{a.KUse, (*Checker).checkUse},
	{a.KStruct, (*Checker).checkStruct},
	{a.KFunc, (*Checker).checkFuncSignature},
	{a.KFunc, (*Checker).checkFuncBody},
}

type Checker struct {
	idMap *t.IDMap
	funcs map[t.QID]Func
}

func (c *Checker) Funcs() map[t.QID]Func { return c.funcs }

func (c *Checker) checkUse(n *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkStruct(n *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkFuncSignature(n *a.Node) error {
	f := n.Func()

	// TODO: check the in and out params.

	qid := f.QID()
	if _, ok := c.funcs[qid]; ok {
		if qid[0] == 0 {
			return fmt.Errorf(`check: duplicate function "%s"`, c.idMap.ByID(qid[1]))
		} else {
			return fmt.Errorf(`check: duplicate function "%s.%s"`, c.idMap.ByID(qid[0]), c.idMap.ByID(qid[1]))
		}
	}
	c.funcs[qid] = Func{
		QID:       qid,
		Func:      f,
		LocalVars: TypeMap{},
	}
	return nil
}

func (c *Checker) checkFuncBody(n *a.Node) error {
	f := n.Func()
	tc := &typeChecker{
		c:       c,
		idMap:   c.idMap,
		typeMap: c.funcs[f.QID()].LocalVars,
	}
	for _, m := range f.Body() {
		if err := tc.checkVars(m); err != nil {
			return fmt.Errorf("%v at %s:%d", err, m.Raw().Filename(), m.Raw().Line())
		}
	}
	for _, m := range f.Body() {
		if err := tc.checkStatement(m); err != nil {
			return fmt.Errorf("%v at %s:%d", err, m.Raw().Filename(), m.Raw().Line())
		}
	}
	return nil
}

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
		if err := c.checkExpr(o.LHS()); err != nil {
			return err
		}
		if err := c.checkExpr(o.RHS()); err != nil {
			return err
		}
		// TODO: check that o.RHS().Type is assignable to o.LHS().Type.
		// TODO, look at o.Operator().
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
						return fmt.Errorf("check: %q is not constant", ExprString(c.idMap, bound))
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
				return fmt.Errorf("check: %q is not constant", ExprString(c.idMap, aLen))
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
		return fmt.Errorf("check: expression %q has no inferred type", ExprString(c.idMap, e))
	}

	if typ.PackageOrDecorator() == 0 {
		if name := typ.Name(); name.IsBuiltIn() && name.IsNumType() {
			if eTyp == DummyTypeIdealNumber {
				minMax := numTypeRanges[0xFF&(name>>t.KeyShift)]
				if minMax[0] == nil {
					return fmt.Errorf("check: unknown range for built-in numeric type %q",
						TypeExprString(c.idMap, typ))
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
		ExprString(c.idMap, e),
		TypeExprString(c.idMap, eTyp),
		TypeExprString(c.idMap, typ),
	)
}

var numTypeRanges = [256][2]*big.Int{
	t.IDI8 >> t.KeyShift:    {big.NewInt(-1 << 7), big.NewInt(1<<7 - 1)},
	t.IDI16 >> t.KeyShift:   {big.NewInt(-1 << 15), big.NewInt(1<<15 - 1)},
	t.IDI32 >> t.KeyShift:   {big.NewInt(-1 << 31), big.NewInt(1<<31 - 1)},
	t.IDI64 >> t.KeyShift:   {big.NewInt(-1 << 63), big.NewInt(1<<63 - 1)},
	t.IDU8 >> t.KeyShift:    {zero, big.NewInt(0).SetUint64(1<<8 - 1)},
	t.IDU16 >> t.KeyShift:   {zero, big.NewInt(0).SetUint64(1<<16 - 1)},
	t.IDU32 >> t.KeyShift:   {zero, big.NewInt(0).SetUint64(1<<32 - 1)},
	t.IDU64 >> t.KeyShift:   {zero, big.NewInt(0).SetUint64(1<<64 - 1)},
	t.IDUsize >> t.KeyShift: {zero, zero},
}

var zero = big.NewInt(0)
