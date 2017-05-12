// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package ast

import (
	t "github.com/google/puffs/lang/token"
)

// String returns a string form of n.
func (n *Expr) String(m *t.IDMap) string {
	if n == nil {
		return ""
	}
	if n.ID0() == 0 {
		return m.ByID(n.ID1())
	}
	return "Expr.String.TODO"
}

// Eq returns whether n and o are equal.
//
// It may return false negatives. In general, it will not report that "x + y"
// equals "y + x". However, if both are constant expressions (i.e. each Expr
// node, including the sum nodes, has a ConstValue), both sums will have the
// same value and will compare equal.
func (n *Expr) Eq(o *Expr) bool {
	if n == nil || o == nil {
		return n == nil && o == nil
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

	if n.id0 == t.IDXBinaryAs {
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

// String returns a string form of n.
func (n *TypeExpr) String(m *t.IDMap) string {
	if n != nil {
		s := ""
		switch n.PackageOrDecorator() {
		case 0:
			s = m.ByID(n.Name())
		case t.IDPtr:
			return "ptr " + n.Inner().String(m)
		case t.IDOpenBracket:
			return "[" + n.ArrayLength().String(m) + "] " + n.Inner().String(m)
		default:
			s = m.ByID(n.PackageOrDecorator()) + "." + m.ByID(n.Name())
		}
		if n.InclMin() == nil && n.ExclMax() == nil {
			return s
		}
		return s + "[" + n.InclMin().String(m) + ":" + n.ExclMax().String(m) + "]"
	}
	return "!invalid_type!"
}

// Eq returns whether n and o are equal.
func (n *TypeExpr) Eq(o *TypeExpr) bool {
	for {
		if n == nil || o == nil {
			return n == nil && o == nil
		}
		if n.id0 != o.id0 || n.id1 != o.id1 ||
			!n.lhs.Expr().Eq(o.lhs.Expr()) ||
			!n.mhs.Expr().Eq(o.mhs.Expr()) {
			return false
		}
		if n.rhs == nil && o.rhs == nil {
			return true
		}
		n = n.rhs.TypeExpr()
		o = o.rhs.TypeExpr()
	}
}
