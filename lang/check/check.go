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

var (
	// DummyTypeFoo are dummy values for the a.Node.Type field:
	//  - DummyTypeType means that n is itself a type.
	//  - DummyTypeNumber means that n is an ideal numeric constant.
	DummyTypeType        = &a.Node{Kind: a.KType}
	DummyTypeIdealNumber = &a.Node{Kind: a.KType}
)

// TypeString returns a string form of the type n.
func TypeString(n *a.Node, m *t.IDMap) string {
	if n == nil {
		return "!nil!"
	}
	if n.Kind != a.KType {
		return "!not_a_type!"
	}
	switch n {
	case DummyTypeType:
		return "!invalid_type!"
	case DummyTypeIdealNumber:
		return "!ideal!"
	}
	switch n.ID0 {
	case 0:
		return m.ByID(n.ID1)
	case t.IDPtr:
		return "ptr " + TypeString(n.RHS, m)
	case t.IDOpenBracket:
		// TODO.
	default:
		return m.ByID(n.ID0) + "." + m.ByID(n.ID1)
	}
	return "!invalid_type!"
}

// TypeMap maps from variable names (as token IDs) to types.
type TypeMap map[t.ID]*a.Node

type Func struct {
	QID       t.QID
	Node      *a.Node
	LocalVars TypeMap
}

func Check(m *t.IDMap, files ...*a.Node) (*Checker, error) {
	for _, f := range files {
		if f == nil {
			return nil, errors.New("check: Check given a nil ast.Node")
		}
		if f.Kind != a.KFile {
			return nil, errors.New("check: Check given a non-file ast.Node")
		}
	}

	if len(files) > 1 {
		m := map[string]bool{}
		for _, f := range files {
			if m[f.Filename] {
				return nil, fmt.Errorf("check: Check given duplicate filename %q", f.Filename)
			}
			m[f.Filename] = true
		}
	}

	c := &Checker{
		idMap: m,
		funcs: map[t.QID]Func{},
	}
	for _, phase := range phases {
		for _, f := range files {
			for _, n := range f.List0 {
				if err := phase(c, n); err != nil {
					return nil, err
				}
			}
		}
	}
	return c, nil
}

var phases = [...]func(*Checker, *a.Node) error{
	(*Checker).checkUse,
	(*Checker).checkStruct,
	(*Checker).checkFuncSignature,
	(*Checker).checkFuncBody,
}

type Checker struct {
	idMap *t.IDMap
	funcs map[t.QID]Func
}

func (c *Checker) Funcs() map[t.QID]Func { return c.funcs }

func (c *Checker) checkUse(n *a.Node) error {
	if n.Kind != a.KUse {
		return nil
	}
	// TODO.
	return nil
}

func (c *Checker) checkStruct(n *a.Node) error {
	if n.Kind != a.KStruct {
		return nil
	}
	// TODO.
	return nil
}

func (c *Checker) checkFuncSignature(n *a.Node) error {
	if n.Kind != a.KFunc {
		return nil
	}
	// TODO.
	return nil
}

func (c *Checker) checkFuncBody(n *a.Node) error {
	if n.Kind != a.KFunc {
		return nil
	}
	p := TypeMap{}
	for _, m := range n.List2 {
		if err := c.checkStatement(m, p); err != nil {
			return fmt.Errorf("%v at %s:%d", err, m.Filename, m.Line)
		}
	}
	qid := t.QID{n.ID0, n.ID1}
	c.funcs[qid] = Func{
		QID:       qid,
		Node:      n,
		LocalVars: p,
	}
	return nil
}

func (c *Checker) checkStatement(n *a.Node, p TypeMap) error {
	if n == nil {
		return nil
	}

	switch n.Kind {
	case a.KAssert:
		if err := c.checkExpr(n.RHS); err != nil {
			return err
		}
		// TODO: check that n.RHS has type bool.
		// TODO: check the actual assertion.
		return nil

	case a.KAssign:
		if err := c.checkExpr(n.RHS); err != nil {
			return err
		}
		// TODO, look at the op (n.ID0).
		if id := isUserDefinedIdent(n.LHS); id != 0 {
			if previouslyInferredType, ok := p[id]; ok {
				// TODO: check that n.RHS.Type is assignable to previouslyInferredType.
				n.LHS.Type = previouslyInferredType
			} else {
				p[id] = n.RHS.Type
				n.LHS.Type = n.RHS.Type
			}
		} else {
			if err := c.checkExpr(n.LHS); err != nil {
				return err
			}
			// TODO: check that n.RHS.Type is assignable to n.LHS.Type.
		}
		return nil

	case a.KFor:
		if err := c.checkExpr(n.LHS); err != nil {
			return err
		}
		// TODO: check n.LHS has type bool.
		for _, m := range n.List2 {
			if err := c.checkStatement(m, p); err != nil {
				return err
			}
		}
		return nil

	case a.KIf:
		for ; n != nil; n = n.RHS {
			if err := c.checkExpr(n.LHS); err != nil {
				return err
			}
			// TODO: check n.LHS has type bool.
			for _, m := range n.List1 {
				if err := c.checkStatement(m, p); err != nil {
					return err
				}
			}
			for _, m := range n.List2 {
				if err := c.checkStatement(m, p); err != nil {
					return err
				}
			}
		}
		return nil

	case a.KReturn:
		if err := c.checkExpr(n.LHS); err != nil {
			return err
		}
		// TODO: type-check that n.LHS is assignable to the return value. This
		// needs the context of what func we're in.
		return nil

	case a.KBreak:
		// TODO: check that we're in a for loop.
		return nil

	case a.KContinue:
		// TODO: check that we're in a for loop.
		return nil
	}

	return fmt.Errorf("check: unrecognized ast.Kind (%d) for checkStatement", n.Kind)
}

