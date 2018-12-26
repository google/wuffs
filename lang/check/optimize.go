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
	n = n.LHS().AsExpr()
	if n.Operator() != t.IDDot {
		return nil, 0, nil
	}
	return n.LHS().AsExpr(), n.Ident(), args
}

func (q *checker) optimizeIOMethodAdvance(receiver *a.Expr, advance *big.Int, update bool) (retOK bool, retErr error) {
	// TODO: do two passes? The first one to non-destructively check retOK. The
	// second one to drop facts if indeed retOK? Otherwise, an advance
	// precondition failure will lose some of the facts in its error message.

	retErr = q.facts.update(func(x *a.Expr) (*a.Expr, error) {
		// TODO: update (discard?) any facts that merely mention
		// receiver.available(), even if they aren't an exact match.

		op := x.Operator()
		if op != t.IDXBinaryGreaterEq && op != t.IDXBinaryGreaterThan {
			return x, nil
		}

		rcv := x.RHS().AsExpr().ConstValue()
		if rcv == nil {
			return x, nil
		}

		// Check that lhs is "receiver.available()".
		lhs := x.LHS().AsExpr()
		if lhs.Operator() != t.IDOpenParen || len(lhs.Args()) != 0 {
			return x, nil
		}
		lhs = lhs.LHS().AsExpr()
		if lhs.Operator() != t.IDDot || lhs.Ident() != t.IDAvailable {
			return x, nil
		}
		lhs = lhs.LHS().AsExpr()
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

		if rcv.Cmp(advance) == 0 {
			// TODO: delete the (adjusted) fact, as newRCV will be zero, and
			// "foo.advance() >= 0" is redundant.
		}

		// Create a new a.Expr to hold the adjusted RHS constant value, newRCV.
		newRCV := big.NewInt(0).Sub(rcv, advance)
		o, err := makeConstValueExpr(q.tm, newRCV)
		if err != nil {
			return nil, err
		}

		return a.NewExpr(x.AsNode().AsRaw().Flags(),
			t.IDXBinaryGreaterEq, 0, 0, x.LHS(), nil, o.AsNode(), nil), nil
	})
	return retOK, retErr
}
