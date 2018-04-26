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
			if _, ok := q.localVars[name]; ok {
				return fmt.Errorf("check: duplicate var %q", name.Str(q.tm))
			}
			if err := q.tcheckTypeExpr(o.XType(), 0); err != nil {
				return err
			}
			q.localVars[name] = o.XType()

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
		if n.Keyword() == t.IDBreak {
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
				if lTyp.Decorator() != t.IDPtr {
					return fmt.Errorf("check: iterate variable %q, of type %q, does not have a pointer type",
						n.Name().Str(q.tm), lTyp.Str(q.tm))
				}
				if !rTyp.IsSliceType() {
					return fmt.Errorf("check: iterate range %q, of type %q, does not have a slice type",
						value.Str(q.tm), rTyp.Str(q.tm))
				}
				if !lTyp.Inner().Eq(rTyp.Inner()) {
					return fmt.Errorf("check: cannot iterate %q of type %q over %q of type %q "+
						"as their inner types don't match",
						n.Name().Str(q.tm), lTyp.Str(q.tm), value.Str(q.tm), rTyp.Str(q.tm))
				}
			} else if err := q.tcheckEq(n.Name(), nil, lTyp, value, rTyp); err != nil {
				return err
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

func (q *checker) tcheckEq(lID t.ID, lhs *a.Expr, lTyp *a.TypeExpr, rhs *a.Expr, rTyp *a.TypeExpr) error {
	if (rTyp.IsIdeal() && lTyp.IsNumType()) || lTyp.EqIgnoringRefinements(rTyp) {
		return nil
	}
	lStr := "???"
	if lID != 0 {
		lStr = lID.Str(q.tm)
	} else if lhs != nil {
		lStr = lhs.Str(q.tm)
	}
	return fmt.Errorf("check: cannot assign %q of type %q to %q of type %q",
		rhs.Str(q.tm), rTyp.Str(q.tm), lStr, lTyp.Str(q.tm))
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

	if n.Operator() == t.IDEq {
		return q.tcheckEq(0, lhs, lTyp, rhs, rTyp)
	}

	if !lTyp.IsNumType() {
		return fmt.Errorf("check: assignment %q: assignee %q, of type %q, does not have numeric type",
			n.Operator().Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
	}

	switch n.Operator() {
	case t.IDShiftLEq, t.IDShiftREq:
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: assignment %q: shift %q, of type %q, does not have numeric type",
				n.Operator().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
		return nil
	case t.IDTildeModPlusEq, t.IDTildeModMinusEq, t.IDTildeSatPlusEq, t.IDTildeSatMinusEq:
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

func (q *checker) tcheckArg(n *a.Arg, inField *a.Field, genericType *a.TypeExpr, depth uint32) error {
	if err := q.tcheckExpr(n.Value(), depth); err != nil {
		return err
	}

	if inField == nil {
		// TODO: panic.
		n.Node().SetTypeChecked()
		return nil
	}

	if n.Name() != inField.Name() {
		return fmt.Errorf("check: argument name: got %q, want %q", n.Name().Str(q.tm), inField.Name().Str(q.tm))
	}
	inFieldTyp := inField.XType()
	if genericType != nil && inFieldTyp.Eq(typeExprGeneric) {
		inFieldTyp = genericType
	}
	if err := q.tcheckEq(inField.Name(), nil, inFieldTyp, n.Value(), n.Value().MType()); err != nil {
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

	switch op := n.Operator(); {
	case op.IsXUnaryOp():
		if err := q.tcheckExprUnaryOp(n, depth); err != nil {
			return err
		}
	case op.IsXBinaryOp():
		if err := q.tcheckExprBinaryOp(n, depth); err != nil {
			return err
		}
	case op.IsXAssociativeOp():
		if err := q.tcheckExprAssociativeOp(n, depth); err != nil {
			return err
		}
	default:
		if err := q.tcheckExprOther(n, depth); err != nil {
			return err
		}
	}
	n.Node().SetTypeChecked()
	return nil
}

func (q *checker) tcheckExprOther(n *a.Expr, depth uint32) error {
	switch n.Operator() {
	case 0:
		id1 := n.Ident()
		if id1.IsNumLiteral(q.tm) {
			z := big.NewInt(0)
			s := id1.Str(q.tm)
			if _, ok := z.SetString(s, 0); !ok {
				return fmt.Errorf("check: invalid numeric literal %q", s)
			}
			n.SetConstValue(z)
			n.SetMType(typeExprIdeal)
			return nil

		} else if id1.IsIdent(q.tm) {
			if q.localVars != nil {
				if typ, ok := q.localVars[id1]; ok {
					n.SetMType(typ)
					return nil
				}
			}
			if c, ok := q.c.consts[t.QID{0, id1}]; ok {
				// TODO: check somewhere that a global ident (i.e. a const) is
				// not directly in the LHS of an assignment.
				n.SetGlobalIdent()
				n.SetMType(c.XType())
				return nil
			}
			// TODO: look for other (global) names: consts, funcs, statuses,
			// structs from used packages.
			return fmt.Errorf("check: unrecognized identifier %q", id1.Str(q.tm))
		}
		switch id1 {
		case t.IDFalse:
			n.SetConstValue(zero)
			n.SetMType(typeExprBool)
			return nil

		case t.IDTrue:
			n.SetConstValue(one)
			n.SetMType(typeExprBool)
			return nil

		case t.IDUnderscore:
			// TODO.

		case t.IDThis:
			// TODO.
		}

	case t.IDOpenParen, t.IDTry:
		// n is a function call.

		// TODO: be consistent about type-checking n.LHS().Expr() or
		// n.LHS().Expr().LHS().Expr(). Doing this properly will probably
		// require a TypeExpr being able to express function and method types.

		// TODO: delete this hack that only matches "foo.decode(etc)".
		if idDecode := q.tm.ByName("decode"); isThatMethod(q.tm, n, idDecode, 3) {
			foo := n.LHS().Expr().LHS().Expr()
			if err := q.tcheckExpr(foo, depth); err != nil {
				return err
			}
			n.LHS().SetTypeChecked()
			n.LHS().Expr().SetMType(a.NewTypeExpr(t.IDFunc, 0, idDecode, foo.MType().Node(), nil, nil))
			for _, o := range n.Args() {
				if err := q.tcheckArg(o.Arg(), nil, nil, depth); err != nil {
					return err
				}
			}
			if n.Operator() != t.IDTry {
				return fmt.Errorf("TODO: foo.decode(etc) outside of a try statement")
			}
			// TODO: be more principled about inferring "try etc"'s type.
			n.SetMType(typeExprStatus)
			return nil
		}

		return q.tcheckExprCall(n, depth)

	case t.IDOpenBracket:
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
		if key := lTyp.Decorator(); key != t.IDArray && key != t.IDSlice {
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

	case t.IDColon:
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
		switch lTyp.Decorator() {
		default:
			return fmt.Errorf("check: %s is a slice expression but %s has type %s, not an array or slice type",
				n.Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		case t.IDArray:
			n.SetMType(a.NewTypeExpr(t.IDSlice, 0, 0, nil, nil, lTyp.Inner()))
		case t.IDSlice:
			n.SetMType(lTyp)
		}
		return nil

	case t.IDDot:
		return q.tcheckDot(n, depth)

	case t.IDError, t.IDStatus, t.IDSuspension:
		nominalKeyword := n.Operator()
		declaredKeyword := t.ID(0)
		if s, ok := q.c.statuses[n.StatusQID()]; ok {
			declaredKeyword = s.Keyword()
		} else {
			msg, _ := t.Unescape(n.Ident().Str(q.tm))
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

	case t.IDDollar:
		for _, o := range n.Args() {
			o := o.Expr()
			if err := q.tcheckExpr(o, depth); err != nil {
				return err
			}
		}
		n.SetMType(typeExprList)
		return nil
	}

	return fmt.Errorf("check: unrecognized token (0x%X) in expression %q for tcheckExprOther",
		n.Operator(), n.Str(q.tm))
}

func (q *checker) tcheckExprCall(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := q.tcheckExpr(lhs, depth); err != nil {
		return err
	}
	f, err := q.c.resolveFunc(lhs.MType())
	if err != nil {
		return err
	}
	if ne, fe := n.Effect(), f.Effect(); ne != fe {
		return fmt.Errorf("check: %q has effect %q but %q has effect %q",
			n.Str(q.tm), ne, f.QQID().Str(q.tm), fe)
	}

	genericType := (*a.TypeExpr)(nil)
	if f.Receiver() == (t.QID{t.IDBase, t.IDDiamond}) {
		genericType = lhs.MType().Receiver()
	}

	// Check that the func's in type matches the arguments.
	inFields := f.In().Fields()
	if len(inFields) != len(n.Args()) {
		return fmt.Errorf("check: %q has %d arguments but %d were given",
			lhs.MType().Str(q.tm), len(inFields), len(n.Args()))
	}
	for i, o := range n.Args() {
		// TODO: inline tcheckArg here, after removing the special-cased hacks
		// in tcheckExprOther.
		if err := q.tcheckArg(o.Arg(), inFields[i].Field(), genericType, depth); err != nil {
			return err
		}
	}

	// TODO: distinguish t.IDOpenParen vs t.IDTry in a more principled way?
	//
	// TODO: figure out calls with and without "?" should interact with the out
	// type.
	if n.Operator() == t.IDTry {
		n.SetMType(typeExprStatus)
	} else {
		outFields := f.Out().Fields()
		switch len(outFields) {
		default:
			// TODO: figure out how to translate f.Out(), which is an
			// *ast.Struct, to an *ast.TypeExpr we can pass to n.SetMType.
			return fmt.Errorf("TODO: translate an *ast.Struct to an *ast.TypeExpr")
		case 0:
			n.SetMType(typeExprEmptyStruct)
		case 1:
			oTyp := outFields[0].Field().XType()
			if genericType != nil && oTyp.Eq(typeExprGeneric) {
				n.SetMType(genericType)
			} else {
				n.SetMType(oTyp)
			}
		}
	}
	return nil
}

// isThatMethod matches foo.methodName(etc) where etc has nArgs elements.
func isThatMethod(tm *t.Map, n *a.Expr, methodName t.ID, nArgs int) bool {
	if k := n.Operator(); k != t.IDOpenParen && k != t.IDTry {
		return false
	}
	if len(n.Args()) != nArgs {
		return false
	}
	n = n.LHS().Expr()
	if n.Operator() != t.IDDot || n.Ident() != methodName {
		return false
	}
	n = n.LHS().Expr()
	if n.Operator() == t.IDDot &&
		(n.Ident() == tm.ByName("src") || n.Ident() == tm.ByName("dst")) {
		return false
	}
	return true
}

func (q *checker) tcheckDot(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := q.tcheckExpr(lhs, depth); err != nil {
		return err
	}
	lTyp := lhs.MType().Pointee()
	lQID := lTyp.QID()
	qqid := t.QQID{lQID[0], lQID[1], n.Ident()}

	if lTyp.IsSliceType() {
		qqid[0] = t.IDBase
		qqid[1] = t.IDDiamond
		if f, err := q.c.builtInSliceFunc(qqid); err != nil {
			return err
		} else if f == nil {
			return fmt.Errorf("check: no slice method %q", n.Ident().Str(q.tm))
		}
		n.SetMType(a.NewTypeExpr(t.IDFunc, 0, n.Ident(), lTyp.Node(), nil, nil))
		return nil
	} else if lTyp.Decorator() != 0 {
		return fmt.Errorf("check: invalid type %q for dot-expression LHS %q", lTyp.Str(q.tm), lhs.Str(q.tm))
	}

	f, err := q.c.builtInFunc(qqid)
	if err != nil {
		return err
	} else if f == nil {
		f = q.c.funcs[qqid]
	}
	if f != nil {
		n.SetMType(a.NewTypeExpr(t.IDFunc, 0, n.Ident(), lTyp.Node(), nil, nil))
		return nil
	}

	s := (*a.Struct)(nil)
	if q.astFunc != nil && lQID[0] == 0 {
		switch lQID[1] {
		case t.IDIn:
			s = q.astFunc.In()
		case t.IDOut:
			s = q.astFunc.Out()
		}
	}
	if s == nil {
		s = q.c.structs[lQID]
		if s == nil {
			return fmt.Errorf("check: no struct type %q found for expression %q", lTyp.Str(q.tm), lhs.Str(q.tm))
		}
	}

	for _, field := range s.Fields() {
		f := field.Field()
		if f.Name() == n.Ident() {
			n.SetMType(f.XType())
			return nil
		}
	}

	return fmt.Errorf("check: no field or method named %q found in type %q for expression %q",
		n.Ident().Str(q.tm), lTyp.Str(q.tm), n.Str(q.tm))
}

func (q *checker) tcheckExprUnaryOp(n *a.Expr, depth uint32) error {
	rhs := n.RHS().Expr()
	if err := q.tcheckExpr(rhs, depth); err != nil {
		return err
	}
	rTyp := rhs.MType()

	switch n.Operator() {
	case t.IDXUnaryPlus, t.IDXUnaryMinus:
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: unary %q: %q, of type %q, does not have a numeric type",
				n.Operator().AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
		if cv := rhs.ConstValue(); cv != nil {
			if n.Operator() == t.IDXUnaryMinus {
				cv = neg(cv)
			}
			n.SetConstValue(cv)
		}
		n.SetMType(rTyp.Unrefined())
		return nil

	case t.IDXUnaryNot:
		if !rTyp.IsBool() {
			return fmt.Errorf("check: unary %q: %q, of type %q, does not have a boolean type",
				n.Operator().AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
		if cv := rhs.ConstValue(); cv != nil {
			n.SetConstValue(btoi(cv.Sign() == 0))
		}
		n.SetMType(typeExprBool)
		return nil

	case t.IDXUnaryRef:
		// TODO.

	case t.IDXUnaryDeref:
		if rTyp.Decorator() != t.IDPtr { // TODO: t.IDNptr?
			return fmt.Errorf("check: %q is a dereference of a non-pointer type %q",
				n.Str(q.tm), rTyp.Str(q.tm))
		}
		n.SetMType(rTyp.Inner())
		return nil
	}
	return fmt.Errorf("check: unrecognized token (0x%X) for tcheckExprUnaryOp", n.Operator())
}

func (q *checker) tcheckExprBinaryOp(n *a.Expr, depth uint32) error {
	lhs := n.LHS().Expr()
	if err := q.tcheckExpr(lhs, depth); err != nil {
		return err
	}
	lTyp := lhs.MType()
	op := n.Operator()
	if op == t.IDXBinaryAs {
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

	switch op {
	default:
		if !lTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				op.AmbiguousForm().Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		}
		if !rTyp.IsNumTypeOrIdeal() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a numeric type",
				op.AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
	case t.IDXBinaryNotEq, t.IDXBinaryEqEq:
		// No-op.
	case t.IDXBinaryAnd, t.IDXBinaryOr:
		if !lTyp.IsBool() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().Str(q.tm), lhs.Str(q.tm), lTyp.Str(q.tm))
		}
		if !rTyp.IsBool() {
			return fmt.Errorf("check: binary %q: %q, of type %q, does not have a boolean type",
				op.AmbiguousForm().Str(q.tm), rhs.Str(q.tm), rTyp.Str(q.tm))
		}
	}

	switch op {
	default:
		if !lTyp.EqIgnoringRefinements(rTyp) && !lTyp.IsIdeal() && !rTyp.IsIdeal() {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q, do not have compatible types",
				op.AmbiguousForm().Str(q.tm),
				lhs.Str(q.tm), rhs.Str(q.tm),
				lTyp.Str(q.tm), rTyp.Str(q.tm),
			)
		}
	case t.IDXBinaryShiftL, t.IDXBinaryShiftR:
		if lTyp.IsIdeal() && !rTyp.IsIdeal() {
			return fmt.Errorf("check: binary %q: %q and %q, of types %q and %q; "+
				"cannot shift an ideal number by a non-ideal number",
				op.AmbiguousForm().Str(q.tm),
				lhs.Str(q.tm), rhs.Str(q.tm),
				lTyp.Str(q.tm), rTyp.Str(q.tm),
			)
		}
	}

	switch op {
	case t.IDXBinaryTildeModPlus, t.IDXBinaryTildeModMinus,
		t.IDXBinaryTildeSatPlus, t.IDXBinaryTildeSatMinus:
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

	if comparisonOps[0xFF&op] {
		n.SetMType(typeExprBool)
	} else if !lTyp.IsIdeal() {
		n.SetMType(lTyp.Unrefined())
	} else {
		n.SetMType(rTyp.Unrefined())
	}

	return nil
}

func evalConstValueBinaryOp(tm *t.Map, n *a.Expr, l *big.Int, r *big.Int) (*big.Int, error) {
	switch n.Operator() {
	case t.IDXBinaryPlus:
		return big.NewInt(0).Add(l, r), nil
	case t.IDXBinaryMinus:
		return big.NewInt(0).Sub(l, r), nil
	case t.IDXBinaryStar:
		return big.NewInt(0).Mul(l, r), nil
	case t.IDXBinarySlash:
		if r.Sign() == 0 {
			return nil, fmt.Errorf("check: division by zero in const expression %q", n.Str(tm))
		}
		// TODO: decide on Euclidean division vs other definitions. See "go doc
		// math/big int.divmod" for details.
		return big.NewInt(0).Div(l, r), nil
	case t.IDXBinaryShiftL:
		if r.Sign() < 0 || r.Cmp(ffff) > 0 {
			return nil, fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().Str(tm), n.Str(tm))
		}
		return big.NewInt(0).Lsh(l, uint(r.Uint64())), nil
	case t.IDXBinaryShiftR:
		if r.Sign() < 0 || r.Cmp(ffff) > 0 {
			return nil, fmt.Errorf("check: shift %q out of range in const expression %q",
				n.RHS().Expr().Str(tm), n.Str(tm))
		}
		return big.NewInt(0).Rsh(l, uint(r.Uint64())), nil
	case t.IDXBinaryAmp:
		return big.NewInt(0).And(l, r), nil
	case t.IDXBinaryAmpHat:
		return big.NewInt(0).AndNot(l, r), nil
	case t.IDXBinaryPipe:
		return big.NewInt(0).Or(l, r), nil
	case t.IDXBinaryHat:
		return big.NewInt(0).Xor(l, r), nil
	case t.IDXBinaryPercent:
		if r.Sign() == 0 {
			return nil, fmt.Errorf("check: division by zero in const expression %q", n.Str(tm))
		}
		return big.NewInt(0).Mod(l, r), nil
	case t.IDXBinaryTildeModPlus, t.IDXBinaryTildeModMinus,
		t.IDXBinaryTildeSatPlus, t.IDXBinaryTildeSatMinus:
		return nil, fmt.Errorf("check: cannot apply tilde-operators to ideal numbers")
	case t.IDXBinaryNotEq:
		return btoi(l.Cmp(r) != 0), nil
	case t.IDXBinaryLessThan:
		return btoi(l.Cmp(r) < 0), nil
	case t.IDXBinaryLessEq:
		return btoi(l.Cmp(r) <= 0), nil
	case t.IDXBinaryEqEq:
		return btoi(l.Cmp(r) == 0), nil
	case t.IDXBinaryGreaterEq:
		return btoi(l.Cmp(r) >= 0), nil
	case t.IDXBinaryGreaterThan:
		return btoi(l.Cmp(r) > 0), nil
	case t.IDXBinaryAnd:
		return btoi((l.Sign() != 0) && (r.Sign() != 0)), nil
	case t.IDXBinaryOr:
		return btoi((l.Sign() != 0) || (r.Sign() != 0)), nil
	}
	return nil, fmt.Errorf("check: unrecognized token (0x%02X) for evalConstValueBinaryOp", n.Operator())
}

func (q *checker) tcheckExprAssociativeOp(n *a.Expr, depth uint32) error {
	switch n.Operator() {
	case t.IDXAssociativePlus, t.IDXAssociativeStar,
		t.IDXAssociativeAmp, t.IDXAssociativePipe, t.IDXAssociativeHat:

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
					n.Operator().AmbiguousForm().Str(q.tm), o.Str(q.tm), oTyp.Str(q.tm))
			}
			if typ == nil {
				expr, typ = o, oTyp.Unrefined()
				continue
			}
			if !typ.EqIgnoringRefinements(oTyp) {
				return fmt.Errorf("check: associative %q: %q and %q, of types %q and %q, "+
					"do not have compatible types",
					n.Operator().AmbiguousForm().Str(q.tm),
					expr.Str(q.tm), o.Str(q.tm),
					expr.MType().Str(q.tm), o.MType().Str(q.tm))
			}
		}
		if typ == nil {
			typ = typeExprIdeal
		}
		n.SetMType(typ)
		return nil

	case t.IDXAssociativeAnd, t.IDXAssociativeOr:
		for _, o := range n.Args() {
			o := o.Expr()
			if err := q.tcheckExpr(o, depth); err != nil {
				return err
			}
			if !o.MType().IsBool() {
				return fmt.Errorf("check: associative %q: %q, of type %q, does not have a boolean type",
					n.Operator().AmbiguousForm().Str(q.tm), o.Str(q.tm), o.MType().Str(q.tm))
			}
		}
		n.SetMType(typeExprBool)
		return nil
	}

	return fmt.Errorf("check: unrecognized token (0x%X) for tcheckExprAssociativeOp", n.Operator())
}

func (q *checker) tcheckTypeExpr(typ *a.TypeExpr, depth uint32) error {
	if depth > a.MaxTypeExprDepth {
		return fmt.Errorf("check: type expression recursion depth too large")
	}
	depth++

swtch:
	switch typ.Decorator() {
	// TODO: also check t.IDFunc.
	case 0:
		qid := typ.QID()
		if qid[0] == t.IDBase && qid[1].IsNumType() {
			for _, b := range typ.Bounds() {
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
		if typ.Min() != nil || typ.Max() != nil {
			// TODO: reject. You can only refine numeric types.
		}
		if qid[0] == t.IDBase {
			if _, ok := builtInTypeMap[qid[1]]; ok {
				break swtch
			}
		}
		for _, s := range q.c.structs {
			if s.QID() == qid {
				break swtch
			}
		}
		return fmt.Errorf("check: %q is not a type", typ.Str(q.tm))

	case t.IDArray:
		aLen := typ.ArrayLength()
		if err := q.tcheckExpr(aLen, 0); err != nil {
			return err
		}
		if aLen.ConstValue() == nil {
			return fmt.Errorf("check: %q is not constant", aLen.Str(q.tm))
		}
		fallthrough

	// TODO: also check t.IDNptr? Where else should we look for nptr?
	case t.IDPtr, t.IDSlice:
		if err := q.tcheckTypeExpr(typ.Inner(), depth); err != nil {
			return err
		}

	default:
		return fmt.Errorf("check: %q is not a type", typ.Str(q.tm))
	}
	typ.Node().SetTypeChecked()
	return nil
}

var comparisonOps = [256]bool{
	t.IDXBinaryNotEq:       true,
	t.IDXBinaryLessThan:    true,
	t.IDXBinaryLessEq:      true,
	t.IDXBinaryEqEq:        true,
	t.IDXBinaryGreaterEq:   true,
	t.IDXBinaryGreaterThan: true,
}
