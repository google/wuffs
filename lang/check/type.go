// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

func (q *checker) tcheckVars(block []*a.Node) error {
	for _, o := range block {
		q.errFilename, q.errLine = o.Raw().FilenameLine()

		switch o.Kind() {
		case a.KIf:
			for o := o.If(); o != nil; o = o.ElseIf() {
				if err := q.tcheckVars(o.BodyIfTrue()); err != nil {
					return err
				}
				if err := q.tcheckVars(o.BodyIfFalse()); err != nil {
					return err
				}
			}

		case a.KVar:
			o := o.Var()
			name := o.Name()
			if _, ok := q.f.LocalVars[name]; ok {
				return fmt.Errorf("check: duplicate var %q", name.String(q.tm))
			}
			if err := q.tcheckTypeExpr(o.XType(), 0); err != nil {
				return err
			}
			q.f.LocalVars[name] = o.XType()

		case a.KWhile:
			if err := q.tcheckVars(o.While().Body()); err != nil {
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

	case a.KExpr:
		return q.tcheckExpr(n.Expr(), 0)

	case a.KIf:
		for n := n.If(); n != nil; n = n.ElseIf() {
			cond := n.Condition()
			if err := q.tcheckExpr(cond, 0); err != nil {
				return err
			}
			if !cond.MType().IsBool() {
				return fmt.Errorf("check: if condition %q, of type %q, does not have a boolean type",
					cond.String(q.tm), cond.MType().String(q.tm))
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
		for n := n.If(); n != nil; n = n.ElseIf() {
			n.Node().SetTypeChecked()
		}
		return nil

	case a.KJump:
		n := n.Jump()
		jumpTarget := (*a.While)(nil)
		if id := n.Label(); id != 0 {
			for i := len(q.jumpTargets) - 1; i >= 0; i-- {
				if w := q.jumpTargets[i]; w.Label() == id {
					jumpTarget = w
					break
				}
			}
		} else if nj := len(q.jumpTargets); nj > 0 {
			jumpTarget = q.jumpTargets[nj-1]
		}
		if jumpTarget == nil {
			sepStr, labelStr := "", ""
			if id := n.Label(); id != 0 {
				sepStr, labelStr = ":", id.String(q.tm)
			}
			return fmt.Errorf("no matching while statement for %s%s%s",
				n.Keyword().String(q.tm), sepStr, labelStr)
		}
		if n.Keyword().Key() == t.KeyBreak {
			jumpTarget.SetHasBreak()
		} else {
			jumpTarget.SetHasContinue()
		}
		n.SetJumpTarget(jumpTarget)

	case a.KReturn:
		n := n.Return()
		if nk := n.Keyword(); nk != 0 {
			if s, ok := q.c.statuses[n.Message()]; !ok {
				return fmt.Errorf("check: no error or status with message %q",
					trimQuotes(n.Message().String(q.tm)))
			} else if sk := s.Status.Keyword(); nk != sk {
				return fmt.Errorf("check: return statement says %q but declaration says %q",
					nk.String(q.tm), sk.String(q.tm))
			}
		} else if value := n.Value(); value != nil {
			if err := q.tcheckExpr(value, 0); err != nil {
				return err
			}
			// TODO: type-check that value is assignable to the return value.
			// This needs the context of what func we're in.
		}

	case a.KVar:
		n := n.Var()
		if !n.XType().Node().TypeChecked() {
			return fmt.Errorf("check: internal error: unchecked type expression %q", n.XType().String(q.tm))
		}
		if value := n.Value(); value != nil {
			if err := q.tcheckExpr(value, 0); err != nil {
				return err
			}
			lTyp := n.XType()
			rTyp := value.MType()
			if (rTyp.IsIdeal() && lTyp.IsNumType()) || lTyp.EqIgnoringRefinements(rTyp) {
				// No-op.
			} else {
				return fmt.Errorf("check: cannot assign %q of type %q to %q of type %q",
					value.String(q.tm), rTyp.String(q.tm), n.Name().String(q.tm), lTyp.String(q.tm))
			}
		} else {
			// TODO: check that the default zero value is assignable to n.XType().
		}

	case a.KWhile:
		n := n.While()
		cond := n.Condition()
		if err := q.tcheckExpr(cond, 0); err != nil {
			return err
		}
		if !cond.MType().IsBool() {
			return fmt.Errorf("check: for-loop condition %q, of type %q, does not have a boolean type",
				cond.String(q.tm), cond.MType().String(q.tm))
		}
		for _, o := range n.Asserts() {
			if err := q.tcheckAssert(o.Assert()); err != nil {
				return err
			}
			o.SetTypeChecked()
		}
		q.jumpTargets = append(q.jumpTargets, n)
		defer func() {
			q.jumpTargets = q.jumpTargets[:len(q.jumpTargets)-1]
		}()
		for _, o := range n.Body() {
			if err := q.tcheckStatement(o); err != nil {
				return err
			}
		}

	default:
		return fmt.Errorf("check: unrecognized ast.Kind (%s) for tcheckStatement", n.Kind())
	}

	n.SetTypeChecked()
	return nil
}

func (q *checker) tcheckAssert(n *a.Assert) error {
	cond := n.Condition()
	if err := q.tcheckExpr(cond, 0); err != nil {
		return err
	}
	if !cond.MType().IsBool() {
		return fmt.Errorf("check: assert condition %q, of type %q, does not have a boolean type",
			cond.String(q.tm), cond.MType().String(q.tm))
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
		if (rTyp.IsIdeal() && lTyp.IsNumType()) || lTyp.EqIgnoringRefinements(rTyp) {
			return nil
		}
		return fmt.Errorf("check: cannot assign %q of type %q to %q of type %q",
			rhs.String(q.tm), rTyp.String(q.tm), lhs.String(q.tm), lTyp.String(q.tm))
	}

	if !lTyp.IsNumType() {
		return fmt.Errorf("check: assignment %q: assignee %q, of type %q, does not have numeric type",
			n.Operator().String(q.tm), lhs.String(q.tm), lTyp.String(q.tm))
	}

	switch n.Operator().Key() {
	case t.KeyShiftLEq, t.KeyShiftREq:
		if rTyp.IsNumTypeOrIdeal() {
			return nil
		}
		return fmt.Errorf("check: assignment %q: shift %q, of type %q, does not have numeric type",
			n.Operator().String(q.tm), rhs.String(q.tm), rTyp.String(q.tm))
	}

	if rTyp.IsIdeal() || lTyp.EqIgnoringRefinements(rTyp) {
		return nil
	}
	return fmt.Errorf("check: assignment %q: %q and %q, of types %q and %q, do not have compatible types",
		n.Operator().String(q.tm),
		lhs.String(q.tm), rhs.String(q.tm),
		lTyp.String(q.tm), rTyp.String(q.tm),
	)
}

func (q *checker) tcheckArg(n *a.Arg, depth uint32) error {
	if err := q.tcheckExpr(n.Value(), depth); err != nil {
		return err
	}
	n.Node().SetTypeChecked()
	return nil
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
		return fmt.Errorf("check: unrecognized token.Key (0x%X) for tcheckExpr", n.ID0().Key())
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
			s := id1.String(q.tm)
			if _, ok := z.SetString(s, 0); !ok {
				return fmt.Errorf("check: invalid numeric literal %q", s)
			}
			n.SetConstValue(z)
			n.SetMType(typeExprIdeal)
			return nil

		} else if id1.IsIdent() {
			if q.f.LocalVars != nil {
				if typ, ok := q.f.LocalVars[id1]; ok {
					n.SetMType(typ)
					return nil
				}
			}
			if c, ok := q.c.consts[id1]; ok {
				// TODO: check somewhere that a global ident (i.e. a const) is
				// not directly in the LHS of an assignment.
				n.SetGlobalIdent()
				n.SetMType(c.Const.XType())
				return nil
			}
			// TODO: look for other (global) names: funcs, structs.
			return fmt.Errorf("check: unrecognized identifier %q", id1.String(q.tm))
		}
		switch id1.Key() {
		case t.KeyFalse:
			n.SetConstValue(zero)
			n.SetMType(typeExprBool)
			return nil

		case t.KeyTrue:
			n.SetConstValue(one)
			n.SetMType(typeExprBool)
			return nil

		case t.KeyUnderscore:
			// TODO.

		case t.KeyThis:
			// TODO.
		}

	case t.KeyOpenParen:
		// n is a function call.
		// TODO: delete this hack that only matches "in.src.read_u8?()" etc.
		if isInSrc(q.tm, n, t.KeyReadU8, 0) || isInSrc(q.tm, n, t.KeyReadU32LE, 0) ||
			isInSrc(q.tm, n, t.KeySkip32, 1) ||
			isInDst(q.tm, n, t.KeyWrite, 1) || isInDst(q.tm, n, t.KeyWriteU8, 1) ||
			isInDst(q.tm, n, t.KeyCopyFrom32, 2) ||
			isThisMethod(q.tm, n, "decode_header", 1) || isThisMethod(q.tm, n, "decode_lsd", 1) ||
			isThisMethod(q.tm, n, "decode_extension", 1) || isThisMethod(q.tm, n, "decode_id", 2) ||
			isThisMethod(q.tm, n, "decode_uncompressed", 2) || isThisMethod(q.tm, n, "decode_dynamic", 2) ||
			isThisMethod(q.tm, n, "init_huffs", 1) || isThisMethod(q.tm, n, "init_huff", 3) {

			if err := q.tcheckExpr(n.LHS().Expr(), depth); err != nil {
				return err
			}
			for _, o := range n.Args() {
				if err := q.tcheckArg(o.Arg(), depth); err != nil {
					return err
				}
			}
			if isInSrc(q.tm, n, t.KeyReadU32LE, 0) {
				n.SetMType(typeExprPlaceholder32) // HACK.
			} else {
				n.SetMType(typeExprPlaceholder) // HACK.
			}
			return nil
		}
		// TODO: delete this hack that only matches "foo.bar_bits(etc)".
		if isLowHighBits(q.tm, n, t.KeyLowBits) || isLowHighBits(q.tm, n, t.KeyHighBits) {
			foo := n.LHS().Expr().LHS().Expr()
			if err := q.tcheckExpr(foo, depth); err != nil {
				return err
			}
			n.LHS().SetTypeChecked()
			n.LHS().Expr().SetMType(typeExprPlaceholder) // HACK.
			for _, o := range n.Args() {
				if err := q.tcheckArg(o.Arg(), depth); err != nil {
					return err
				}
			}
			n.SetMType(foo.MType().Unrefined())
			return nil
		}
		// TODO: delete this hack that only matches "foo.set_literal_width(etc)".
		if isSetLiteralWidth(q.tm, n) {
			foo := n.LHS().Expr().LHS().Expr()
			if err := q.tcheckExpr(foo, depth); err != nil {
				return err
			}
			n.LHS().SetTypeChecked()
			n.LHS().Expr().SetMType(typeExprPlaceholder) // HACK.
			for _, o := range n.Args() {
				if err := q.tcheckArg(o.Arg(), depth); err != nil {
					return err
				}
			}
			n.SetMType(typeExprPlaceholder) // HACK.
			return nil
		}
		// TODO: delete this hack that only matches "foo.decode(etc)".
		if isDecode(q.tm, n) {
			foo := n.LHS().Expr().LHS().Expr()
			if err := q.tcheckExpr(foo, depth); err != nil {
				return err
			}
			n.LHS().SetTypeChecked()
			n.LHS().Expr().SetMType(typeExprPlaceholder) // HACK.
			for _, o := range n.Args() {
				if err := q.tcheckArg(o.Arg(), depth); err != nil {
					return err
				}
			}
			n.SetMType(typeExprPlaceholder) // HACK.
			return nil
		}

	case t.KeyOpenBracket:
		// n is an index.
		lhs := n.LHS().Expr()
		if err := q.tcheckExpr(lhs, depth); err != nil {
			return err
		}
		rhs := n.RHS().Expr()
		if err := q.tcheckExpr(rhs, depth); err != nil {
			return err
		}
		lTyp := lhs.MType()
		if lTyp.Decorator().Key() != t.KeyOpenBracket {
			return fmt.Errorf("check: %s is an array-index expression but %s has type %s, not an array type",
				n.String(q.tm), lhs.String(q.tm), lTyp.String(q.tm))
		}
		rTyp := rhs.MType()
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: %s is an array-index expression but %s has type %s, not a numeric type",
				n.String(q.tm), rhs.String(q.tm), rTyp.String(q.tm))
		}
		n.SetMType(lTyp.Inner())
		return nil

	case t.KeyColon:
		// n is a slice.
		// TODO.

	case t.KeyDot:
		return q.tcheckDot(n, depth)

	case t.KeyDollar:
		for _, o := range n.Args() {
			o := o.Expr()
			if err := q.tcheckExpr(o, depth); err != nil {
				return err
			}
		}
		n.SetMType(typeExprList)
		return nil

	case t.KeyLimit:
		lhs := n.LHS().Expr()
		if err := q.tcheckExpr(lhs, depth); err != nil {
			return err
		}
		if lhs.Impure() {
			return fmt.Errorf(`check: limit count %q is not pure`, lhs.String(q.tm))
		}
		lTyp := lhs.MType()
		if !lTyp.IsIdeal() && !lTyp.EqIgnoringRefinements(typeExprU64) {
			return fmt.Errorf("check: limit count %q type %q not compatible with u64",
				lhs.String(q.tm), lTyp.String(q.tm))
		}
		rhs := n.RHS().Expr()
		if err := q.tcheckExpr(rhs, depth); err != nil {
			return err
		}
		if rhs.Impure() {
			return fmt.Errorf("check: limit arg %q is not pure", rhs.String(q.tm))
		}
		rTyp := rhs.MType()
		if rTyp.Decorator() != 0 || (rTyp.Name().Key() != t.KeyReader1 && rTyp.Name().Key() != t.KeyWriter1) {
			return fmt.Errorf(`check: limit arg %q type %q not "reader1" or "writer1"`,
				rhs.String(q.tm), rTyp.String(q.tm))
		}
		n.SetMType(rTyp)
		return nil
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for tcheckExprOther", n.ID0().Key())
}

func isInSrc(tm *t.Map, n *a.Expr, methodName t.Key, nArgs int) bool {
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1().Key() != methodName {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName("src") {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyIn
}

func isInDst(tm *t.Map, n *a.Expr, methodName t.Key, nArgs int) bool {
	// TODO: check that n.Args() is "(x:bar)".
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1().Key() != methodName {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName("dst") {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyIn
}

func isThisMethod(tm *t.Map, n *a.Expr, methodName string, nArgs int) bool {
	// TODO: check that n.Args() is "(src:in.src)".
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName(methodName) {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyThis
}

func isLowHighBits(tm *t.Map, n *a.Expr, methodName t.Key) bool {
	// TODO: check that n.Args() is "(n:bar)".
	if n.ID0().Key() != t.KeyOpenParen || n.CallImpure() || len(n.Args()) != 1 {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1().Key() == methodName
}

func isSetLiteralWidth(tm *t.Map, n *a.Expr) bool {
	// TODO: check that n.Args() is "(lw:bar)".
	if n.ID0().Key() != t.KeyOpenParen || n.CallImpure() || len(n.Args()) != 1 {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1() == tm.ByName("set_literal_width")
}

func isDecode(tm *t.Map, n *a.Expr) bool {
	// TODO: check that n.Args() is "(dst:bar, src:baz)".
	if n.ID0().Key() != t.KeyOpenParen || !n.CallSuspendible() || len(n.Args()) != 2 {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1() == tm.ByName("decode")
}

func (q *checker) tcheckDot(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := q.tcheckExpr(lhs, depth); err != nil {
		return err
	}
	lTyp := lhs.MType()
	for ; lTyp.Decorator().Key() == t.KeyPtr; lTyp = lTyp.Inner() {
	}

	if lTyp.Decorator() != 0 {
		// TODO.
		return fmt.Errorf("check: unsupported decorator for tcheckDot")
	}

	s := (*a.Struct)(nil)
	if q.f.Func != nil {
		switch name := lTyp.Name(); name.Key() {
		case t.KeyIn:
			s = q.f.Func.In()
		case t.KeyOut:
			s = q.f.Func.Out()
		case t.KeyReader1, t.KeyWriter1:
			// TODO: remove this hack and be more principled about the built-in
			// buf1, reader1, writer1 types.
			//
			// Another hack is using typeExprPlaceholder until a TypeExpr can
			// represent function types.
			n.SetMType(typeExprPlaceholder)
			return nil
		default:
			s = q.c.structs[name].Struct
		}
	}
	if s == nil {
		return fmt.Errorf("check: no struct type %q found for expression %q",
			lTyp.Name().String(q.tm), lhs.String(q.tm))
	}

	for _, field := range s.Fields() {
		f := field.Field()
		if f.Name() == n.ID1() {
			n.SetMType(f.XType())
			return nil
		}
	}

	if f := q.c.funcs[t.QID{lTyp.Name(), n.ID1()}]; f.Func != nil {
		n.SetMType(typeExprPlaceholder) // HACK.
		return nil
	}

	return fmt.Errorf("check: no field or method named %q found in struct type %q for expression %q",
		n.ID1().String(q.tm), lTyp.Name().String(q.tm), n.String(q.tm))
}

func (q *checker) tcheckExprUnaryOp(n *a.Expr, depth uint32) error {
	rhs := n.RHS().Expr()
	if err := q.tcheckExpr(rhs, depth); err != nil {
		return err
	}
	rTyp := rhs.MType()

	switch n.ID0().Key() {
	case t.KeyXUnaryPlus, t.KeyXUnaryMinus:
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: unary %q: %q, of type %q, does not have a numeric type",
				n.ID0().AmbiguousForm().String(q.tm), rhs.String(q.tm), rTyp.String(q.tm))
		}
		if cv := rhs.ConstValue(); cv != nil {
			if n.ID0().Key() == t.KeyXUnaryMinus {
				cv = neg(cv)
			}
			n.SetConstValue(cv)
		}
		n.SetMType(rTyp.Unrefined())
		return nil

	case t.KeyXUnaryNot:
		if !rTyp.IsBool() {
			return fmt.Errorf("check: unary %q: %q, of type %q, does not have a boolean type",
				n.ID0().AmbiguousForm().String(q.tm), rhs.String(q.tm), rTyp.String(q.tm))
		}
		if cv := rhs.ConstValue(); cv != nil {
			n.SetConstValue(btoi(cv.Cmp(zero) == 0))
		}
		n.SetMType(typeExprBool)
		return nil
	}
	return fmt.Errorf("check: unrecognized token.Key (0x%X) for tcheckExprUnaryOp", n.ID0().Key())
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
		if lTyp.IsNumTypeOrIdeal() && rhs.IsNumType() {
			n.SetMType(rhs)
			return nil
		}
		return fmt.Errorf("check: cannot convert expression %q, of type %q, as type %q",
			lhs.String(q.tm), lTyp.String(q.tm), rhs.String(q.tm))
	}
	rhs := n.RHS().Expr()
	if err := q.tcheckExpr(rhs, depth); err != nil {
		return err
	}
	rTyp := rhs.MType()

	switch op.Key() {
	default:
		if !lTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				op.AmbiguousForm().String(q.tm), lhs.String(q.tm), lTyp.String(q.tm))
		}
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				op.AmbiguousForm().String(q.tm), rhs.String(q.tm), rTyp.String(q.tm))
		}
	case t.KeyXBinaryNotEq, t.KeyXBinaryEqEq:
		// No-op.
	case t.KeyXBinaryAnd, t.KeyXBinaryOr:
		if !lTyp.IsBool() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().String(q.tm), lhs.String(q.tm), lTyp.String(q.tm))
		}
		if !rTyp.IsBool() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().String(q.tm), rhs.String(q.tm), rTyp.String(q.tm))
		}
	}

	switch op.Key() {
	default:
		if !lTyp.EqIgnoringRefinements(rTyp) && !lTyp.IsIdeal() && !rTyp.IsIdeal() {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q, do not have compatible types",
				op.AmbiguousForm().String(q.tm),
				lhs.String(q.tm), rhs.String(q.tm),
				lTyp.String(q.tm), rTyp.String(q.tm),
			)
		}
	case t.KeyXBinaryShiftL, t.KeyXBinaryShiftR:
		if lTyp.IsIdeal() && !rTyp.IsIdeal() {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q; "+
				"cannot shift an ideal number by a non-ideal number",
				op.AmbiguousForm().String(q.tm),
				lhs.String(q.tm), rhs.String(q.tm),
				lTyp.String(q.tm), rTyp.String(q.tm),
			)
		}
	}

	if lcv, rcv := lhs.ConstValue(), rhs.ConstValue(); lcv != nil && rcv != nil {
		ncv, err := evalConstValueBinaryOp(q.tm, n, lcv, rcv)
		if err != nil {
			return err
		}
		n.SetConstValue(ncv)
	}

	if comparisonOps[0xFF&op.Key()] {
		n.SetMType(typeExprBool)
	} else if !lTyp.IsIdeal() {
		n.SetMType(lTyp.Unrefined())
	} else {
		n.SetMType(rTyp.Unrefined())
	}

	return nil
}

