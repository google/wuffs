// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package ast

import (
	t "github.com/google/puffs/lang/token"
)

// Eq returns whether n and o are equal.
//
// It may return false negatives. In general, it will not report that "x + y"
// equals "y + x". However, if both are constant expressions (i.e. each Expr
// node, including the sum nodes, has a ConstValue), both sums will have the
// same value and will compare equal.
func (n *Expr) Eq(o *Expr) bool {
	if n == o {
		return true
	}
	if n == nil || o == nil {
		return false
	}
	if n.constValue != nil && o.constValue != nil {
		return n.constValue.Cmp(o.constValue) == 0
	}

	if n.flags != o.flags || n.id0 != o.id0 || n.id1 != o.id1 {
		return false
	}
	if !n.lhs.Expr().Eq(o.lhs.Expr()) {
		return false
	}
	if !n.mhs.Expr().Eq(o.mhs.Expr()) {
		return false
	}

	if n.id0.Key() == t.KeyXBinaryAs {
		if !n.rhs.TypeExpr().Eq(o.rhs.TypeExpr()) {
			return false
		}
	} else if !n.rhs.Expr().Eq(o.rhs.Expr()) {
		return false
	}

	if len(n.list0) != len(o.list0) {
		return false
	}
	for i, x := range n.list0 {
		if !x.Expr().Eq(o.list0[i].Expr()) {
			return false
		}
	}
	return true
}

// Eq returns whether n and o are equal.
func (n *TypeExpr) Eq(o *TypeExpr) bool {
	return n.eq(o, false)
}

// EqIgnoringRefinements returns whether n and o are equal, ignoring the
// "[i:j]" in "u32[i:j]".
func (n *TypeExpr) EqIgnoringRefinements(o *TypeExpr) bool {
	return n.eq(o, true)
}

func (n *TypeExpr) eq(o *TypeExpr, ignoreRefinements bool) bool {
	for {
		if n == o {
			return true
		}
		if n == nil || o == nil {
			return false
		}
		if n.id0 != o.id0 || n.id1 != o.id1 {
			return false
		}
		if n.id0.Key() == t.KeyOpenBracket || !ignoreRefinements {
			if !n.lhs.Expr().Eq(o.lhs.Expr()) || !n.mhs.Expr().Eq(o.mhs.Expr()) {
				return false
			}
		}
		if n.rhs == nil && o.rhs == nil {
			return true
		}
		n = n.rhs.TypeExpr()
		o = o.rhs.TypeExpr()
	}
}