func (c *Checker) checkExpr(n *a.Node) error {
	if n == nil {
		return nil
	}
	if n.Kind != a.KExpr {
		return fmt.Errorf("check: unexpected ast.Kind (%d) for checkExpr", n.Kind)
	}
	switch n.ID0.Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
	case 0:
		return c.checkExprOther(n)
	case t.FlagsUnaryOp:
		return c.checkExprUnaryOp(n)
	case t.FlagsBinaryOp:
		return c.checkExprBinaryOp(n)
	case t.FlagsAssociativeOp:
		return c.checkExprAssociativeOp(n)
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExpr", n.ID0.Key())
}

func (c *Checker) checkExprOther(n *a.Node) error {
	switch n.ID0 {
	case 0:
		// n is an identifier or a literal.
		if n.ID1.IsNumLiteral() {
			n.Type = DummyTypeIdealNumber
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
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprOther", n.ID0.Key())
}

func (c *Checker) checkExprUnaryOp(n *a.Node) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprUnaryOp", n.ID0.Key())
}

func (c *Checker) checkExprBinaryOp(n *a.Node) error {
	if err := c.checkExpr(n.LHS); err != nil {
		return err
	}
	if n.ID0 == t.IDXBinaryAs {
		if err := c.checkType(n.RHS); err != nil {
			return err
		}
		if err := c.typeConvertible(n.LHS, n.RHS); err != nil {
			return err
		}
		n.Type = n.RHS
		return nil
	}
	if err := c.checkExpr(n.RHS); err != nil {
		return err
	}
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprBinaryOp", n.ID0.Key())
}

func (c *Checker) checkExprAssociativeOp(n *a.Node) error {
	// TODO.
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for checkExprAssociativeOp", n.ID0.Key())
}

func (c *Checker) checkType(n *a.Node) error {
	for n != nil {
		if n.Kind != a.KType {
			return fmt.Errorf("check: unexpected ast.Kind (%d) for checkType", n.Kind)
		}

		switch n.ID0 {
		case 0:
			if n.ID1.IsNumType() || n.ID1 == t.IDBool {
				n.Type = DummyTypeType
				return nil
			}
			// TODO: see if n.ID1 refers to a struct type.
			return fmt.Errorf("check: %q is not a type", c.idMap.ByID(n.ID1))

		case t.IDPtr:
			n = n.RHS

		default:
			return fmt.Errorf("check: unrecognized node for checkType")
		}
	}
	return nil
}

func (c *Checker) typeConvertible(val, typ *a.Node) error {
	if val.Type == nil {
		return fmt.Errorf("check: typeConvertible: no type for val %v", val)
	}
	if typ.Type == nil {
		return fmt.Errorf("check: typeConvertible: no type for typ %v", typ)
	}
	if typ.Type != DummyTypeType {
		typ = typ.Type
		if typ.Type != DummyTypeType {
			return fmt.Errorf("check: typeConvertible: invalid type for typ %v", typ)
		}
	}

	if typ.ID0 == 0 && typ.ID1.IsBuiltIn() && typ.ID1.IsNumType() {
		if minMax := numTypeRanges[0xFF&(typ.ID1>>t.KeyShift)]; minMax[0] != nil {
			if val.Type == DummyTypeIdealNumber {
				// TODO: check that the val is in typ's bounds.
				return nil
			}
		}
	}

	return fmt.Errorf("check: typeConvertible: cannot convert value %v to type %v", val, typ)
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

func isUserDefinedIdent(n *a.Node) t.ID {
	if n != nil && n.Kind == a.KExpr && n.ID0 == 0 && n.ID1.IsIdent() && !n.ID1.IsBuiltIn() {
		return n.ID1
	}
	return 0
}