func evalConstValueBinaryOp(tm *t.Map, n *a.Expr, l *big.Int, r *big.Int) (*big.Int, error) {
	switch n.ID0().Key() {
	case t.KeyXBinaryPlus:
		return big.NewInt(0).Add(l, r), nil
	case t.KeyXBinaryMinus:
		return big.NewInt(0).Sub(l, r), nil
	case t.KeyXBinaryStar:
		return big.NewInt(0).Mul(l, r), nil
	case t.KeyXBinarySlash:
		if r.Cmp(zero) == 0 {
			return nil, fmt.Errorf("check: division by zero in const expression %q", n.String(tm))
		}
		// TODO: decide on Euclidean division vs other definitions. See "go doc
		// math/big int.divmod" for details.
		return big.NewInt(0).Div(l, r), nil
	case t.KeyXBinaryShiftL:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return nil, fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().String(tm), n.String(tm))
		}
		return big.NewInt(0).Lsh(l, uint(r.Uint64())), nil
	case t.KeyXBinaryShiftR:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return nil, fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().String(tm), n.String(tm))
		}
		return big.NewInt(0).Rsh(l, uint(r.Uint64())), nil
	case t.KeyXBinaryAmp:
		return big.NewInt(0).And(l, r), nil
	case t.KeyXBinaryAmpHat:
		return big.NewInt(0).AndNot(l, r), nil
	case t.KeyXBinaryPipe:
		return big.NewInt(0).Or(l, r), nil
	case t.KeyXBinaryHat:
		return big.NewInt(0).Xor(l, r), nil
	case t.KeyXBinaryNotEq:
		return btoi(l.Cmp(r) != 0), nil
	case t.KeyXBinaryLessThan:
		return btoi(l.Cmp(r) < 0), nil
	case t.KeyXBinaryLessEq:
		return btoi(l.Cmp(r) <= 0), nil
	case t.KeyXBinaryEqEq:
		return btoi(l.Cmp(r) == 0), nil
	case t.KeyXBinaryGreaterEq:
		return btoi(l.Cmp(r) >= 0), nil
	case t.KeyXBinaryGreaterThan:
		return btoi(l.Cmp(r) > 0), nil
	case t.KeyXBinaryAnd:
		return btoi((l.Cmp(zero) != 0) && (r.Cmp(zero) != 0)), nil
	case t.KeyXBinaryOr:
		return btoi((l.Cmp(zero) != 0) || (r.Cmp(zero) != 0)), nil
	}
	return nil, fmt.Errorf("check: unrecognized token.Key (0x%02X) for evalConstValueBinaryOp", n.ID0().Key())
}

