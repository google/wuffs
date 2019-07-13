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

package cgen

import (
	"fmt"
	"math/big"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

type funk struct {
	bPrologue    buffer
	bBodyResume  buffer
	bBody        buffer
	bBodySuspend buffer
	bEpilogue    buffer

	astFunc       *a.Func
	cName         string
	coroID        uint32
	returnsStatus bool

	varList           []*a.Var
	varResumables     map[t.ID]bool
	derivedVars       map[t.ID]struct{}
	jumpTargets       map[a.Loop]uint32
	coroSuspPoint     uint32
	ioBinds           uint32
	tempW             uint32
	tempR             uint32
	usesEmptyIOBuffer bool
	usesScratch       bool
	hasGotoOK         bool
}

func (k *funk) jumpTarget(n a.Loop) (uint32, error) {
	if k.jumpTargets == nil {
		k.jumpTargets = map[a.Loop]uint32{}
	}
	if jt, ok := k.jumpTargets[n]; ok {
		return jt, nil
	}
	jt := uint32(len(k.jumpTargets))
	if jt == 1000000 {
		return 0, fmt.Errorf("too many jump targets")
	}
	k.jumpTargets[n] = jt
	return jt, nil
}

func (g *gen) funcCName(n *a.Func) string {
	if r := n.Receiver(); !r.IsZero() {
		// TODO: this isn't right if r[0] != 0, i.e. the receiver is from a
		// used package. There might be similar cases elsewhere in this
		// package.
		return g.pkgPrefix + r.Str(g.tm) + "__" + n.FuncName().Str(g.tm)
	}
	return g.pkgPrefix + n.FuncName().Str(g.tm)
}

// C++ related function signature constants.
const (
	cppNone          = 0 // Not C++, just plain C.
	cppInsideStruct  = 1
	cppOutsideStruct = 2
)

func (g *gen) writeFuncSignature(b *buffer, n *a.Func, cpp uint32) error {
	if cpp != cppNone {
		b.writes("inline ")
	} else if n.Public() {
		b.writes("WUFFS_BASE__MAYBE_STATIC ")
	} else {
		b.writes("static ")
	}

	// TODO: write n's return values.
	if n.Effect().Coroutine() {
		b.writes("wuffs_base__status ")
	} else if out := n.Out(); out == nil {
		b.writes("wuffs_base__empty_struct ")
		// TODO: does writeCTypeName generate the right C if out is an array?
	} else if err := g.writeCTypeName(b, out, "", ""); err != nil {
		return err
	}

	// The empty // comment makes clang-format place the function name at the
	// start of a line.
	b.writes("//\n")

	switch cpp {
	case cppNone:
		b.writes(g.funcCName(n))
	case cppInsideStruct:
		b.writes(n.FuncName().Str(g.tm))
	case cppOutsideStruct:
		b.writes(g.pkgPrefix)
		b.writes(n.Receiver().Str(g.tm))
		b.writes("::")
		b.writes(n.FuncName().Str(g.tm))
	}

	b.writeb('(')
	comma := false
	if cpp == cppNone {
		if r := n.Receiver(); !r.IsZero() {
			if n.Effect().Pure() {
				b.writes("const ")
			}
			b.printf("%s%s *self", g.pkgPrefix, r.Str(g.tm))
			comma = true
		}
	}

	for _, o := range n.In().Fields() {
		if comma {
			b.writeb(',')
		}
		comma = true
		o := o.AsField()
		if err := g.writeCTypeName(b, o.XType(), aPrefix, o.Name().Str(g.tm)); err != nil {
			return err
		}
	}

	b.printf(")")
	if cpp != cppNone && !n.Receiver().IsZero() && n.Effect().Pure() {
		b.writes(" const ")
	}
	return nil
}

func (g *gen) writeFuncPrototype(b *buffer, n *a.Func) error {
	if err := g.writeFuncSignature(b, n, cppNone); err != nil {
		return err
	}
	b.writes(";\n\n")
	return nil
}

func (g *gen) writeFuncImpl(b *buffer, n *a.Func) error {
	k := g.funks[n.QQID()]

	b.printf("// -------- func %s.%s\n\n", g.pkgName, n.QQID().Str(g.tm))
	if err := g.writeFuncSignature(b, n, cppNone); err != nil {
		return err
	}
	b.writes("{\n")
	b.writex(k.bPrologue)
	if k.astFunc.Effect().Coroutine() {
		b.writex(k.bBodyResume)
	}
	b.writex(k.bBody)
	if k.astFunc.Effect().Coroutine() {
		b.writex(k.bBodySuspend)
	} else if k.hasGotoOK {
		b.writes("\ngoto ok;ok:\n") // The goto avoids the "unused label" warning.
	}
	b.writex(k.bEpilogue)
	b.writes("}\n\n")
	return nil
}

func (g *gen) gatherFuncImpl(_ *buffer, n *a.Func) error {
	if n.Public() && n.Effect().Coroutine() {
		g.numPublicCoroutines++
	}

	g.currFunk = funk{
		astFunc: n,
		cName:   g.funcCName(n),
		coroID:  g.numPublicCoroutines,

		returnsStatus: n.Effect().Coroutine() ||
			((n.Out() != nil) && n.Out().IsStatus()),
	}

	if err := g.findVars(); err != nil {
		return err
	}
	g.findDerivedVars()

	if err := g.writeFuncImplPrologue(&g.currFunk.bPrologue); err != nil {
		return err
	}
	if err := g.writeFuncImplBodyResume(&g.currFunk.bBodyResume); err != nil {
		return err
	}
	if err := g.writeFuncImplBody(&g.currFunk.bBody); err != nil {
		return err
	}
	if err := g.writeFuncImplBodySuspend(&g.currFunk.bBodySuspend); err != nil {
		return err
	}
	if err := g.writeFuncImplEpilogue(&g.currFunk.bEpilogue); err != nil {
		return err
	}

	if g.currFunk.tempW != g.currFunk.tempR {
		return fmt.Errorf("internal error: temporary variable count out of sync")
	}
	g.funks[n.QQID()] = g.currFunk
	return nil
}

func (g *gen) writeOutParamZeroValue(b *buffer, typ *a.TypeExpr) error {
	if typ == nil {
		b.writes("wuffs_base__make_empty_struct()")
		return nil
	} else if typ.IsNumType() {
		b.writes("0")
		return nil
	} else if typ.IsSliceType() {
		if inner := typ.Inner(); (inner.Decorator() == 0) && (inner.QID() == t.QID{t.IDBase, t.IDU8}) {
			b.writes("wuffs_base__make_slice_u8(NULL, 0)")
			return nil
		}
	} else if (typ.Decorator() == 0) && (typ.QID()[0] == t.IDBase) {
		switch typ.QID()[1] {
		case t.IDRangeIEU32:
			b.writes("wuffs_base__utility__make_range_ie_u32(0, 0)")
			return nil
		case t.IDRangeIIU32:
			b.writes("wuffs_base__utility__make_range_ii_u32(0, 0)")
			return nil
		case t.IDRangeIEU64:
			b.writes("wuffs_base__utility__make_range_ie_u64(0, 0)")
			return nil
		case t.IDRangeIIU64:
			b.writes("wuffs_base__utility__make_range_ii_u64(0, 0)")
			return nil
		case t.IDRectIEU32:
			b.writes("wuffs_base__utility__make_rect_ie_u32(0, 0, 0, 0)")
			return nil
		case t.IDRectIIU32:
			b.writes("wuffs_base__utility__make_rect_ii_u32(0, 0, 0, 0)")
			return nil
		}
	}
	return fmt.Errorf("internal error: cannot write the zero value of type %q", typ.Str(g.tm))
}

func (g *gen) writeFuncImplPrologue(b *buffer) error {
	// Check the initialized/disabled state and the "self" arg.
	if g.currFunk.astFunc.Public() && !g.currFunk.astFunc.Receiver().IsZero() {
		out := g.currFunk.astFunc.Out()

		b.writes("if (!self) { return ")
		if g.currFunk.returnsStatus {
			b.writes("wuffs_base__error__bad_receiver")
		} else if err := g.writeOutParamZeroValue(b, out); err != nil {
			return err
		}
		b.writes(";}")

		if g.currFunk.astFunc.Effect().Pure() {
			b.writes("if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&")
			b.writes("    (self->private_impl.magic != WUFFS_BASE__DISABLED)) {")
		} else {
			b.writes("if (self->private_impl.magic != WUFFS_BASE__MAGIC) {")
		}
		b.writes("return ")
		if g.currFunk.returnsStatus {
			b.writes("(self->private_impl.magic == WUFFS_BASE__DISABLED) " +
				"? wuffs_base__error__disabled_by_previous_error " +
				": wuffs_base__error__initialize_not_called")
		} else if err := g.writeOutParamZeroValue(b, out); err != nil {
			return err
		}

		b.writes(";}\n")
	}

	// For public functions, check (at runtime) the other args for bounds and
	// null-ness. For private functions, those checks are done at compile time.
	if g.currFunk.astFunc.Public() {
		if err := g.writeFuncImplArgChecks(b, g.currFunk.astFunc); err != nil {
			return err
		}

		// For public coroutines, check that we are not suspended in an active
		// coroutine.
		if g.currFunk.astFunc.Effect().Coroutine() {
			b.printf("if ((self->private_impl.active_coroutine != 0) && "+
				"(self->private_impl.active_coroutine != %d)) {\n", g.currFunk.coroID)
			b.writes("self->private_impl.magic = WUFFS_BASE__DISABLED;\n")
			b.writes("return wuffs_base__error__interleaved_coroutine_calls;\n")
			b.writes("}\n")
			b.writes("self->private_impl.active_coroutine = 0;\n")
		}
	}

	if g.currFunk.astFunc.Effect().Coroutine() ||
		(g.currFunk.returnsStatus && (len(g.currFunk.derivedVars) > 0)) {
		// TODO: rename the "status" variable to "ret"?
		b.printf("wuffs_base__status status = NULL;\n")
	}
	b.writes("\n")

	// Generate the local variables.
	if err := g.writeVars(b, &g.currFunk, false); err != nil {
		return err
	}
	b.writes("\n")

	if g.currFunk.derivedVars != nil {
		for _, o := range g.currFunk.astFunc.In().Fields() {
			o := o.AsField()
			if err := g.writeLoadDerivedVar(b, "", aPrefix, o.Name(), o.XType(), true); err != nil {
				return err
			}
		}
		b.writes("\n")
	}
	return nil
}

func (g *gen) writeFuncImplBodyResume(b *buffer) error {
	if g.currFunk.astFunc.Effect().Coroutine() {
		// TODO: don't hard-code [0], and allow recursive coroutines.
		b.printf("uint32_t coro_susp_point = self->private_impl.%s%s[0];\n",
			pPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))
		b.printf("if (coro_susp_point) {\n")
		if err := g.writeResumeSuspend(b, &g.currFunk, false); err != nil {
			return err
		}
		b.writes("}\n")
		// Generate a coroutine switch similiar to the technique in
		// https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
		//
		// The matching } is written below. See "Close the coroutine switch".
		b.writes("switch (coro_susp_point) {\nWUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;\n\n")
	}
	return nil
}

