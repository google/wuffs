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

// TypeString returns a string form of the type n.
func TypeString(m *t.IDMap, n *a.TypeExpr) string {
	if n != nil {
		switch n.PackageOrDecorator() {
		case 0:
			return m.ByID(n.Name())
		case t.IDPtr:
			return "ptr " + TypeString(m, n.Inner())
		case t.IDOpenBracket:
			// TODO.
		default:
			return m.ByID(n.PackageOrDecorator()) + "." + m.ByID(n.Name())
		}
	}
	return "!invalid_type!"
}

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
	p := c.funcs[f.QID()].LocalVars
	for _, m := range f.Body() {
		if err := c.checkVars(m, p); err != nil {
			return fmt.Errorf("%v at %s:%d", err, m.Raw().Filename(), m.Raw().Line())
		}
	}
	for _, m := range f.Body() {
		if err := c.checkStatement(m, p); err != nil {
			return fmt.Errorf("%v at %s:%d", err, m.Raw().Filename(), m.Raw().Line())
		}
	}
	return nil
}

func (c *Checker) checkVars(n *a.Node, p TypeMap) error {
	if n.Kind() == a.KVar {
		v := n.Var()
		name := v.Name()
		if _, ok := p[name]; ok {
			return fmt.Errorf("check: duplicate var %q", c.idMap.ByID(name))
		}
		if err := c.checkTypeExpr(v.XType()); err != nil {
			return err
		}
		if value := v.Value(); value != nil {
			// TODO: check that value doesn't mention the variable itself.
		}
		p[name] = v.XType()
		return nil
	}
	for _, l := range n.Raw().SubLists() {
		for _, m := range l {
			if err := c.checkVars(m, p); err != nil {
				return err
			}
		}
	}
	return nil
}

func (c *Checker) checkStatement(n *a.Node, p TypeMap) error {
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
			if err := c.checkStatement(m, p); err != nil {
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
				if err := c.checkStatement(m, p); err != nil {
					return err
				}
			}
			for _, m := range o.BodyIfFalse() {
				if err := c.checkStatement(m, p); err != nil {
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

func (c *Checker) checkExpr(n *a.Expr) error {
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

func (c *Checker) checkExprOther(n *a.Expr) error {
	switch n.ID0() {
	case 0:
		// n is an identifier or a literal.
		if n.ID1().IsNumLiteral() {
			n.SetMType(DummyTypeIdealNumber)
			return nil
		}
		// TODO.

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

func (c *Checker) checkExprUnaryOp(n *a.Expr) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprUnaryOp", n.ID0().Key())
}

func (c *Checker) checkExprBinaryOp(n *a.Expr) error {
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

func (c *Checker) checkExprAssociativeOp(n *a.Expr) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprAssociativeOp", n.ID0().Key())
}

func (c *Checker) checkTypeExpr(n *a.TypeExpr) error {
	for n != nil {
		switch n.PackageOrDecorator() {
		case 0:
			name := n.Name()
			if name.IsNumType() || name == t.IDBool {
				return nil
			}
			// TODO: see if name refers to a struct type.
			return fmt.Errorf("check: %q is not a type", c.idMap.ByID(name))

		case t.IDPtr:
			n = n.Inner()

		case t.IDOpenBracket:
			// TODO.
			fallthrough

		default:
			return fmt.Errorf("check: unrecognized node for checkTypeExpr")
		}
	}
	return nil
}

func (c *Checker) typeConvertible(e *a.Expr, typ *a.TypeExpr) error {
	if typ.PackageOrDecorator() == 0 {
		if name := typ.Name(); name.IsBuiltIn() && name.IsNumType() {
			if minMax := numTypeRanges[0xFF&(name>>t.KeyShift)]; minMax[0] != nil {
				if e.MType() == DummyTypeIdealNumber {
					// TODO: check that the val is in typ's bounds.
					return nil
				}
			}
		}
	}

	return fmt.Errorf("check: typeConvertible: cannot convert expression %v to type %v", e, typ)
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