func (q *checker) tcheckExprAssociativeOp(n *a.Expr, depth uint32) error {
	switch n.ID0().Key() {
	case t.KeyXAssociativePlus, t.KeyXAssociativeStar,
		t.KeyXAssociativeAmp, t.KeyXAssociativePipe, t.KeyXAssociativeHat:

		expr, typ := (*a.Expr)(nil), (*a.TypeExpr)(nil)
		for _, o := range n.Args() {
			o := o.Expr()
			if err := q.tcheckExpr(o, depth); err != nil {
				return err
			}
			oTyp := o.MType()
			if oTyp.IsIdeal() {
				continue
			}
			if !oTyp.IsNumType() {
				return fmt.Errorf("check: associative %q: %q, of type %q, does not have a numeric type",
					n.ID0().AmbiguousForm().String(q.tm), o.String(q.tm), oTyp.String(q.tm))
			}
			if typ == nil {
				expr, typ = o, oTyp.Unrefined()
				continue
			}
			if !typ.EqIgnoringRefinements(oTyp) {
				return fmt.Errorf("check: associative %q: %q and %q, of types %q and %q, "+
					"do not have compatible types",
					n.ID0().AmbiguousForm().String(q.tm),
					expr.String(q.tm), o.String(q.tm),
					expr.MType().String(q.tm), o.MType().String(q.tm))
			}
		}
		if typ == nil {
			typ = typeExprIdeal
		}
		n.SetMType(typ)
		return nil

	case t.KeyXAssociativeAnd, t.KeyXAssociativeOr:
		for _, o := range n.Args() {
			o := o.Expr()
			if err := q.tcheckExpr(o, depth); err != nil {
				return err
			}
			if !o.MType().IsBool() {
				return fmt.Errorf("check: associative %q: %q, of type %q, does not have a boolean type",
					n.ID0().AmbiguousForm().String(q.tm), o.String(q.tm), o.MType().String(q.tm))
			}
		}
		n.SetMType(typeExprBool)
		return nil
	}

	return fmt.Errorf("check: unrecognized token.Key (0x%X) for tcheckExprAssociativeOp", n.ID0().Key())
}