func (g *gen) writeFuncImplBody(b *buffer) error {
	for _, o := range g.currFunk.astFunc.Body() {
		if err := g.writeStatement(b, o, 0); err != nil {
			return err
		}
	}
	return nil
}

func (g *gen) writeFuncImplBodySuspend(b *buffer) error {
	if g.currFunk.astFunc.Effect().Coroutine() {
		// We've reached the end of the function body. Reset the coroutine
		// suspension point so that the next call to this function starts at
		// the top.
		b.writes("\ngoto ok;ok:") // The goto avoids the "unused label" warning.
		b.printf("self->private_impl.%s%s[0] = 0;\n",
			pPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))
		b.writes("goto exit; }\n\n") // Close the coroutine switch.

		b.writes("goto suspend;suspend:") // The goto avoids the "unused label" warning.

		b.printf("self->private_impl.%s%s[0] = "+
			"wuffs_base__status__is_suspension(status) ? coro_susp_point : 0;\n",
			pPrefix, g.currFunk.astFunc.FuncName().Str(g.tm))
		if g.currFunk.astFunc.Public() {
			b.printf("self->private_impl.active_coroutine = "+
				"wuffs_base__status__is_suspension(status) ? %d : 0;\n", g.currFunk.coroID)
		}
		if err := g.writeResumeSuspend(b, &g.currFunk, true); err != nil {
			return err
		}
		b.writes("\n")
	}
	return nil
}

