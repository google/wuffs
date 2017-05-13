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
	if n.id0 == 0 {
		return m.ByID(n.id1)
	}
	return string(n.appendString(nil, m, false))
}

func (n *Expr) appendString(buf []byte, m *t.IDMap, parenthesize bool) []byte {
	if n != nil {
		switch n.id0.Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
		case 0:
			switch n.id0 {
			case 0:
				buf = append(buf, m.ByID(n.id1)...)

			case t.IDOpenParen:
				buf = n.lhs.Expr().appendString(buf, m, true)
				if n.flags&FlagsSuspendible != 0 {
					buf = append(buf, '?')
				}
				buf = append(buf, '(')
				for i, o := range n.list0 {
					if i != 0 {
						buf = append(buf, ", "...)
					}
					buf = o.Expr().appendString(buf, m, false)
				}
				buf = append(buf, ')')

			case t.IDOpenBracket:
				buf = n.lhs.Expr().appendString(buf, m, true)
				buf = append(buf, '[')
				buf = n.rhs.Expr().appendString(buf, m, false)
				buf = append(buf, ']')

			case t.IDColon:
				buf = n.lhs.Expr().appendString(buf, m, true)
				buf = append(buf, '[')
				buf = n.mhs.Expr().appendString(buf, m, false)
				buf = append(buf, ':')
				buf = n.rhs.Expr().appendString(buf, m, false)
				buf = append(buf, ']')

			case t.IDDot:
				buf = n.lhs.Expr().appendString(buf, m, true)
				buf = append(buf, '.')
				buf = append(buf, m.ByID(n.id1)...)
			}

		case t.FlagsUnaryOp:
			buf = append(buf, opStrings[0xFF&n.id0.Key()]...)
			buf = n.rhs.Expr().appendString(buf, m, true)

		case t.FlagsBinaryOp:
			if parenthesize {
				buf = append(buf, '(')
			}
			buf = n.lhs.Expr().appendString(buf, m, true)
			buf = append(buf, opStrings[0xFF&n.id0.Key()]...)
			if n.id0 == t.IDXBinaryAs {
				buf = append(buf, n.rhs.TypeExpr().String(m)...)
			} else {
				buf = n.rhs.Expr().appendString(buf, m, true)
			}
			if parenthesize {
				buf = append(buf, ')')
			}

		case t.FlagsAssociativeOp:
			if parenthesize {
				buf = append(buf, '(')
			}
			op := opStrings[0xFF&n.id0.Key()]
			for i, o := range n.list0 {
				if i != 0 {
					buf = append(buf, op...)
				}
				buf = o.Expr().appendString(buf, m, true)
			}
			if parenthesize {
				buf = append(buf, ')')
			}
		}
	}

	return buf
}

var opStrings = [256]string{
	t.IDXUnaryPlus >> t.KeyShift:  "+",
	t.IDXUnaryMinus >> t.KeyShift: "-",
	t.IDXUnaryNot >> t.KeyShift:   "not ",

	t.IDXBinaryPlus >> t.KeyShift:        " + ",
	t.IDXBinaryMinus >> t.KeyShift:       " - ",
	t.IDXBinaryStar >> t.KeyShift:        " * ",
	t.IDXBinarySlash >> t.KeyShift:       " / ",
	t.IDXBinaryShiftL >> t.KeyShift:      " << ",
	t.IDXBinaryShiftR >> t.KeyShift:      " >> ",
	t.IDXBinaryAmp >> t.KeyShift:         " & ",
	t.IDXBinaryAmpHat >> t.KeyShift:      " &^ ",
	t.IDXBinaryPipe >> t.KeyShift:        " | ",
	t.IDXBinaryHat >> t.KeyShift:         " ^ ",
	t.IDXBinaryNotEq >> t.KeyShift:       " != ",
	t.IDXBinaryLessThan >> t.KeyShift:    " < ",
	t.IDXBinaryLessEq >> t.KeyShift:      " <= ",
	t.IDXBinaryEqEq >> t.KeyShift:        " == ",
	t.IDXBinaryGreaterEq >> t.KeyShift:   " >= ",
	t.IDXBinaryGreaterThan >> t.KeyShift: " > ",
	t.IDXBinaryAnd >> t.KeyShift:         " and ",
	t.IDXBinaryOr >> t.KeyShift:          " or ",
	t.IDXBinaryAs >> t.KeyShift:          " as ",

	t.IDXAssociativePlus >> t.KeyShift: " + ",
	t.IDXAssociativeStar >> t.KeyShift: " * ",
	t.IDXAssociativeAmp >> t.KeyShift:  " & ",
	t.IDXAssociativePipe >> t.KeyShift: " | ",
	t.IDXAssociativeHat >> t.KeyShift:  " ^ ",
	t.IDXAssociativeAnd >> t.KeyShift:  " and ",
	t.IDXAssociativeOr >> t.KeyShift:   " or ",
}

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
		if n.id0 == t.IDOpenBracket || !ignoreRefinements {
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
