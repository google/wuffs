// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

// binds returns fact's lower or upper bound for id.
func binds(fact *a.Expr, id t.ID) (*big.Int, *big.Int) {
	op := fact.ID0()
	if !op.IsBinaryOp() || op.Key() == t.KeyAs {
		return nil, nil
	}

	lhs := fact.LHS().Expr()
	rhs := fact.RHS().Expr()
	cv := (*big.Int)(nil)
	idLeft := lhs.ID0() == 0 && lhs.ID1() == id
	if idLeft {
		cv = rhs.ConstValue()
	} else {
		if rhs.ID0() != 0 || rhs.ID1() != id {
			return nil, nil
		}
		cv = lhs.ConstValue()
	}
	if cv == nil {
		return nil, nil
	}

	switch op.Key() {
	case t.KeyXBinaryLessThan:
		if idLeft {
			return nil, sub1(cv)
		} else {
			return add1(cv), nil
		}
	case t.KeyXBinaryLessEq:
		if idLeft {
			return nil, cv
		} else {
			return cv, nil
		}
	case t.KeyXBinaryGreaterEq:
		if idLeft {
			return cv, nil
		} else {
			return nil, cv
		}
	case t.KeyXBinaryGreaterThan:
		if idLeft {
			return add1(cv), nil
		} else {
			return nil, sub1(cv)
		}
	}

	return nil, nil
}
