// Copyright 2018 The Wuffs Authors.
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

package cgen

// This file deals with liveness analysis, determining whether local variables
// need to be kept: saved / loaded when suspending / resuming a coroutine. For
// simplicity, this is a boolean property of each local variable. The analysis
// does not determine that a local variable needs saving at some CSPs
// (coroutine suspension points) but not others.
//
// For example, in this code:
//
//   i = etc
//   yield foo  // The first CSP.
//   j = 0
//   while j < i {
//     a[j] = 42
//     j += 1
//   }
//   c = args.src.read_u8?()  // The second CSP.
//   k = a[c]
//
// There are two CSPs. The i local variable will need to be kept, as it is
// written to before the first CSP and read from after the first CSP.
// Similarly, the a local variable (an array) will need to be kept, as its use
// crosses the second CSP. The j local variable need not be kept, as all of its
// uses happens between two consecutive CSPs. To be precise, for every possible
// code path (including if branches and while loops), each read from j is
// preceded by a write to j without an intervening CSP. The c and k local
// variables also need not be kept, as their uses are all after the last CSP.
//
// Algorithmically, we walk through a function's statements in two passes. The
// first pass finds all of the local variables, and assigns an integer index to
// each one. The second tracks (in a slice indexed by that integer index) each
// local variable's liveness, one of three possible states: none, weak and
// strong. Strong means that the local variable definitely needs to be kept.
// Weak means that we have (in the worst case, for branches' and loops'
// multiple paths) seen a CSP without a write afterwards, so that seeing a read
// would change the liveness to strong. None means that we have not seen a CSP
// since the last write.
//
// Essentially, the state transitions are: strong is sticky, weak to strong
// happens on a read, weak to none happens on a write, and none to weak happens
// on a CSP. When reconciling multiple code paths, the strongest wins, where
// strong > weak and weak > none. Loops are repeated until the reconciliations
// between the N'th and N+1'th iteration are all no-ops: a steady state has
// been reached. There can be multiple reconciliations per iteration, due to
// break and continue statements.
//
// Care is taken with a coroutine call in a statement or expression. Arguments
// to the coroutine are 'inside' and are before the CSP. The rest of the
// expression is 'outside' and are after the CSP.

