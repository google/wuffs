// Copyright 2017 The Puffs Authors.
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
// optimized implementations of e.g. read_u8? Instead, the Puffs code could
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
func splitReceiverMethodArgs(n *a.Expr) (receiver *a.Expr, method t.Key, args []*a.Node) {
	if n.ID0().Key() != t.KeyOpenParen {
		return nil, 0, nil
	}
	args = n.Args()
	n = n.LHS().Expr()
	if n.ID0().Key() != t.KeyDot {
		return nil, 0, nil
	}
	return n.LHS().Expr(), n.ID1().Key(), args
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

	// TODO: check that nReceiver's type is actually a reader1 or writer1.

	switch nMethod {
	case t.KeyCopyFromHistory32:
		return q.optimizeCopyFromHistory32(n)
	case t.KeySinceMark:
		for _, x := range q.facts {
			xReceiver, xMethod, _ := splitReceiverMethodArgs(x)
			if xMethod == t.KeyIsMarked && xReceiver.Eq(nReceiver) {
				n.SetBoundsCheckOptimized()
				return nil
			}
		}
	}

	return nil
}

// optimizeCopyFromHistory32 checks if the code generator can call the Bounds
// Check Optimized version of CopyFromHistory32. As per cgen/base-impl.h, the
// conditions are:
//  - start    != NULL
//  - distance != 0
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

	// Check "start != NULL".
check0:
	for {
		for _, x := range q.facts {
			xReceiver, xMethod, _ := splitReceiverMethodArgs(x)
			if xMethod == t.KeyIsMarked && xReceiver.Eq(nReceiver) {
				break check0
			}
		}
		return nil
	}

	// Check "distance != 0".
	//
	// TODO: check that typeof(distance) is an unsigned integer type, so that
	// an equivalent test is "distance > 0".
check1:
	for {
		for _, x := range q.facts {
			if x.ID0().Key() != t.KeyXBinaryGreaterThan {
				continue
			}
			if lhs := x.LHS().Expr(); !lhs.Eq(d) {
				continue
			}
			if rcv := x.RHS().Expr().ConstValue(); rcv == nil || rcv.Cmp(zero) != 0 {
				continue
			}
			break check1
		}
		return nil
	}

	// Check "distance <= (*ptr_ptr - start)".
check2:
	for {
		for _, x := range q.facts {
			if x.ID0().Key() != t.KeyXBinaryLessEq {
				continue
			}

			// Check that the LHS is "d as u64".
			lhs := x.LHS().Expr()
			if lhs.ID0().Key() != t.KeyXBinaryAs {
				continue
			}
			llhs, lrhs := lhs.LHS().Expr(), lhs.RHS().TypeExpr()
			if !llhs.Eq(d) || !lrhs.Eq(typeExprU64) {
				continue
			}

			// Check that the RHS is "nReceiver.since_mark().length()".
			y, method, yArgs := splitReceiverMethodArgs(x.RHS().Expr())
			if method != t.KeyLength || len(yArgs) != 0 {
				continue
			}
			z, method, zArgs := splitReceiverMethodArgs(y)
			if method != t.KeySinceMark || len(zArgs) != 0 {
				continue
			}
			if !z.Eq(nReceiver) {
				continue
			}

			break check2
		}
		return nil
	}

	// Check "length <= (end - *ptr_ptr)".
check3:
	for {
		for _, x := range q.facts {
			if x.ID0().Key() != t.KeyXBinaryLessEq {
				continue
			}

			// Check that the LHS is "l as u64".
			lhs := x.LHS().Expr()
			if lhs.ID0().Key() != t.KeyXBinaryAs {
				continue
			}
			llhs, lrhs := lhs.LHS().Expr(), lhs.RHS().TypeExpr()
			if !llhs.Eq(l) || !lrhs.Eq(typeExprU64) {
				continue
			}

			// Check that the RHS is "nReceiver.available()".
			y, method, yArgs := splitReceiverMethodArgs(x.RHS().Expr())
			if method != t.KeyAvailable || len(yArgs) != 0 {
				continue
			}
			if !y.Eq(nReceiver) {
				continue
			}

			break check3
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

	// TODO: check that nReceiver's type is actually a reader1 or writer1.

	if nMethod == t.KeyUnreadU8 {
		// unread_u8 can never suspend, only succeed or fail.
		n.SetProvenNotToSuspend()
		return nil
	}

	if nMethod < t.Key(len(ioMethodAdvances)) {
		if advance := ioMethodAdvances[nMethod]; advance != nil {
			return q.optimizeIOMethodAdvance(n, nReceiver, advance)
		}
	}

	return nil
}

func (q *checker) optimizeIOMethodAdvance(n *a.Expr, receiver *a.Expr, advance *big.Int) error {
	return q.facts.update(func(x *a.Expr) (*a.Expr, error) {
		op := x.ID0().Key()
		if op != t.KeyXBinaryGreaterEq && op != t.KeyXBinaryGreaterThan {
			return x, nil
		}

		rcv := x.RHS().Expr().ConstValue()
		if rcv == nil {
			return x, nil
		}

		// Check that lhs is "receiver.available()".
		lhs := x.LHS().Expr()
		if lhs.ID0().Key() != t.KeyOpenParen || len(lhs.Args()) != 0 {
			return x, nil
		}
		lhs = lhs.LHS().Expr()
		if lhs.ID0().Key() != t.KeyDot || lhs.ID1().Key() != t.KeyAvailable {
			return x, nil
		}
		lhs = lhs.LHS().Expr()
		if !lhs.Eq(receiver) {
			return x, nil
		}

		// Check if the bytes available is >= the bytes needed. If so, update
		// rcv to be the bytes remaining. If not, discard the fact x.
		//
		// TODO: mark the call as proven not to suspend.
		if op == t.KeyXBinaryGreaterThan {
			op = t.KeyXBinaryGreaterEq
			rcv = big.NewInt(0).Add(rcv, one)
		}
		if rcv.Cmp(advance) < 0 {
			return nil, nil
		}
		rcv = big.NewInt(0).Sub(rcv, advance)

		// Create a new a.Expr to hold the adjusted RHS constant value rcv.
		id, err := q.tm.Insert(rcv.String())
		if err != nil {
			return nil, err
		}
		o := a.NewExpr(a.FlagsTypeChecked, 0, id, nil, nil, nil, nil)
		o.SetConstValue(rcv)
		o.SetMType(typeExprIdeal)

		if !n.CallSuspendible() {
			return nil, fmt.Errorf("check: internal error: inconsistent suspendible-ness for %q", n.String(q.tm))
		}
		n.SetProvenNotToSuspend()

		return a.NewExpr(x.Node().Raw().Flags(), t.IDXBinaryGreaterEq, 0, x.LHS(), nil, o.Node(), nil), nil
	})
}

var ioMethodAdvances = [256]*big.Int{
	t.KeyReadU8:    one,
	t.KeyReadU16BE: two,
	t.KeyReadU16LE: two,
	t.KeyReadU32BE: four,
	t.KeyReadU32LE: four,
	t.KeyReadU64BE: eight,
	t.KeyReadU64LE: eight,

	t.KeyWriteU8:    one,
	t.KeyWriteU16BE: two,
	t.KeyWriteU16LE: two,
	t.KeyWriteU32BE: four,
	t.KeyWriteU32LE: four,
	t.KeyWriteU64BE: eight,
	t.KeyWriteU64LE: eight,
}
