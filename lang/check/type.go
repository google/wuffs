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
	"fmt"
	"math/big"

	"github.com/google/wuffs/lang/builtin"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
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

		case a.KIterate:
			if err := q.tcheckVars(o.Iterate().Variables()); err != nil {
				return err
			}
			if err := q.tcheckVars(o.Iterate().Body()); err != nil {
				return err
			}

		case a.KVar:
			o := o.Var()
			name := o.Name()
			if _, ok := q.f.LocalVars[name]; ok {
				return fmt.Errorf("check: duplicate var %q", name.Str(q.tm))
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
					cond.Str(q.tm), cond.MType().Str(q.tm))
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

	case a.KIterate:
		n := n.Iterate()
		unroll := n.UnrollCount()
		if err := q.tcheckExpr(unroll, 0); err != nil {
			return err
		}
		if cv := unroll.ConstValue(); cv == nil {
			return fmt.Errorf("check: unroll count %q is not constant", unroll.Str(q.tm))
		}
		for _, o := range n.Variables() {
			if err := q.tcheckStatement(o); err != nil {
				return err
			}
		}
		if err := q.tcheckLoop(n); err != nil {
			return err
		}

	case a.KJump:
		n := n.Jump()
		jumpTarget := (a.Loop)(nil)
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
				sepStr, labelStr = ":", id.Str(q.tm)
			}
			return fmt.Errorf("no matching while/iterate statement for %s%s%s",
				n.Keyword().Str(q.tm), sepStr, labelStr)
		}
		if n.Keyword().Key() == t.KeyBreak {
			jumpTarget.SetHasBreak()
		} else {
			jumpTarget.SetHasContinue()
		}
		n.SetJumpTarget(jumpTarget)

	case a.KRet:
		n := n.Ret()
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
			return fmt.Errorf("check: internal error: unchecked type expression %q", n.XType().Str(q.tm))
		}
		if value := n.Value(); value != nil {
			if err := q.tcheckExpr(value, 0); err != nil {
				return err
			}
			lTyp := n.XType()
			rTyp := value.MType()
			if n.IterateVariable() {
				if lTyp.Decorator().Key() != t.KeyPtr {
					return fmt.Errorf("check: iterate variable %q, of type %q, does not have a pointer type",
						n.Name().Str(q.tm), lTyp.Str(q.tm))
				}
				if rTyp.Decorator().Key() != t.KeyColon {
					return fmt.Errorf("check: iterate range %q, of type %q, does not have a slice type",
						value.Str(q.tm), rTyp.Str(q.tm))
				}
				if !lTyp.Inner().Eq(rTyp.Inner()) {
					return fmt.Errorf("check: cannot iterate %q of type %q over %q of type %q "+
						"as their inner types don't match",
						n.Name().Str(q.tm), lTyp.Str(q.tm), value.Str(q.tm), rTyp.Str(q.tm))
				}

			} else if (rTyp.IsIdeal() && lTyp.IsNumType()) || lTyp.EqIgnoringRefinements(rTyp) {
				// No-op.

			} else {
				return fmt.Errorf("check: cannot assign %q of type %q to %q of type %q",
					value.Str(q.tm), rTyp.Str(q.tm), n.Name().Str(q.tm), lTyp.Str(q.tm))
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
				cond.Str(q.tm), cond.MType().Str(q.tm))
		}
		if err := q.tcheckLoop(n); err != nil {
			return err
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
			cond.Str(q.tm), cond.MType().Str(q.tm))
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
			rhs.Str(q.tm), rTyp.Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
	}

	if !lTyp.IsNumType() {
		return fmt.Errorf("check: assignment %q: assignee %q, of type %q, does not have numeric type",
			n.Operator().Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
	}

	switch n.Operator().Key() {
	case t.KeyShiftLEq, t.KeyShiftREq:
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: assignment %q: shift %q, of type %q, does not have numeric type",
				n.Operator().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
		return nil
	case t.KeyTildePlusEq:
		if !lTyp.IsUnsignedInteger() {
			return fmt.Errorf("check: assignment %q: %q, of type %q, does not have unsigned integer type",
				n.Operator().Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		}
	}

	if rTyp.IsIdeal() || lTyp.EqIgnoringRefinements(rTyp) {
		return nil
	}
	return fmt.Errorf("check: assignment %q: %q and %q, of types %q and %q, do not have compatible types",
		n.Operator().Str(q.tm),
		lhs.Str(q.tm), rhs.Str(q.tm),
		lTyp.Str(q.tm), rTyp.Str(q.tm),
	)
}

func (q *checker) tcheckArg(n *a.Arg, depth uint32) error {
	if err := q.tcheckExpr(n.Value(), depth); err != nil {
		return err
	}
	n.Node().SetTypeChecked()
	return nil
}

func (q *checker) tcheckLoop(n a.Loop) error {
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
			s := id1.Str(q.tm)
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
			return fmt.Errorf("check: unrecognized identifier %q", id1.Str(q.tm))
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

	case t.KeyOpenParen, t.KeyTry:
		// n is a function call.

		// TODO: be consistent about type-checking n.LHS().Expr() or
		// n.LHS().Expr().LHS().Expr(). Doing this properly will probably
		// require a TypeExpr being able to express function and method types.

		// TODO: delete this hack that only matches "in.src.read_u8?()" etc.
		if isInSrc(q.tm, n, t.KeyReadU8, 0) || isInSrc(q.tm, n, t.KeyUnreadU8, 0) ||
			isInSrc(q.tm, n, t.KeyReadU16BE, 0) || isInSrc(q.tm, n, t.KeyReadU32BE, 0) ||
			isInSrc(q.tm, n, t.KeyReadU32BE, 0) || isInSrc(q.tm, n, t.KeyReadU32LE, 0) ||
			isInSrc(q.tm, n, t.KeySkip32, 1) || isInSrc(q.tm, n, t.KeySinceMark, 0) ||
			isInSrc(q.tm, n, t.KeyMark, 0) || isInSrc(q.tm, n, t.KeyLimit, 1) ||
			isInDst(q.tm, n, t.KeyCopyFromSlice, 1) || isInDst(q.tm, n, t.KeyCopyFromSlice32, 2) ||
			isInDst(q.tm, n, t.KeyCopyFromReader32, 2) || isInDst(q.tm, n, t.KeyCopyFromHistory32, 2) ||
			isInDst(q.tm, n, t.KeyWriteU8, 1) || isInDst(q.tm, n, t.KeySinceMark, 0) ||
			isInDst(q.tm, n, t.KeyMark, 0) || isInDst(q.tm, n, t.KeyIsMarked, 0) ||
			isThatMethod(q.tm, n, t.KeyMark, 0) || isThatMethod(q.tm, n, t.KeyLimit, 1) ||
			isThatMethod(q.tm, n, t.KeySinceMark, 0) {

			if err := q.tcheckExpr(n.LHS().Expr(), depth); err != nil {
				return err
			}
			for _, o := range n.Args() {
				if err := q.tcheckArg(o.Arg(), depth); err != nil {
					return err
				}
			}
			if n.ID0().Key() == t.KeyTry {
				n.SetMType(typeExprStatus)
			} else if isInSrc(q.tm, n, t.KeyReadU32BE, 0) || isInSrc(q.tm, n, t.KeyReadU32LE, 0) {
				n.SetMType(typeExprPlaceholder32) // HACK.
			} else if isInSrc(q.tm, n, t.KeyReadU16BE, 0) || isInSrc(q.tm, n, t.KeyReadU16LE, 0) {
				n.SetMType(typeExprPlaceholder16) // HACK.
			} else if isInSrc(q.tm, n, t.KeySinceMark, 0) || isInDst(q.tm, n, t.KeySinceMark, 0) {
				n.SetMType(typeExprSliceU8) // HACK.
			} else if isInDst(q.tm, n, t.KeyCopyFromSlice, 1) {
				n.SetMType(typeExprU64) // HACK.
			} else if isInDst(q.tm, n, t.KeyCopyFromSlice32, 2) || isInDst(q.tm, n, t.KeyCopyFromReader32, 2) ||
				isInDst(q.tm, n, t.KeyCopyFromHistory32, 2) {
				n.SetMType(typeExprU32) // HACK.
			} else if isInDst(q.tm, n, t.KeyIsMarked, 0) {
				n.SetMType(typeExprBool) // HACK.
			} else {
				n.SetMType(typeExprPlaceholder) // HACK.
			}
			return nil
		}
		// TODO: delete this hack that only matches "foo.bar_bits(etc)".
		if isThatMethod(q.tm, n, t.KeyLowBits, 1) || isThatMethod(q.tm, n, t.KeyHighBits, 1) {
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
		// TODO: delete this hack that only matches "foo.is_suspension(etc)".
		if isThatMethod(q.tm, n, t.KeyIsError, 0) ||
			isThatMethod(q.tm, n, t.KeyIsOK, 0) ||
			isThatMethod(q.tm, n, t.KeyIsSuspension, 0) {
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
			n.SetMType(typeExprBool)
			return nil
		}
		// TODO: delete this hack that only matches "foo.suffix(etc)".
		if isThatMethod(q.tm, n, t.KeySuffix, 1) {
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
			// TODO: don't assume that the slice is a slice of u8.
			n.SetMType(typeExprSliceU8)
			return nil
		}
		// TODO: delete this hack that only matches "foo.set_literal_width(etc)".
		if isThatMethod(q.tm, n, q.tm.ByName("set_literal_width").Key(), 1) ||
			isThatMethod(q.tm, n, q.tm.ByName("decode").Key(), 2) ||
			isThatMethod(q.tm, n, q.tm.ByName("decode").Key(), 3) {
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
			if n.ID0().Key() == t.KeyTry {
				// TODO: be more principled about inferring "try etc"'s type.
				n.SetMType(typeExprStatus)
			} else {
				n.SetMType(typeExprPlaceholder) // HACK.
			}
			return nil
		}
		// TODO: delete this hack that only matches "foo.length(etc)".
		if isThatMethod(q.tm, n, t.KeyCopyFromSlice, 1) || isThatMethod(q.tm, n, t.KeyLength, 0) ||
			isThatMethod(q.tm, n, t.KeyAvailable, 0) {
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
			n.SetMType(typeExprU64)
			return nil
		}
		// TODO: delete this hack that only matches "foo.update(etc)".
		if isThatMethod(q.tm, n, q.tm.ByName("update").Key(), 1) {
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
			n.SetMType(typeExprU32)
			return nil
		}

		return q.tcheckExprCall(n, depth)

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
		if key := lTyp.Decorator().Key(); key != t.KeyOpenBracket && key != t.KeyColon {
			return fmt.Errorf("check: %s is an index expression but %s has type %s, not an array or slice type",
				n.Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		}
		rTyp := rhs.MType()
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: %s is an index expression but %s has type %s, not a numeric type",
				n.Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
		n.SetMType(lTyp.Inner())
		return nil

	case t.KeyColon:
		// n is a slice.
		// TODO: require that the i and j in a[i:j] are *unsigned* (or
		// non-negative constants)?
		if mhs := n.MHS().Expr(); mhs != nil {
			if err := q.tcheckExpr(mhs, depth); err != nil {
				return err
			}
			mTyp := mhs.MType()
			if !mTyp.IsNumTypeOrIdeal() {
				return fmt.Errorf("check: %s is a slice expression but %s has type %s, not a numeric type",
					n.Str(q.tm), mhs.Str(q.tm), mTyp.Str(q.tm))
			}
		}
		if rhs := n.RHS().Expr(); rhs != nil {
			if err := q.tcheckExpr(rhs, depth); err != nil {
				return err
			}
			rTyp := rhs.MType()
			if !rTyp.IsNumTypeOrIdeal() {
				return fmt.Errorf("check: %s is a slice expression but %s has type %s, not a numeric type",
					n.Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
			}
		}
		lhs := n.LHS().Expr()
		if err := q.tcheckExpr(lhs, depth); err != nil {
			return err
		}
		lTyp := lhs.MType()
		switch lTyp.Decorator().Key() {
		default:
			return fmt.Errorf("check: %s is a slice expression but %s has type %s, not an array or slice type",
				n.Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		case t.KeyOpenBracket:
			n.SetMType(a.NewTypeExpr(t.IDColon, 0, nil, nil, lTyp.Inner()))
		case t.KeyColon:
			n.SetMType(lTyp)
		}
		return nil

	case t.KeyDot:
		return q.tcheckDot(n, depth)

	case t.KeyError, t.KeyStatus, t.KeySuspension:
		nominalKeyword := n.ID0()
		declaredKeyword := t.ID(0)
		if s, ok := q.c.statuses[n.ID1()]; ok {
			declaredKeyword = s.Status.Keyword()
		} else {
			msg, _ := t.Unescape(n.ID1().Str(q.tm))
			z, ok := builtin.StatusMap[msg]
			if !ok {
				return fmt.Errorf("check: no error or status with message %q", msg)
			}
			declaredKeyword = z.Keyword
		}
		if nominalKeyword != declaredKeyword {
			return fmt.Errorf("check: status literal says %q but declaration says %q",
				nominalKeyword.Str(q.tm), declaredKeyword.Str(q.tm))
		}
		n.SetMType(typeExprStatus)
		return nil

	case t.KeyDollar:
		for _, o := range n.Args() {
			o := o.Expr()
			if err := q.tcheckExpr(o, depth); err != nil {
				return err
			}
		}
		n.SetMType(typeExprList)
		return nil
	}

	return fmt.Errorf("check: unrecognized token.Key (0x%X) in expression %q for tcheckExprOther",
		n.ID0().Key(), n.Str(q.tm))
}

func (q *checker) tcheckExprCall(n *a.Expr, depth uint32) error {
	if err := q.tcheckExpr(n.LHS().Expr(), depth); err != nil {
		return err
	}
	lTyp := n.LHS().Expr().MType()
	// TODO: look up funcs in used packages. Factor out a resolveFunc method?
	f := q.c.funcs[t.QID{lTyp.Receiver().Name(), lTyp.Name()}]
	if f.Func == nil {
		return fmt.Errorf("check: cannot look up %q", lTyp.Str(q.tm))
	}

	// Check that the func's in type matches the arguments.
	inFields := f.Func.In().Fields()
	if len(inFields) != len(n.Args()) {
		return fmt.Errorf("check: %q has %d arguments but %d were given",
			lTyp.Str(q.tm), len(inFields), len(n.Args()))
	}
	for _, o := range n.Args() {
		// TODO: tcheckArg should take inFields[i], checking both assignability
		// of types and that the "name" in the arg "name:val" matches the field
		// name.
		if err := q.tcheckArg(o.Arg(), depth); err != nil {
			return err
		}
	}

	// TODO: distinguish t.KeyOpenParen vs t.KeyTry in a more principled way?
	//
	// TODO: figure out calls with and without "?" should interact with the out
	// type.
	if n.ID0().Key() == t.KeyTry {
		n.SetMType(typeExprStatus)
	} else {
		// TODO: figure out how to translate f.Func.Out(), which is an
		// *ast.Struct, to an *ast.TypeExpr we can pass to n.SetMType.
		n.SetMType(typeExprPlaceholder) // HACK.
	}
	return nil
}

func isInSrc(tm *t.Map, n *a.Expr, methodName t.Key, nArgs int) bool {
	callSuspendible := methodName != t.KeySinceMark &&
		methodName != t.KeyMark &&
		methodName != t.KeyLimit
	if n.ID0().Key() != t.KeyOpenParen || n.CallSuspendible() != callSuspendible || len(n.Args()) != nArgs {
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
	callSuspendible := methodName != t.KeyCopyFromReader32 &&
		methodName != t.KeyCopyFromHistory32 &&
		methodName != t.KeyCopyFromSlice32 &&
		methodName != t.KeyCopyFromSlice &&
		methodName != t.KeySinceMark &&
		methodName != t.KeyIsMarked &&
		methodName != t.KeyMark &&
		methodName != t.KeyLimit
	// TODO: check that n.Args() is "(x:bar)".
	if n.ID0().Key() != t.KeyOpenParen || n.CallSuspendible() != callSuspendible || len(n.Args()) != nArgs {
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
	if k := n.ID0().Key(); k != t.KeyOpenParen && k != t.KeyTry {
		return false
	}
	if len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot || n.ID1() != tm.ByName(methodName) {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0() == 0 && n.ID1().Key() == t.KeyThis
}

// isThatMethod is like isThisMethod but for foo.bar(etc), not this.bar(etc).
func isThatMethod(tm *t.Map, n *a.Expr, methodName t.Key, nArgs int) bool {
	if k := n.ID0().Key(); k != t.KeyOpenParen && k != t.KeyTry {
		return false
	}
	if len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	return n.ID0().Key() == t.KeyDot && n.ID1().Key() == methodName
}

func (q *checker) tcheckDot(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := q.tcheckExpr(lhs, depth); err != nil {
		return err
	}
	lTyp := lhs.MType().Pointee()

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
			lTyp.Name().Str(q.tm), lhs.Str(q.tm))
	}

	for _, field := range s.Fields() {
		f := field.Field()
		if f.Name() == n.ID1() {
			n.SetMType(f.XType())
			return nil
		}
	}

	if f := q.c.funcs[t.QID{lTyp.Name(), n.ID1()}]; f.Func != nil {
		n.SetMType(a.NewTypeExpr(t.IDOpenParen, n.ID1(), lTyp.Node(), nil, nil))
		return nil
	}

	return fmt.Errorf("check: no field or method named %q found in struct type %q for expression %q",
		n.ID1().Str(q.tm), lTyp.Name().Str(q.tm), n.Str(q.tm))
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
				n.ID0().AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
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
				n.ID0().AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
		if cv := rhs.ConstValue(); cv != nil {
			n.SetConstValue(btoi(cv.Cmp(zero) == 0))
		}
		n.SetMType(typeExprBool)
		return nil

	case t.KeyXUnaryRef:
		// TODO.

	case t.KeyXUnaryDeref:
		if rTyp.Decorator().Key() != t.KeyPtr { // TODO: t.KeyNptr?
			return fmt.Errorf("check: %q is a dereference of a non-pointer type %q",
				n.Str(q.tm), rTyp.Str(q.tm))
		}
		n.SetMType(rTyp.Inner())
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
			lhs.Str(q.tm), lTyp.Str(q.tm), rhs.Str(q.tm))
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
				op.AmbiguousForm().Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		}
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				op.AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
	case t.KeyXBinaryNotEq, t.KeyXBinaryEqEq:
		// No-op.
	case t.KeyXBinaryAnd, t.KeyXBinaryOr:
		if !lTyp.IsBool() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		}
		if !rTyp.IsBool() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
	}

	switch op.Key() {
	default:
		if !lTyp.EqIgnoringRefinements(rTyp) && !lTyp.IsIdeal() && !rTyp.IsIdeal() {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q, do not have compatible types",
				op.AmbiguousForm().Str(q.tm),
				lhs.Str(q.tm), rhs.Str(q.tm),
				lTyp.Str(q.tm), rTyp.Str(q.tm),
			)
		}
	case t.KeyXBinaryShiftL, t.KeyXBinaryShiftR:
		if lTyp.IsIdeal() && !rTyp.IsIdeal() {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q; "+
				"cannot shift an ideal number by a non-ideal number",
				op.AmbiguousForm().Str(q.tm),
				lhs.Str(q.tm), rhs.Str(q.tm),
				lTyp.Str(q.tm), rTyp.Str(q.tm),
			)
		}
	}

	switch op.Key() {
	case t.KeyXBinaryTildePlus:
		typ := lTyp
		if typ.IsIdeal() {
			typ = rTyp
			if typ.IsIdeal() {
				return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q, do not have non-ideal types",
					op.AmbiguousForm().Str(q.tm),
					lhs.Str(q.tm), rhs.Str(q.tm),
					lTyp.Str(q.tm), rTyp.Str(q.tm),
				)
			}
		}
		if !typ.IsUnsignedInteger() {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q, do not have unsigned integer types",
				op.AmbiguousForm().Str(q.tm),
				lhs.Str(q.tm), rhs.Str(q.tm),
				lTyp.Str(q.tm), rTyp.Str(q.tm),
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
			return nil, fmt.Errorf("check: division by zero in const expression %q", n.Str(tm))
		}
		// TODO: decide on Euclidean division vs other definitions. See "go doc
		// math/big int.divmod" for details.
		return big.NewInt(0).Div(l, r), nil
	case t.KeyXBinaryShiftL:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return nil, fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().Str(tm), n.Str(tm))
		}
		return big.NewInt(0).Lsh(l, uint(r.Uint64())), nil
	case t.KeyXBinaryShiftR:
		if r.Cmp(zero) < 0 || r.Cmp(ffff) > 0 {
			return nil, fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().Str(tm), n.Str(tm))
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
	case t.KeyXBinaryPercent:
		if r.Cmp(zero) == 0 {
			return nil, fmt.Errorf("check: division by zero in const expression %q", n.Str(tm))
		}
		return big.NewInt(0).Mod(l, r), nil
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
	case t.KeyXBinaryTildePlus:
		return nil, fmt.Errorf("check: cannot apply ~+ operator to ideal numbers")
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
					n.ID0().AmbiguousForm().Str(q.tm), o.Str(q.tm), oTyp.Str(q.tm))
			}
			if typ == nil {
				expr, typ = o, oTyp.Unrefined()
				continue
			}
			if !typ.EqIgnoringRefinements(oTyp) {
				return fmt.Errorf("check: associative %q: %q and %q, of types %q and %q, "+
					"do not have compatible types",
					n.ID0().AmbiguousForm().Str(q.tm),
					expr.Str(q.tm), o.Str(q.tm),
					expr.MType().Str(q.tm), o.MType().Str(q.tm))
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
					n.ID0().AmbiguousForm().Str(q.tm), o.Str(q.tm), o.MType().Str(q.tm))
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
					return fmt.Errorf("check: %q is not constant", b.Str(q.tm))
				}
			}
			break
		}
		if n.Min() != nil || n.Max() != nil {
			// TODO: reject. You can only refine numeric types.
		}
		switch n.Name().Key() {
		case t.KeyBool, t.KeyStatus, t.KeyReader1, t.KeyWriter1:
			break swtch
		}
		for _, s := range q.c.structs {
			if s.ID == n.Name() {
				break swtch
			}
		}
		return fmt.Errorf("check: %q is not a type", n.Name().Str(q.tm))

	case t.KeyOpenBracket:
		aLen := n.ArrayLength()
		if err := q.tcheckExpr(aLen, 0); err != nil {
			return err
		}
		if aLen.ConstValue() == nil {
			return fmt.Errorf("check: %q is not constant", aLen.Str(q.tm))
		}
		fallthrough

	// TODO: also check t.KeyNptr? Where else should we look for nptr?
	case t.KeyPtr, t.KeyColon:
		if err := q.tcheckTypeExpr(n.Inner(), depth); err != nil {
			return err
		}

	default:
		// TODO: don't hard-code deflate.decoder as an acceptable type.
		if q.tm.ByID(n.Decorator()) == "deflate" && q.tm.ByID(n.Name()) == "decoder" {
			break
		}
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
