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

package check

// TODO: should bounds checking even be responsible for selecting between
// optimized implementations of e.g. read_u8? Instead, the Wuffs code could
// explicitly call either read_u8 or read_u8_fast, with the latter having
// stronger preconditions.
//
// Doing so might need some syntactical distinction (not just a question mark)
// between "foo?" and "bar?" if one of those methods can still return an error
// code but never actually suspend.

import (
	"fmt"
	"math/big"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

// splitReceiverMethodArgs returns the "receiver", "method" and "args" in the
// expression "receiver.method(args)".
func splitReceiverMethodArgs(n *a.Expr) (receiver *a.Expr, method t.ID, args []*a.Node) {
	if n.Operator() != t.IDOpenParen {
		return nil, 0, nil
	}
	args = n.Args()
	n = n.LHS().Expr()
	if n.Operator() != t.IDDot {
		return nil, 0, nil
	}
	return n.LHS().Expr(), n.Ident(), args
}

// TODO: should optimizeNonSuspendible be optimizeExpr and check both
// suspendible and non-suspendible expressions?
//
// See also "be principled about checking for provenNotToSuspend".

func (q *checker) optimizeNonSuspendible(n *a.Expr) error {
	// TODO: check n.CallSuspendible()?
	//
	// Should we have an explicit "foo = call bar?()" syntax?

	nReceiver, nMethod, _ := splitReceiverMethodArgs(n)
	if nReceiver == nil {
		return nil
	}

	// TODO: check that nReceiver's type is actually a io_reader or io_writer.

	switch nMethod {
	case t.IDCopyFromHistory32:
		return q.optimizeCopyFromHistory32(n)
	}

	return nil
}

// optimizeCopyFromHistory32 checks if the code generator can call the Bounds
// Check Optimized version of CopyFromHistory32. As per cgen's base-private.h,
// the conditions are:
//  - distance >  0
//  - distance <= (*ptr_ptr - start)
//  - length   <= (end      - *ptr_ptr)
func (q *checker) optimizeCopyFromHistory32(n *a.Expr) error {
	// The arguments are (distance, length).
	nReceiver, _, args := splitReceiverMethodArgs(n)
	if len(args) != 2 {
		return nil
	}
	d := args[0].Arg().Value()
	l := args[1].Arg().Value()

	// Check "distance > 0".
check0:
	for {
		for _, x := range q.facts {
			if x.Operator() != t.IDXBinaryGreaterThan {
				continue
			}
			if lhs := x.LHS().Expr(); !lhs.Eq(d) {
				continue
			}
			if rcv := x.RHS().Expr().ConstValue(); rcv == nil || rcv.Sign() != 0 {
				continue
			}
			break check0
		}
		return nil
	}

	// Check "distance <= (*ptr_ptr - start)".
check1:
	for {
		for _, x := range q.facts {
			if x.Operator() != t.IDXBinaryLessEq {
				continue
			}

			// Check that the LHS is "d as base.u64".
			lhs := x.LHS().Expr()
			if lhs.Operator() != t.IDXBinaryAs {
				continue
			}
			llhs, lrhs := lhs.LHS().Expr(), lhs.RHS().TypeExpr()
			if !llhs.Eq(d) || !lrhs.Eq(typeExprU64) {
				continue
			}

			// Check that the RHS is "nReceiver.since_mark().length()".
			y, method, yArgs := splitReceiverMethodArgs(x.RHS().Expr())
			if method != t.IDLength || len(yArgs) != 0 {
				continue
			}
			z, method, zArgs := splitReceiverMethodArgs(y)
			if method != t.IDSinceMark || len(zArgs) != 0 {
				continue
			}
			if !z.Eq(nReceiver) {
				continue
			}

			break check1
		}
		return nil
	}

	// Check "length <= (end - *ptr_ptr)".
check2:
	for {
		for _, x := range q.facts {
			if x.Operator() != t.IDXBinaryLessEq {
				continue
			}

			// Check that the LHS is "l as base.u64".
			lhs := x.LHS().Expr()
			if lhs.Operator() != t.IDXBinaryAs {
				continue
			}
			llhs, lrhs := lhs.LHS().Expr(), lhs.RHS().TypeExpr()
			if !llhs.Eq(l) || !lrhs.Eq(typeExprU64) {
				continue
			}

			// Check that the RHS is "nReceiver.available()".
			y, method, yArgs := splitReceiverMethodArgs(x.RHS().Expr())
			if method != t.IDAvailable || len(yArgs) != 0 {
				continue
			}
			if !y.Eq(nReceiver) {
				continue
			}

			break check2
		}
		return nil
	}

	n.SetBoundsCheckOptimized()
	return nil
}

func (q *checker) optimizeSuspendible(n *a.Expr, depth uint32) error {
	if depth > a.MaxExprDepth {
		return fmt.Errorf("check: expression recursion depth too large")
	}
	depth++

	if !n.CallSuspendible() {
		for _, o := range n.Node().Raw().SubNodes() {
			if o != nil && o.Kind() == a.KExpr {
				if err := q.optimizeSuspendible(o.Expr(), depth); err != nil {
					return err
				}
			}
		}
		for _, o := range n.Args() {
			if o != nil && o.Kind() == a.KExpr {
				if err := q.optimizeSuspendible(o.Expr(), depth); err != nil {
					return err
				}
			}
		}
		return nil
	}

	nReceiver, nMethod, _ := splitReceiverMethodArgs(n)
	if nReceiver == nil {
		return nil
	}

	// TODO: check that nReceiver's type is actually a io_reader or io_writer.

	if nMethod == t.IDUnreadU8 {
		// unread_u8 can never suspend, only succeed or fail.
		n.SetProvenNotToSuspend()
		return nil
	}

	if advance := ioMethodAdvance(nMethod); advance != nil {
		if ok, err := q.optimizeIOMethodAdvance(nReceiver, advance, true); err != nil {
			return err
		} else if ok {
			n.SetProvenNotToSuspend()
		}
	}

	return nil
}

func (q *checker) optimizeIOMethodAdvance(receiver *a.Expr, advance *big.Int, update bool) (retOK bool, retErr error) {
	retErr = q.facts.update(func(x *a.Expr) (*a.Expr, error) {
		// TODO: update (discard?) any facts that merely mention
		// receiver.available(), even if they aren't an exact match.

		op := x.Operator()
		if op != t.IDXBinaryGreaterEq && op != t.IDXBinaryGreaterThan {
			return x, nil
		}

		rcv := x.RHS().Expr().ConstValue()
		if rcv == nil {
			return x, nil
		}

		// Check that lhs is "receiver.available()".
		lhs := x.LHS().Expr()
		if lhs.Operator() != t.IDOpenParen || len(lhs.Args()) != 0 {
			return x, nil
		}
		lhs = lhs.LHS().Expr()
		if lhs.Operator() != t.IDDot || lhs.Ident() != t.IDAvailable {
			return x, nil
		}
		lhs = lhs.LHS().Expr()
		if !lhs.Eq(receiver) {
			return x, nil
		}

		// Check if the bytes available is >= the bytes needed. If so, update
		// rcv to be the bytes remaining. If not, discard the fact x.
		if op == t.IDXBinaryGreaterThan {
			op = t.IDXBinaryGreaterEq
			rcv = big.NewInt(0).Add(rcv, one)
		}
		if rcv.Cmp(advance) < 0 {
			if !update {
				return x, nil
			}
			return nil, nil
		}

		retOK = true

		if !update {
			return x, nil
		}

		// Create a new a.Expr to hold the adjusted RHS constant value, newRCV.
		newRCV := big.NewInt(0).Sub(rcv, advance)
		id, err := q.tm.Insert(newRCV.String())
		if err != nil {
			return nil, err
		}
		o := a.NewExpr(0, 0, 0, id, nil, nil, nil, nil)
		o.SetConstValue(newRCV)
		o.SetMType(typeExprIdeal)

		return a.NewExpr(x.Node().Raw().Flags(), t.IDXBinaryGreaterEq, 0, 0, x.LHS(), nil, o.Node(), nil), nil
	})
	return retOK, retErr
}

func ioMethodAdvance(x t.ID) *big.Int {
	switch x {
	case t.IDReadU8, t.IDWriteU8:
		return one
	case t.IDReadU16BE, t.IDReadU16LE, t.IDWriteU16BE, t.IDWriteU16LE:
		return two
	case t.IDReadU24BE, t.IDReadU24LE, t.IDWriteU24BE, t.IDWriteU24LE:
		return three
	case t.IDReadU32BE, t.IDReadU32LE, t.IDWriteU32BE, t.IDWriteU32LE:
		return four
	case t.IDReadU40BE, t.IDReadU40LE, t.IDWriteU40BE, t.IDWriteU40LE:
		return five
	case t.IDReadU48BE, t.IDReadU48LE, t.IDWriteU48BE, t.IDWriteU48LE:
		return six
	case t.IDReadU56BE, t.IDReadU56LE, t.IDWriteU56BE, t.IDWriteU56LE:
		return seven
	case t.IDReadU64BE, t.IDReadU64LE, t.IDWriteU64BE, t.IDWriteU64LE:
		return eight
	}
	return nil
}