import (
	"fmt"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

// liveness tracks whether a local variable's value will need to be kept across
// CSPs (coroutine suspension points).
type liveness uint32

const (
	livenessNone   = liveness(0)
	livenessWeak   = liveness(1)
	livenessStrong = liveness(2)
)

// livenesses is a slice of liveness values, one per local variable.
type livenesses []liveness

func (r livenesses) clone() livenesses {
	return append(livenesses(nil), r...)
}

func (r livenesses) reconcile(s livenesses) (changed bool) {
	if len(r) != len(s) {
		panic("livenesses have different length")
	}
	for i := range r {
		if r[i] < s[i] {
			r[i] = s[i]
			changed = true
		}
	}
	return changed
}

func (r livenesses) lowerWeakToNone(i int) {
	if r[i] == livenessWeak {
		r[i] = livenessNone
	}
}

func (r livenesses) raiseWeakToStrong(i int) {
	if r[i] == livenessWeak {
		r[i] = livenessStrong
	}
}

func (r livenesses) raiseNoneToWeak() {
	for i, x := range r {
		if x == livenessNone {
			r[i] = livenessWeak
		}
	}
}

type loopLivenesses struct {
	before  livenesses
	after   livenesses
	changed bool
}

type livenessHelper struct {
	tm    *t.Map
	vars  map[t.ID]int // Maps from local variable name to livenesses index.
	loops map[a.Loop]*loopLivenesses
}

func (g *gen) findVars() error {
	h := livenessHelper{
		tm:    g.tm,
		vars:  map[t.ID]int{},
		loops: map[a.Loop]*loopLivenesses{},
	}

	f := g.currFunk.astFunc
	for _, n := range f.Body() {
		if n.Kind() != a.KVar {
			break
		}
		g.currFunk.varList = append(g.currFunk.varList, n.AsVar())
		h.vars[n.AsVar().Name()] = len(h.vars)
	}

	if f.Effect().Coroutine() {
		r := make(livenesses, len(h.vars))
		if err := h.doBlock(r, f.Body(), 0); err != nil {
			return err
		}

		g.currFunk.varResumables = map[t.ID]bool{}
		for i, v := range g.currFunk.varList {
			g.currFunk.varResumables[v.Name()] = r[i] == livenessStrong
		}
	}

	return nil
}

func (h *livenessHelper) doBlock(r livenesses, block []*a.Node, depth uint32) error {
	if depth > a.MaxBodyDepth {
		return fmt.Errorf("body recursion depth too large")
	}
	depth++

loop:
	for _, o := range block {
		switch o.Kind() {
		case a.KAssign:
			if err := h.doAssign(r, o.AsAssign(), depth); err != nil {
				return err
			}

		case a.KExpr:
			if err := h.doExpr(r, o.AsExpr()); err != nil {
				return err
			}

		case a.KIOBind:
			if err := h.doIOBind(r, o.AsIOBind(), depth); err != nil {
				return err
			}

		case a.KIf:
			if err := h.doIf(r, o.AsIf(), depth); err != nil {
				return err
			}

		case a.KIterate:
			if err := h.doIterate(r, o.AsIterate(), depth); err != nil {
				return err
			}

		case a.KJump:
			if err := h.doJump(r, o.AsJump(), depth); err != nil {
				return err
			}
			break loop

		case a.KRet:
			if err := h.doRet(r, o.AsRet(), depth); err != nil {
				return err
			}
			if o.AsRet().Keyword() == t.IDReturn {
				break loop
			}

		case a.KVar:
			if err := h.doVar(r, o.AsVar(), depth); err != nil {
				return err
			}

		case a.KWhile:
			if err := h.doWhile(r, o.AsWhile(), depth); err != nil {
				return err
			}
		}
	}
	return nil
}

func (h *livenessHelper) doAssign(r livenesses, n *a.Assign, depth uint32) error {
	if n.Operator() == t.IDEqQuestion {
		if err := h.doExpr1(r, n.RHS(), 0); err != nil {
			return err
		}
	} else {
		if err := h.doExpr(r, n.RHS()); err != nil {
			return err
		}
	}

	if n.LHS() == nil {
		return nil
	}

	// If the LHS is not a local variable (e.g. "this.foo[bar] = etc", or if
	// the LHS is implicitly also on the RHS (e.g. for a += or *= operator),
	// walk the LHS Expr.
	if n.LHS().Operator() != 0 || (n.Operator() != t.IDEq && n.Operator() != t.IDEqQuestion) {
		if err := h.doExpr(r, n.LHS()); err != nil {
			return err
		}
	}

	// If the LHS is just a local variable, then call lowerWeakToNone.
	if lhs := n.LHS(); lhs.Operator() == 0 {
		if i, ok := h.vars[lhs.Ident()]; !ok {
			return fmt.Errorf("unrecognized variable %q", lhs.Ident().Str(h.tm))
		} else {
			r.lowerWeakToNone(i)
		}
	}
	return nil
}

func (h *livenessHelper) doExpr(r livenesses, n *a.Expr) error {
	if err := h.doExpr1(r, n, 0); err != nil {
		return err
	}
	if n.Effect().Coroutine() {
		r.raiseNoneToWeak()
	}
	return nil
}

func (h *livenessHelper) doExpr1(r livenesses, n *a.Expr, depth uint32) error {
	if depth > a.MaxBodyDepth {
		return fmt.Errorf("body recursion depth too large")
	}
	depth++

	for _, o := range n.AsNode().AsRaw().SubNodes() {
		if o != nil && o.Kind() == a.KExpr {
			if err := h.doExpr1(r, o.AsExpr(), depth); err != nil {
				return err
			}
		}
	}
	for _, o := range n.Args() {
		e := (*a.Expr)(nil)
		switch o.Kind() {
		case a.KArg:
			e = o.AsArg().Value()
		case a.KExpr:
			e = o.AsExpr()
		default:
			return fmt.Errorf("unrecognized arg kind")
		}
		if err := h.doExpr1(r, e, depth); err != nil {
			return err
		}
	}

	if n.Operator() == 0 {
		if i, ok := h.vars[n.Ident()]; ok {
			r.raiseWeakToStrong(i)
		}
	}

	return nil
}

func (h *livenessHelper) doIOBind(r livenesses, n *a.IOBind, depth uint32) error {
	if err := h.doExpr(r, n.IO()); err != nil {
		return err
	}
	if err := h.doExpr(r, n.Arg1()); err != nil {
		return err
	}
	return h.doBlock(r, n.Body(), depth)
}

func (h *livenessHelper) doIf(r livenesses, n *a.If, depth uint32) error {
	scratch := make(livenesses, len(r))
	result := make(livenesses, len(r))
	for ; n != nil; n = n.ElseIf() {
		if err := h.doExpr(r, n.Condition()); err != nil {
			return err
		}

		copy(scratch, r)
		if err := h.doBlock(scratch, n.BodyIfTrue(), depth); err != nil {
			return err
		}
		result.reconcile(scratch)

		copy(scratch, r)
		if err := h.doBlock(scratch, n.BodyIfFalse(), depth); err != nil {
			return err
		}
		result.reconcile(scratch)
	}
	copy(r, result)
	return nil
}

func (h *livenessHelper) doIterate(r livenesses, n *a.Iterate, depth uint32) error {
	// TODO: ban jumps, rets and coroutine calls inside an iterate. Also ensure
	// that the iterate variable values are all pure expressions.
	for _, o := range n.Assigns() {
		o := o.AsAssign()
		if err := h.doExpr(r, o.RHS()); err != nil {
			return err
		}
		name := o.LHS().Ident()
		if i, ok := h.vars[name]; !ok {
			return fmt.Errorf("unrecognized variable %q", name.Str(h.tm))
		} else {
			r.lowerWeakToNone(i)
		}
	}
	return h.doBlock(r, n.Body(), depth)
}

func (h *livenessHelper) doJump(r livenesses, n *a.Jump, depth uint32) error {
	l := h.loops[n.JumpTarget()]
	switch n.Keyword() {
	case t.IDBreak:
		l.changed = l.before.reconcile(r) || l.changed
	case t.IDContinue:
		l.changed = l.after.reconcile(r) || l.changed
	default:
		return fmt.Errorf("unrecognized ast.Jump keyword")
	}
	return nil
}

func (h *livenessHelper) doRet(r livenesses, n *a.Ret, depth uint32) error {
	if err := h.doExpr(r, n.Value()); err != nil {
		return err
	}

	switch n.Keyword() {
	case t.IDReturn:
		// No-op.
	case t.IDYield:
		r.raiseNoneToWeak()
	default:
		return fmt.Errorf("unrecognized ast.Ret keyword")
	}
	return nil
}

func (h *livenessHelper) doVar(r livenesses, n *a.Var, depth uint32) error {
	name := n.Name()
	if i, ok := h.vars[name]; !ok {
		return fmt.Errorf("unrecognized variable %q", name.Str(h.tm))
	} else {
		r.lowerWeakToNone(i)
	}
	return nil
}

func (h *livenessHelper) doWhile(r livenesses, n *a.While, depth uint32) error {
	l := &loopLivenesses{
		before:  make(livenesses, len(r)),
		after:   make(livenesses, len(r)),
		changed: false,
	}
	h.loops[n] = l

	copy(l.before, r)
	if err := h.doExpr(r, n.Condition()); err != nil {
		return err
	}
	copy(l.after, r)

	for {
		copy(r, l.after)
		if err := h.doBlock(r, n.Body(), depth); err != nil {
			return err
		}
		l.changed = l.before.reconcile(r) || l.changed

		copy(r, l.before)
		if err := h.doExpr(r, n.Condition()); err != nil {
			return err
		}
		l.changed = l.after.reconcile(r) || l.changed

		if !l.changed {
			break
		}
		l.changed = false
	}

	copy(r, l.after)
	return nil
}