func (g *gen) writeFuncImplEpilogue(b *buffer) error {
	if g.currFunk.astFunc.Effect().Coroutine() ||
		(g.currFunk.returnsStatus && (len(g.currFunk.derivedVars) > 0)) {

		b.writes("goto exit;exit:") // The goto avoids the "unused label" warning.
	}

	if g.currFunk.derivedVars != nil {
		for _, o := range g.currFunk.astFunc.In().Fields() {
			o := o.AsField()
			if err := g.writeSaveDerivedVar(b, "", aPrefix, o.Name(), o.XType()); err != nil {
				return err
			}
		}
		b.writes("\n")
	}

	if g.currFunk.astFunc.Effect().Coroutine() ||
		(g.currFunk.returnsStatus && (len(g.currFunk.derivedVars) > 0)) {

		if g.currFunk.astFunc.Public() {
			b.writes("if (wuffs_base__status__is_error(status)) { " +
				"self->private_impl.magic = WUFFS_BASE__DISABLED; }\n")
		}
		b.writes("return status;\n")
	} else if g.currFunk.astFunc.Out() == nil {
		b.writes("return wuffs_base__make_empty_struct();\n")
	}
	return nil
}

func (g *gen) writeFuncImplArgChecks(b *buffer, n *a.Func) error {
	checks := []string(nil)

	for _, o := range n.In().Fields() {
		o := o.AsField()
		oTyp := o.XType()

		// TODO: Also check elements, for array-typed arguments.
		switch {
		case oTyp.IsIOType() || (oTyp.Decorator() == t.IDPtr):
			checks = append(checks, fmt.Sprintf("!%s%s", aPrefix, o.Name().Str(g.tm)))

		case oTyp.IsRefined():
			bounds := [2]*big.Int{}
			for i, bound := range oTyp.Bounds() {
				if bound != nil {
					if cv := bound.ConstValue(); cv != nil {
						bounds[i] = cv
					}
				}
			}
			if qid := oTyp.QID(); qid[0] == t.IDBase {
				if key := qid[1]; key < t.ID(len(numTypeBounds)) {
					ntb := numTypeBounds[key]
					for i := 0; i < 2; i++ {
						if bounds[i] != nil && ntb[i] != nil && bounds[i].Cmp(ntb[i]) == 0 {
							bounds[i] = nil
							continue
						}
					}
				}
			}
			for i, bound := range bounds {
				if bound != nil {
					op := '<'
					if i != 0 {
						op = '>'
					}
					checks = append(checks, fmt.Sprintf("%s%s %c %s", aPrefix, o.Name().Str(g.tm), op, bound))
				}
			}
		}
	}

	if len(checks) == 0 {
		return nil
	}

	b.writes("if (")
	for i, c := range checks {
		if i != 0 {
			b.writes(" || ")
		}
		b.writes(c)
	}
	b.writes(") {")
	b.writes("self->private_impl.magic = WUFFS_BASE__DISABLED;\n")
	if g.currFunk.astFunc.Effect().Coroutine() {
		b.writes("return wuffs_base__error__bad_argument;\n\n")
	} else {
		// TODO: don't assume that the return type is empty.
		b.printf("return wuffs_base__make_empty_struct();\n")
	}
	b.writes("}\n")
	return nil
}

var numTypeBounds = [...][2]*big.Int{
	t.IDI8:   {big.NewInt(-1 << 7), big.NewInt(1<<7 - 1)},
	t.IDI16:  {big.NewInt(-1 << 15), big.NewInt(1<<15 - 1)},
	t.IDI32:  {big.NewInt(-1 << 31), big.NewInt(1<<31 - 1)},
	t.IDI64:  {big.NewInt(-1 << 63), big.NewInt(1<<63 - 1)},
	t.IDU8:   {zero, big.NewInt(0).SetUint64(1<<8 - 1)},
	t.IDU16:  {zero, big.NewInt(0).SetUint64(1<<16 - 1)},
	t.IDU32:  {zero, big.NewInt(0).SetUint64(1<<32 - 1)},
	t.IDU64:  {zero, big.NewInt(0).SetUint64(1<<64 - 1)},
	t.IDBool: {zero, one},
}
