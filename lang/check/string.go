// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

// ExprString returns a string form of the Expr n.
func ExprString(m *t.IDMap, n *a.Expr) string {
	if n == nil {
		return ""
	}
	if v := n.ConstValue(); v != nil {
		return v.String()
	}
	return "ExprString.TODO"
}

// TypeExprString returns a string form of the TypeExpr n.
func TypeExprString(m *t.IDMap, n *a.TypeExpr) string {
	if n != nil {
		s := ""
		switch n.PackageOrDecorator() {
		case 0:
			s = m.ByID(n.Name())
		case t.IDPtr:
			return "ptr " + TypeExprString(m, n.Inner())
		case t.IDOpenBracket:
			return "[" + ExprString(m, n.ArrayLength()) + "] " + TypeExprString(m, n.Inner())
		default:
			s = m.ByID(n.PackageOrDecorator()) + "." + m.ByID(n.Name())
		}
		if n.InclMin() == nil && n.ExclMax() == nil {
			return s
		}
		return s + "[" + ExprString(m, n.InclMin()) + ":" + ExprString(m, n.ExclMax()) + "]"
	}
	return "!invalid_type!"
}
