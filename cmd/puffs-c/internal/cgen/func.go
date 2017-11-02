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

package cgen

import (
	"fmt"
	"math/big"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

type funk struct {
	bHeader      buffer
	bBodyResume  buffer
	bBody        buffer
	bBodySuspend buffer
	bFooter      buffer

	astFunc       *a.Func
	cName         string
	derivedVars   map[t.ID]struct{}
	jumpTargets   map[a.Loop]uint32
	coroSuspPoint uint32
	tempW         uint32
	tempR         uint32
	public        bool
	suspendible   bool
	usesScratch   bool
	shortReads    []string
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
	if r := n.Receiver(); r != 0 {
		return g.pkgPrefix + r.String(g.tm) + "__" + n.Name().String(g.tm)
	}
	return g.pkgPrefix + n.Name().String(g.tm)
}

func (g *gen) writeFuncSignature(b *buffer, n *a.Func) error {
	// TODO: write n's return values.
	if n.Suspendible() {
		b.printf("%sstatus ", g.pkgPrefix)
	} else if outFields := n.Out().Fields(); len(outFields) == 0 {
		b.writes("void ")
	} else if len(outFields) == 1 {
		// TODO: does this generate the right C if the XType is an array?
		if err := g.writeCTypeName(b, outFields[0].Field().XType(), "", ""); err != nil {
			return err
		}
	} else {
		return fmt.Errorf("TODO: multiple return values")
	}

	b.writes(g.funcCName(n))
	b.writeb('(')

	comma := false
	if r := n.Receiver(); r != 0 {
		b.printf("%s%s *self", g.pkgPrefix, r.String(g.tm))
		comma = true
	}
	for _, o := range n.In().Fields() {
		if comma {
			b.writeb(',')
		}
		comma = true
		o := o.Field()
		if err := g.writeCTypeName(b, o.XType(), aPrefix, o.Name().String(g.tm)); err != nil {
			return err
		}
	}

	b.printf(")")
	return nil
}

func (g *gen) writeFuncPrototype(b *buffer, n *a.Func) error {
	if err := g.writeFuncSignature(b, n); err != nil {
		return err
	}
	b.writes(";\n\n")
	return nil
}

func (g *gen) writeFuncImpl(b *buffer, n *a.Func) error {
	k := g.funks[n.QID()]

	if err := g.writeFuncSignature(b, n); err != nil {
		return err
	}
	b.writes("{\n")
	b.writex(k.bHeader)
	if k.suspendible && k.coroSuspPoint > 0 {
		b.writex(k.bBodyResume)
	}
	b.writex(k.bBody)
	if k.suspendible && k.coroSuspPoint > 0 {
		b.writex(k.bBodySuspend)
	}
	b.writex(k.bFooter)
	b.writes("}\n\n")
	return nil
}

func (g *gen) gatherFuncImpl(_ *buffer, n *a.Func) error {
	g.currFunk = funk{
		astFunc:     n,
		cName:       g.funcCName(n),
		public:      n.Public(),
		suspendible: n.Suspendible(),
	}

	if err := g.writeFuncImplHeader(&g.currFunk.bHeader); err != nil {
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
	if err := g.writeFuncImplFooter(&g.currFunk.bFooter); err != nil {
		return err
	}

	if g.currFunk.tempW != g.currFunk.tempR {
		return fmt.Errorf("internal error: temporary variable count out of sync")
	}
	g.funks[n.QID()] = g.currFunk
	return nil
}

func (g *gen) writeFuncImplHeader(b *buffer) error {
	// Check the previous status and the "self" arg.
	if g.currFunk.public && g.currFunk.astFunc.Receiver() != 0 {
		b.writes("if (!self) {")
		if g.currFunk.suspendible {
			b.printf("return %sERROR_BAD_RECEIVER;", g.PKGPREFIX)
		} else {
			b.printf("return;")
		}
		b.writes("}")

		b.printf("if (self->private_impl.magic != PUFFS_BASE__MAGIC) {"+
			"self->private_impl.status = %sERROR_INITIALIZER_NOT_CALLED; }", g.PKGPREFIX)

		b.writes("if (self->private_impl.status < 0) {")
		if g.currFunk.suspendible {
			b.writes("return self->private_impl.status;")
		} else {
			b.writes("return;")
		}
		b.writes("}\n")
	}

	if g.currFunk.suspendible {
		b.printf("%sstatus status = %sSTATUS_OK;\n", g.pkgPrefix, g.PKGPREFIX)
	}

	// For public functions, check (at runtime) the other args for bounds and
	// null-ness. For private functions, those checks are done at compile time.
	if g.currFunk.public {
		if err := g.writeFuncImplArgChecks(b, g.currFunk.astFunc); err != nil {
			return err
		}
	}
	b.writes("\n")

	// Generate the local variables.
	if err := g.writeVars(b, g.currFunk.astFunc.Body(), false, true); err != nil {
		return err
	}
	b.writes("\n")

	if g.currFunk.suspendible {
		g.findDerivedVars()
		for _, o := range g.currFunk.astFunc.In().Fields() {
			o := o.Field()
			if err := g.writeLoadDerivedVar(b, o.Name(), o.XType(), true); err != nil {
				return err
			}
		}
		b.writes("\n")
	}
	return nil
}

func (g *gen) writeFuncImplBodyResume(b *buffer) error {
	if g.currFunk.suspendible {
		// TODO: don't hard-code [0], and allow recursive coroutines.
		b.printf("uint32_t coro_susp_point = self->private_impl.%s%s[0].coro_susp_point;\n",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))
		b.printf("if (coro_susp_point) {\n")
		if err := g.writeResumeSuspend(b, g.currFunk.astFunc.Body(), false); err != nil {
			return err
		}
		b.writes("}\n")
		// Generate a coroutine switch similiar to the technique in
		// https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
		//
		// The matching } is written below. See "Close the coroutine switch".
		b.writes("switch (coro_susp_point) {\nPUFFS_BASE__COROUTINE_SUSPENSION_POINT(0);\n\n")
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
	if g.currFunk.suspendible {
		// We've reached the end of the function body. Reset the coroutine
		// suspension point so that the next call to this function starts at
		// the top.
		b.writes("\ngoto ok;\n") // Avoid the "unused label" warning.
		b.writes("ok:\n")
		b.printf("self->private_impl.%s%s[0].coro_susp_point = 0;\n",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))
		b.writes("goto exit; }\n\n") // Close the coroutine switch.

		b.writes("goto suspend;\n") // Avoid the "unused label" warning.
		b.writes("suspend:\n")

		b.printf("self->private_impl.%s%s[0].coro_susp_point = coro_susp_point;\n",
			cPrefix, g.currFunk.astFunc.Name().String(g.tm))
		if err := g.writeResumeSuspend(b, g.currFunk.astFunc.Body(), true); err != nil {
			return err
		}
		b.writes("\n")
	}
	return nil
}

func (g *gen) writeFuncImplFooter(b *buffer) error {
	if g.currFunk.suspendible {
		b.writes("exit:")

		for _, o := range g.currFunk.astFunc.In().Fields() {
			o := o.Field()
			if err := g.writeSaveDerivedVar(b, o.Name(), o.XType(), true); err != nil {
				return err
			}
		}
		b.writes("\n")

		if g.currFunk.public {
			b.writes("self->private_impl.status = status;\n")
		}
		b.writes("return status;\n\n")

		shortReadsSeen := map[string]struct{}{}
		for _, sr := range g.currFunk.shortReads {
			if _, ok := shortReadsSeen[sr]; ok {
				continue
			}
			shortReadsSeen[sr] = struct{}{}
			if err := template_short_read(b, template_args_short_read{
				PKGPREFIX: g.PKGPREFIX,
				name:      sr,
			}); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) writeFuncImplArgChecks(b *buffer, n *a.Func) error {
	checks := []string(nil)

	for _, o := range n.In().Fields() {
		o := o.Field()
		oTyp := o.XType()
		if oTyp.Decorator().Key() != t.KeyPtr && !oTyp.IsRefined() {
			// TODO: Also check elements, for array-typed arguments.
			continue
		}

		switch {
		case oTyp.Decorator().Key() == t.KeyPtr:
			checks = append(checks, fmt.Sprintf("!%s%s", aPrefix, o.Name().String(g.tm)))

		case oTyp.IsRefined():
			bounds := [2]*big.Int{}
			for i, bound := range oTyp.Bounds() {
				if bound != nil {
					if cv := bound.ConstValue(); cv != nil {
						bounds[i] = cv
					}
				}
			}
			if key := oTyp.Name().Key(); key < t.Key(len(numTypeBounds)) {
				ntb := numTypeBounds[key]
				for i := 0; i < 2; i++ {
					if bounds[i] != nil && ntb[i] != nil && bounds[i].Cmp(ntb[i]) == 0 {
						bounds[i] = nil
						continue
					}
				}
			}
			for i, bound := range bounds {
				if bound != nil {
					op := '<'
					if i != 0 {
						op = '>'
					}
					checks = append(checks, fmt.Sprintf("%s%s %c %s", aPrefix, o.Name().String(g.tm), op, bound))
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
	if g.currFunk.suspendible {
		b.printf("status = %sERROR_BAD_ARGUMENT; goto exit;", g.PKGPREFIX)
	} else if n.Receiver() != 0 {
		b.printf("self->private_impl.status = %sERROR_BAD_ARGUMENT; return;", g.PKGPREFIX)
	} else {
		b.printf("return;")
	}
	b.writes("}\n")
	return nil
}

var numTypeBounds = [256][2]*big.Int{
	t.KeyI8:    {big.NewInt(-1 << 7), big.NewInt(1<<7 - 1)},
	t.KeyI16:   {big.NewInt(-1 << 15), big.NewInt(1<<15 - 1)},
	t.KeyI32:   {big.NewInt(-1 << 31), big.NewInt(1<<31 - 1)},
	t.KeyI64:   {big.NewInt(-1 << 63), big.NewInt(1<<63 - 1)},
	t.KeyU8:    {zero, big.NewInt(0).SetUint64(1<<8 - 1)},
	t.KeyU16:   {zero, big.NewInt(0).SetUint64(1<<16 - 1)},
	t.KeyU32:   {zero, big.NewInt(0).SetUint64(1<<32 - 1)},
	t.KeyU64:   {zero, big.NewInt(0).SetUint64(1<<64 - 1)},
	t.KeyUsize: {zero, zero},
	t.KeyBool:  {zero, one},
}
