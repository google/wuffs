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
			switch n.id0.Key() {
			case 0:
				buf = append(buf, m.ByID(n.id1)...)

			case t.KeyOpenParen:
				buf = n.lhs.Expr().appendString(buf, m, true, depth)
				if n.flags&FlagsSuspendible != 0 {
					buf = append(buf, '?')
				}
				buf = append(buf, '(')
				for i, o := range n.list0 {
					if i != 0 {
						buf = append(buf, ", "...)
					}
					buf = append(buf, m.ByID(o.Arg().Name())...)
					buf = append(buf, ':')
					buf = o.Arg().Value().appendString(buf, m, false, depth)
				}
				buf = append(buf, ')')

			case t.KeyOpenBracket:
				buf = n.lhs.Expr().appendString(buf, m, true, depth)
				buf = append(buf, '[')
				buf = n.rhs.Expr().appendString(buf, m, false, depth)
				buf = append(buf, ']')

			case t.KeyColon:
				buf = n.lhs.Expr().appendString(buf, m, true, depth)
				buf = append(buf, '[')
				buf = n.mhs.Expr().appendString(buf, m, false, depth)
				buf = append(buf, ':')
				buf = n.rhs.Expr().appendString(buf, m, false, depth)
				buf = append(buf, ']')

			case t.KeyDot:
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
			if n.id0.Key() == t.KeyXBinaryAs {
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
	t.KeyXUnaryPlus:  "+",
	t.KeyXUnaryMinus: "-",
	t.KeyXUnaryNot:   "not ",

	t.KeyXBinaryPlus:        " + ",
	t.KeyXBinaryMinus:       " - ",
	t.KeyXBinaryStar:        " * ",
	t.KeyXBinarySlash:       " / ",
	t.KeyXBinaryShiftL:      " << ",
	t.KeyXBinaryShiftR:      " >> ",
	t.KeyXBinaryAmp:         " & ",
	t.KeyXBinaryAmpHat:      " &^ ",
	t.KeyXBinaryPipe:        " | ",
	t.KeyXBinaryHat:         " ^ ",
	t.KeyXBinaryNotEq:       " != ",
	t.KeyXBinaryLessThan:    " < ",
	t.KeyXBinaryLessEq:      " <= ",
	t.KeyXBinaryEqEq:        " == ",
	t.KeyXBinaryGreaterEq:   " >= ",
	t.KeyXBinaryGreaterThan: " > ",
	t.KeyXBinaryAnd:         " and ",
	t.KeyXBinaryOr:          " or ",
	t.KeyXBinaryAs:          " as ",

	t.KeyXAssociativePlus: " + ",
	t.KeyXAssociativeStar: " * ",
	t.KeyXAssociativeAmp:  " & ",
	t.KeyXAssociativePipe: " | ",
	t.KeyXAssociativeHat:  " ^ ",
	t.KeyXAssociativeAnd:  " and ",
	t.KeyXAssociativeOr:   " or ",
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

	switch n.PackageOrDecorator().Key() {
	case 0:
		buf = append(buf, m.ByID(n.Name())...)
	case t.KeyPtr:
		buf = append(buf, "ptr "...)
		return n.Inner().appendString(buf, m, depth)
	case t.KeyOpenBracket:
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
