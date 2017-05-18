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
	return string(n.appendString(nil, m, false, 0))
}

func (n *Expr) appendString(buf []byte, m *t.IDMap, parenthesize bool, depth uint32) []byte {
	if depth > MaxExprDepth {
		return append(buf, "!expr_recursion_depth_too_large!"...)
	}
	depth++

	if n != nil {
		switch n.id0.Flags() & (t.FlagsUnaryOp | t.FlagsBinaryOp | t.FlagsAssociativeOp) {
		case 0:
			switch n.id0 {
			case 0:
				buf = append(buf, m.ByID(n.id1)...)

			case t.IDOpenParen:
				buf = n.lhs.Expr().appendString(buf, m, true, depth)
				if n.flags&FlagsSuspendible != 0 {
					buf = append(buf, '?')
				}
				buf = append(buf, '(')
				for i, o := range n.list0 {
					if i != 0 {
						buf = append(buf, ", "...)
					}
					buf = o.Expr().appendString(buf, m, false, depth)
				}
				buf = append(buf, ')')

			case t.IDOpenBracket:
				buf = n.lhs.Expr().appendString(buf, m, true, depth)
				buf = append(buf, '[')
				buf = n.rhs.Expr().appendString(buf, m, false, depth)
				buf = append(buf, ']')

			case t.IDColon:
				buf = n.lhs.Expr().appendString(buf, m, true, depth)
				buf = append(buf, '[')
				buf = n.mhs.Expr().appendString(buf, m, false, depth)
				buf = append(buf, ':')
				buf = n.rhs.Expr().appendString(buf, m, false, depth)
				buf = append(buf, ']')

			case t.IDDot:
				buf = n.lhs.Expr().appendString(buf, m, true, depth)
				buf = append(buf, '.')
				buf = append(buf, m.ByID(n.id1)...)
			}

		case t.FlagsUnaryOp:
			buf = append(buf, opStrings[0xFF&n.id0.Key()]...)
			buf = n.rhs.Expr().appendString(buf, m, true, depth)

		case t.FlagsBinaryOp:
			if parenthesize {
				buf = append(buf, '(')
			}
			buf = n.lhs.Expr().appendString(buf, m, true, depth)
			buf = append(buf, opStrings[0xFF&n.id0.Key()]...)
			if n.id0 == t.IDXBinaryAs {
				buf = append(buf, n.rhs.TypeExpr().String(m)...)
			} else {
				buf = n.rhs.Expr().appendString(buf, m, true, depth)
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
				buf = o.Expr().appendString(buf, m, true, depth)
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

// String returns a string form of n.
func (n *TypeExpr) String(m *t.IDMap) string {
	if n == nil {
		return ""
	}
	if n.PackageOrDecorator() == 0 && n.InclMin() == nil && n.ExclMax() == nil {
		return m.ByID(n.Name())
	}
	return string(n.appendString(nil, m, 0))
}

func (n *TypeExpr) appendString(buf []byte, m *t.IDMap, depth uint32) []byte {
	if depth > MaxTypeExprDepth {
		return append(buf, "!type_expr_recursion_depth_too_large!"...)
	}
	depth++
	if n == nil {
		return append(buf, "!invalid_type!"...)
	}

	switch n.PackageOrDecorator() {
	case 0:
		buf = append(buf, m.ByID(n.Name())...)
	case t.IDPtr:
		buf = append(buf, "ptr "...)
		return n.Inner().appendString(buf, m, depth)
	case t.IDOpenBracket:
		buf = append(buf, '[')
		buf = n.ArrayLength().appendString(buf, m, false, 0)
		buf = append(buf, "] "...)
		return n.Inner().appendString(buf, m, depth)
	default:
		buf = append(buf, m.ByID(n.PackageOrDecorator())...)
		buf = append(buf, '.')
		buf = append(buf, m.ByID(n.Name())...)
	}
	if n.InclMin() != nil || n.ExclMax() != nil {
		buf = append(buf, '[')
		buf = n.InclMin().appendString(buf, m, false, 0)
		buf = append(buf, ':')
		buf = n.ExclMax().appendString(buf, m, false, 0)
		buf = append(buf, ']')
	}
	return buf
}
