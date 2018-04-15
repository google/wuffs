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

// Str returns a string form of n.
func (n *Expr) Str(tm *t.Map) string {
	if n == nil {
		return ""
	}
	if n.id0 == 0 && n.id1 == 0 {
		return tm.ByID(n.id2)
	}
	return string(n.appendStr(nil, tm, false, 0))
}

func (n *Expr) appendStr(buf []byte, tm *t.Map, parenthesize bool, depth uint32) []byte {
	if depth > MaxExprDepth {
		return append(buf, "!expr_recursion_depth_too_large!"...)
	}
	depth++

	if n == nil {
		return buf
	}

	if n.id0.IsXOp() {
		switch {
		case n.id0.IsXUnaryOp():
			buf = append(buf, opStrings[0xFF&n.id0.Key()]...)
			buf = n.rhs.Expr().appendStr(buf, tm, true, depth)

		case n.id0.IsXBinaryOp():
			if parenthesize {
				buf = append(buf, '(')
			}
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, opStrings[0xFF&n.id0.Key()]...)
			if n.id0.Key() == t.KeyXBinaryAs {
				buf = append(buf, n.rhs.TypeExpr().Str(tm)...)
			} else {
				buf = n.rhs.Expr().appendStr(buf, tm, true, depth)
			}
			if parenthesize {
				buf = append(buf, ')')
			}

		case n.id0.IsXAssociativeOp():
			if parenthesize {
				buf = append(buf, '(')
			}
			op := opStrings[0xFF&n.id0.Key()]
			for i, o := range n.list0 {
				if i != 0 {
					buf = append(buf, op...)
				}
				buf = o.Expr().appendStr(buf, tm, true, depth)
			}
			if parenthesize {
				buf = append(buf, ')')
			}
		}

	} else {
		switch n.id0.Key() {
		case t.KeyError, t.KeyStatus, t.KeySuspension:
			switch n.id0.Key() {
			case t.KeyError:
				buf = append(buf, "error "...)
			case t.KeyStatus:
				buf = append(buf, "status "...)
			case t.KeySuspension:
				buf = append(buf, "suspension "...)
			}
			if n.id1 != 0 {
				buf = append(buf, tm.ByID(n.id1)...)
				buf = append(buf, '.')
			}
			fallthrough

		case 0:
			buf = append(buf, tm.ByID(n.id2)...)

		case t.KeyTry:
			buf = append(buf, "try "...)
			fallthrough

		case t.KeyOpenParen:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			if n.flags&FlagsSuspendible != 0 {
				buf = append(buf, '?')
			} else if n.flags&FlagsImpure != 0 {
				buf = append(buf, '!')
			}
			buf = append(buf, '(')
			for i, o := range n.list0 {
				if i != 0 {
					buf = append(buf, ", "...)
				}
				buf = append(buf, tm.ByID(o.Arg().Name())...)
				buf = append(buf, ':')
				buf = o.Arg().Value().appendStr(buf, tm, false, depth)
			}
			buf = append(buf, ')')

		case t.KeyOpenBracket:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, '[')
			buf = n.rhs.Expr().appendStr(buf, tm, false, depth)
			buf = append(buf, ']')

		case t.KeyColon:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, '[')
			buf = n.mhs.Expr().appendStr(buf, tm, false, depth)
			buf = append(buf, ':')
			buf = n.rhs.Expr().appendStr(buf, tm, false, depth)
			buf = append(buf, ']')

		case t.KeyDot:
			buf = n.lhs.Expr().appendStr(buf, tm, true, depth)
			buf = append(buf, '.')
			buf = append(buf, tm.ByID(n.id2)...)

		case t.KeyDollar:
			buf = append(buf, "$("...)
			for i, o := range n.list0 {
				if i != 0 {
					buf = append(buf, ", "...)
				}
				buf = o.Expr().appendStr(buf, tm, false, depth)
			}
			buf = append(buf, ')')
		}
	}

	return buf
}

var opStrings = [256]string{
	t.KeyXUnaryPlus:  "+",
	t.KeyXUnaryMinus: "-",
	t.KeyXUnaryNot:   "not ",
	t.KeyXUnaryRef:   "ref ",
	t.KeyXUnaryDeref: "deref ",

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
	t.KeyXBinaryTildePlus:   " ~+ ",

	t.KeyXAssociativePlus: " + ",
	t.KeyXAssociativeStar: " * ",
	t.KeyXAssociativeAmp:  " & ",
	t.KeyXAssociativePipe: " | ",
	t.KeyXAssociativeHat:  " ^ ",
	t.KeyXAssociativeAnd:  " and ",
	t.KeyXAssociativeOr:   " or ",
}

// Str returns a string form of n.
func (n *TypeExpr) Str(tm *t.Map) string {
	if n == nil {
		return ""
	}
	if n.Decorator() == 0 && n.Min() == nil && n.Max() == nil {
		return n.QID().Str(tm)
	}
	return string(n.appendStr(nil, tm, 0))
}

func (n *TypeExpr) appendStr(buf []byte, tm *t.Map, depth uint32) []byte {
	if depth > MaxTypeExprDepth {
		return append(buf, "!type_expr_recursion_depth_too_large!"...)
	}
	depth++
	if n == nil {
		return append(buf, "!invalid_type!"...)
	}

	switch n.Decorator().Key() {
	case 0:
		buf = append(buf, n.QID().Str(tm)...)
	case t.KeyPtr:
		buf = append(buf, "ptr "...)
		return n.Inner().appendStr(buf, tm, depth)
	case t.KeyArray:
		buf = append(buf, "array["...)
		buf = n.ArrayLength().appendStr(buf, tm, false, 0)
		buf = append(buf, "] "...)
		return n.Inner().appendStr(buf, tm, depth)
	case t.KeySlice:
		buf = append(buf, "slice "...)
		return n.Inner().appendStr(buf, tm, depth)
	case t.KeyOpenParen:
		buf = append(buf, "func "...)
		buf = n.Receiver().appendStr(buf, tm, depth)
		buf = append(buf, '.')
		return append(buf, n.FuncName().Str(tm)...)
	default:
		return append(buf, "!invalid_type!"...)
	}
	if n.Min() != nil || n.Max() != nil {
		buf = append(buf, '[')
		buf = n.Min().appendStr(buf, tm, false, 0)
		buf = append(buf, ".."...)
		buf = n.Max().appendStr(buf, tm, false, 0)
		buf = append(buf, ']')
	}
	return buf
}
