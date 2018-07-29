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

package ast

import (
	t "github.com/google/wuffs/lang/token"
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

	if n.flags != o.flags || n.id0 != o.id0 || n.id1 != o.id1 || n.id2 != o.id2 {
		return false
	}
	if !n.lhs.AsExpr().Eq(o.lhs.AsExpr()) {
		return false
	}
	if !n.mhs.AsExpr().Eq(o.mhs.AsExpr()) {
		return false
	}

	if n.id0 == t.IDXBinaryAs {
		if !n.rhs.AsTypeExpr().Eq(o.rhs.AsTypeExpr()) {
			return false
		}
	} else if !n.rhs.AsExpr().Eq(o.rhs.AsExpr()) {
		return false
	}

	if len(n.list0) != len(o.list0) {
		return false
	}
	for i, x := range n.list0 {
		if !x.AsExpr().Eq(o.list0[i].AsExpr()) {
			return false
		}
	}
	return true
}

func (n *Expr) Mentions(o *Expr) bool {
	if n == nil {
		return false
	}
	if n.Eq(o) ||
		n.lhs.AsExpr().Mentions(o) ||
		n.mhs.AsExpr().Mentions(o) ||
		(n.id0 != t.IDXBinaryAs && n.rhs.AsExpr().Mentions(o)) {
		return true
	}
	for _, x := range n.list0 {
		if x.AsExpr().Mentions(o) {
			return true
		}
	}
	return false
}

// Eq returns whether n and o are equal.
func (n *TypeExpr) Eq(o *TypeExpr) bool {
	return n.eq(o, false)
}

// EqIgnoringRefinements returns whether n and o are equal, ignoring the
// "[i:j]" in "base.u32[i:j]".
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
		if n.id0 != o.id0 || n.id1 != o.id1 || n.id2 != o.id2 {
			return false
		}
		if n.IsArrayType() || !ignoreRefinements {
			if !n.lhs.AsExpr().Eq(o.lhs.AsExpr()) || !n.mhs.AsExpr().Eq(o.mhs.AsExpr()) {
				return false
			}
		}
		if n.rhs == nil && o.rhs == nil {
			return true
		}
		n = n.rhs.AsTypeExpr()
		o = o.rhs.AsTypeExpr()
	}
}