func (q *checker) tcheckTypeExpr(n *a.TypeExpr, depth uint32) error {
	if depth > a.MaxTypeExprDepth {
		return fmt.Errorf("check: type expression recursion depth too large")
	}
	depth++

swtch:
	switch n.Decorator().Key() {
	case 0:
		if n.Name().IsNumType() {
			for _, b := range n.Bounds() {
				if b == nil {
					continue
				}
				if err := q.tcheckExpr(b, 0); err != nil {
					return err
				}
				if b.ConstValue() == nil {
					return fmt.Errorf("check: %q is not constant", b.String(q.tm))
				}
			}
			break
		}
		if n.Min() != nil || n.Max() != nil {
			// TODO: reject. You can only refine numeric types.
		}
		if name := n.Name().Key(); name == t.KeyBool || name == t.KeyReader1 || name == t.KeyWriter1 {
			break
		}
		for _, s := range q.c.structs {
			if s.ID == n.Name() {
				break swtch
			}
		}
		return fmt.Errorf("check: %q is not a type", n.Name().String(q.tm))

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
			return fmt.Errorf("check: %q is not constant", aLen.String(q.tm))
		}
		if err := q.tcheckTypeExpr(n.Inner(), depth); err != nil {
			return err
		}

	default:
		return fmt.Errorf("check: unrecognized node for tcheckTypeExpr")
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
