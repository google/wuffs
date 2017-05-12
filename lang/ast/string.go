// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package ast

import (
	t "github.com/google/puffs/lang/token"
)

// String returns a string form of the Expr n.
func (n *Expr) String(m *t.IDMap) string {
	if n == nil {
		return ""
	}
	if n.ID0() == 0 {
		return m.ByID(n.ID1())
	}
	return "Expr.String.TODO"
}

// String returns a string form of the TypeExpr n.
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
