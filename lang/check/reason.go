// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

var reasons = [...]struct {
	s string
	r reason
}{
	{`"a < (b + c): a < c; 0 <= b"`, func(q *checker, n *a.Assert) error {
		op, xa, xbc := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessThan {
			return errFailed
		}
		op, xb, xc := parseBinaryOp(xbc)
		if op.Key() != t.KeyXBinaryPlus {
			return errFailed
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessThan, xa, xc); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessThan, xa, xc, err)
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, zeroExpr, xb); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, zeroExpr, xb, err)
		}
		return nil
	}},
	{`"(a + b) <= c: a <= (c - b)"`, func(q *checker, n *a.Assert) error {
		op, xab, xc := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessEq {
			return errFailed
		}
		op, xa, xb := parseBinaryOp(xab)
		if op.Key() != t.KeyXBinaryPlus {
			return errFailed
		}
		sub := a.NewExpr(a.FlagsTypeChecked, t.IDXBinaryMinus, 0, xc.Node(), nil, xb.Node(), nil)
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, xa, sub); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, xa, sub, err)
		}
		return nil
	}},
	{`"a < b: a < c; c <= b"`, func(q *checker, n *a.Assert) error {
		xc := argValue(q.tm, n.Args(), "c")
		if xc == nil {
			return errFailed
		}
		op, xa, xb := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessThan {
			return errFailed
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessThan, xa, xc); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessThan, xa, xc, err)
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, xc, xb); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, xc, xb, err)
		}
		return nil
	}},
	{`"a < (b + c): a < (b0 + c0); b0 <= b; c0 <= c"`, func(q *checker, n *a.Assert) error {
		xb0 := argValue(q.tm, n.Args(), "b0")
		if xb0 == nil {
			return errFailed
		}
		xc0 := argValue(q.tm, n.Args(), "c0")
		if xc0 == nil {
			return errFailed
		}
		op, xa, xbc := parseBinaryOp(n.Condition())
		if op.Key() != t.KeyXBinaryLessThan {
			return errFailed
		}
		op, xb, xc := parseBinaryOp(xbc)
		if op.Key() != t.KeyXBinaryPlus {
			return errFailed
		}
		plus := a.NewExpr(a.FlagsTypeChecked, t.IDXBinaryPlus, 0, xb0.Node(), nil, xc0.Node(), nil)
		if err := q.proveBinaryOp(t.KeyXBinaryLessThan, xa, plus); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessThan, xa, plus, err)
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, xb0, xb); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, xb0, xb, err)
		}
		if err := q.proveBinaryOp(t.KeyXBinaryLessEq, xc0, xc); err != nil {
			return binOpReasonError(q.tm, t.IDXBinaryLessEq, xc0, xc, err)
		}
		return nil
	}},
}
