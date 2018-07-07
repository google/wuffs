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

//go:generate go run gen.go

package check

import (
	"errors"
	"fmt"
	"path"

	"github.com/google/wuffs/lang/base38"
	"github.com/google/wuffs/lang/builtin"
	"github.com/google/wuffs/lang/parse"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

type Error struct {
	Err           error
	Filename      string
	Line          uint32
	OtherFilename string
	OtherLine     uint32

	TMap  *t.Map
	Facts []*a.Expr
}

func (e *Error) Error() string {
	s := ""
	if e.OtherFilename != "" || e.OtherLine != 0 {
		s = fmt.Sprintf("%s at %s:%d and %s:%d",
			e.Err, e.Filename, e.Line, e.OtherFilename, e.OtherLine)
	} else {
		s = fmt.Sprintf("%s at %s:%d", e.Err, e.Filename, e.Line)
	}
	if e.TMap == nil {
		return s
	}
	b := append([]byte(s), ". Facts:\n"...)
	for _, f := range e.Facts {
		b = append(b, '\t')
		b = append(b, f.Str(e.TMap)...)
		b = append(b, '\n')
	}
	return string(b)
}

func Check(tm *t.Map, files []*a.File, resolveUse func(usePath string) ([]byte, error)) (*Checker, error) {
	for _, f := range files {
		if f == nil {
			return nil, errors.New("check: Check given a nil *ast.File")
		}
	}

	if len(files) > 1 {
		m := map[string]bool{}
		for _, f := range files {
			if m[f.Filename()] {
				return nil, fmt.Errorf("check: Check given duplicate filename %q", f.Filename())
			}
			m[f.Filename()] = true
		}
	}

	rMap := reasonMap{}
	for _, r := range reasons {
		if id := tm.ByName(r.s); id != 0 {
			rMap[id] = r.r
		}
	}
	c := &Checker{
		tm:           tm,
		resolveUse:   resolveUse,
		reasonMap:    rMap,
		packageID:    base38.Max + 1,
		consts:       map[t.QID]*a.Const{},
		funcs:        map[t.QQID]*a.Func{},
		localVars:    map[t.QQID]typeMap{},
		statuses:     map[t.QID]*a.Status{},
		structs:      map[t.QID]*a.Struct{},
		useBaseNames: map[t.ID]struct{}{},
	}

	_, err := c.parseBuiltInFuncs(builtin.Funcs, false)
	if err != nil {
		return nil, err
	}
	c.builtInSliceFuncs, err = c.parseBuiltInFuncs(builtin.SliceFuncs, true)
	if err != nil {
		return nil, err
	}
	c.builtInTableFuncs, err = c.parseBuiltInFuncs(builtin.TableFuncs, true)
	if err != nil {
		return nil, err
	}

	for _, phase := range phases {
		for _, f := range files {
			if phase.kind == a.KInvalid {
				if err := phase.check(c, nil); err != nil {
					return nil, err
				}
				continue
			}
			for _, n := range f.TopLevelDecls() {
				if n.Kind() != phase.kind {
					continue
				}
				if err := phase.check(c, n); err != nil {
					return nil, err
				}
			}
			setPlaceholderMBoundsMType(f.AsNode())
		}
	}

	return c, nil
}

var phases = [...]struct {
	kind  a.Kind
	check func(*Checker, *a.Node) error
}{
	{a.KPackageID, (*Checker).checkPackageID},
	{a.KInvalid, (*Checker).checkPackageIDExists},
	{a.KUse, (*Checker).checkUse},
	{a.KStatus, (*Checker).checkStatus},
	{a.KConst, (*Checker).checkConst},
	{a.KStruct, (*Checker).checkStructDecl},
	{a.KInvalid, (*Checker).checkStructCycles},
	{a.KStruct, (*Checker).checkStructFields},
	{a.KFunc, (*Checker).checkFuncSignature},
	{a.KFunc, (*Checker).checkFuncContract},
	{a.KFunc, (*Checker).checkFuncBody},
	{a.KStruct, (*Checker).checkFieldMethodCollisions},
	{a.KInvalid, (*Checker).checkAllTypeChecked},
	// TODO: check consts, funcs, structs and uses for name collisions.
}

type reason func(q *checker, n *a.Assert) error

type reasonMap map[t.ID]reason

type Checker struct {
	tm         *t.Map
	resolveUse func(usePath string) ([]byte, error)
	reasonMap  reasonMap

	packageID      uint32
	otherPackageID *a.PackageID

	consts    map[t.QID]*a.Const
	funcs     map[t.QQID]*a.Func
	localVars map[t.QQID]typeMap
	statuses  map[t.QID]*a.Status
	structs   map[t.QID]*a.Struct

	// useBaseNames are the base names of packages referred to by `use
	// "foo/bar"` lines. The keys are `bar`, not `"foo/bar"`.
	useBaseNames map[t.ID]struct{}

	builtInSliceFuncs map[t.QQID]*a.Func
	builtInTableFuncs map[t.QQID]*a.Func
	unsortedStructs   []*a.Struct

	statusByValue [256]t.ID
}

func (c *Checker) PackageID() uint32 { return c.packageID }

func (c *Checker) checkPackageID(node *a.Node) error {
	n := node.AsPackageID()
	if c.otherPackageID != nil {
		return &Error{
			Err:           fmt.Errorf("check: multiple packageid declarations"),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: c.otherPackageID.Filename(),
			OtherLine:     c.otherPackageID.Line(),
		}
	}
	raw := n.ID().Str(c.tm)
	s, ok := t.Unescape(raw)
	if !ok {
		return &Error{
			Err:      fmt.Errorf("check: %q is not a valid packageid", raw),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	u, ok := base38.Encode(s)
	if !ok || u == 0 {
		return &Error{
			Err:      fmt.Errorf("check: %q is not a valid packageid", s),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	c.packageID = u
	c.otherPackageID = n
	setPlaceholderMBoundsMType(n.AsNode())
	return nil
}

func (c *Checker) checkPackageIDExists(node *a.Node) error {
	if c.otherPackageID == nil {
		return fmt.Errorf("check: missing packageid declaration")
	}
	return nil
}

func (c *Checker) checkUse(node *a.Node) error {
	usePath := node.AsUse().Path()
	filename, ok := t.Unescape(usePath.Str(c.tm))
	if !ok {
		return fmt.Errorf("check: cannot resolve `use %s`", usePath.Str(c.tm))
	}
	baseName, err := c.tm.Insert(path.Base(filename))
	if err != nil {
		return fmt.Errorf("check: cannot resolve `use %s`: %v", usePath.Str(c.tm), err)
	}
	filename += ".wuffs"
	if _, ok := c.useBaseNames[baseName]; ok {
		return fmt.Errorf("check: duplicate `use \"etc\"` base name %q", baseName.Str(c.tm))
	}

	if c.resolveUse == nil {
		return fmt.Errorf("check: cannot resolve a use declaration")
	}
	src, err := c.resolveUse(filename)
	if err != nil {
		return err
	}
	tokens, _, err := t.Tokenize(c.tm, filename, src)
	if err != nil {
		return err
	}
	f, err := parse.Parse(c.tm, filename, tokens, &parse.Options{
		AllowDoubleUnderscoreNames: true,
	})
	if err != nil {
		return err
	}

	for _, n := range f.TopLevelDecls() {
		if err := n.AsRaw().SetPackage(c.tm, baseName); err != nil {
			return err
		}

		switch n.Kind() {
		case a.KConst:
			return fmt.Errorf("TODO: type-check a used-package const")
		case a.KFunc:
			if err := c.checkFuncSignature(n); err != nil {
				return err
			}
		case a.KStatus:
			if err := c.checkStatus(n); err != nil {
				return err
			}
		case a.KStruct:
			if err := c.checkStructDecl(n); err != nil {
				return err
			}
		}
	}
	c.useBaseNames[baseName] = struct{}{}
	setPlaceholderMBoundsMType(node)
	return nil
}

func (c *Checker) checkStatus(node *a.Node) error {
	n := node.AsStatus()
	qid := n.QID()
	if other, ok := c.statuses[qid]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate status %s", qid.Str(c.tm)),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Filename(),
			OtherLine:     other.Line(),
		}
	}
	c.statuses[qid] = n

	q := &checker{
		c:  c,
		tm: c.tm,
	}
	if err := q.tcheckExpr(n.Value(), 0); err != nil {
		return fmt.Errorf("%v in status %s", err, qid.Str(c.tm))
	}
	if cv := n.Value().ConstValue(); cv == nil {
		return fmt.Errorf("status %s value is not a constant expression", qid.Str(c.tm))
	} else if cv.Cmp(zero) <= 0 || oneTwentyEight.Cmp(cv) <= 0 {
		return fmt.Errorf("status %s value %v is out of range [0x01..0x7F]", qid.Str(c.tm), cv)
	} else if n.QID()[0] == 0 {
		x := uint8(cv.Int64())
		if n.Keyword() == t.IDError {
			x = -x
		}
		if other := c.statusByValue[x]; other != 0 {
			return fmt.Errorf("duplicate %s value 0x%02X: %s and %s",
				n.Keyword().Str(c.tm), cv, other.Str(c.tm), n.QID()[1].Str(c.tm))
		}
		c.statusByValue[x] = n.QID()[1]
	}

	setPlaceholderMBoundsMType(n.AsNode())
	return nil
}

func (c *Checker) checkConst(node *a.Node) error {
	n := node.AsConst()
	qid := n.QID()
	if other, ok := c.consts[qid]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate const %s", qid.Str(c.tm)),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Filename(),
			OtherLine:     other.Line(),
		}
	}
	c.consts[qid] = n

	q := &checker{
		c:  c,
		tm: c.tm,
	}
	if err := q.tcheckTypeExpr(n.XType(), 0); err != nil {
		return fmt.Errorf("%v in const %s", err, qid.Str(c.tm))
	}
	if err := q.tcheckExpr(n.Value(), 0); err != nil {
		return fmt.Errorf("%v in const %s", err, qid.Str(c.tm))
	}

	nLists := 0
	typ := n.XType()
	for typ.IsArrayType() {
		if nLists == a.MaxTypeExprDepth {
			return fmt.Errorf("check: type expression recursion depth too large")
		}
		nLists++
		typ = typ.Inner()
	}
	if typ.Decorator() != 0 {
		return fmt.Errorf("check: invalid const type %q for %s", n.XType().Str(c.tm), qid.Str(c.tm))
	}
	nb, err := typeBounds(q.tm, typ)
	if err != nil {
		return err
	}
	if nb[0] == nil || nb[1] == nil {
		return fmt.Errorf("check: invalid const type %q for %s", n.XType().Str(c.tm), qid.Str(c.tm))
	}
	if err := c.checkConstElement(n.Value(), nb, nLists); err != nil {
		return fmt.Errorf("check: %v for %s", err, qid.Str(c.tm))
	}
	setPlaceholderMBoundsMType(n.AsNode())
	return nil
}

func (c *Checker) checkConstElement(n *a.Expr, nb a.Bounds, nLists int) error {
	if nLists > 0 {
		nLists--
		if n.Operator() != t.IDDollar {
			return fmt.Errorf("invalid const value %q", n.Str(c.tm))
		}
		for _, o := range n.Args() {
			if err := c.checkConstElement(o.AsExpr(), nb, nLists); err != nil {
				return err
			}
		}
		return nil
	}
	if cv := n.ConstValue(); cv == nil || cv.Cmp(nb[0]) < 0 || cv.Cmp(nb[1]) > 0 {
		return fmt.Errorf("invalid const value %q not within %v", n.Str(c.tm), nb)
	}
	return nil
}

func (c *Checker) checkStructDecl(node *a.Node) error {
	n := node.AsStruct()
	qid := n.QID()
	if other, ok := c.structs[qid]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate struct %s", qid.Str(c.tm)),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Filename(),
			OtherLine:     other.Line(),
		}
	}
	c.structs[qid] = n
	c.unsortedStructs = append(c.unsortedStructs, n)
	setPlaceholderMBoundsMType(n.AsNode())

	// A struct declaration implies a reset method.
	in := a.NewStruct(0, n.Filename(), n.Line(), t.IDIn, nil)
	out := a.NewStruct(0, n.Filename(), n.Line(), t.IDOut, nil)
	f := a.NewFunc(0, n.Filename(), n.Line(), qid[1], t.IDReset, in, out, nil, nil)
	if qid[0] != 0 {
		f.AsNode().AsRaw().SetPackage(c.tm, qid[0])
	}
	return c.checkFuncSignature(f.AsNode())
}

func (c *Checker) checkStructCycles(_ *a.Node) error {
	if _, ok := a.TopologicalSortStructs(c.unsortedStructs); !ok {
		return fmt.Errorf("check: cyclical struct definitions")
	}
	return nil
}

func (c *Checker) checkStructFields(node *a.Node) error {
	n := node.AsStruct()
	if err := c.checkFields(n.Fields(), true, true); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in struct %s", err, n.QID().Str(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	return nil
}

func (c *Checker) checkFields(fields []*a.Node, banPtrTypes bool, checkDefaultZeroValue bool) error {
	if len(fields) == 0 {
		return nil
	}

	q := &checker{
		c:  c,
		tm: c.tm,
	}
	fieldNames := map[t.ID]bool{}
	for _, n := range fields {
		f := n.AsField()
		if _, ok := fieldNames[f.Name()]; ok {
			return fmt.Errorf("check: duplicate field %q", f.Name().Str(c.tm))
		}
		if err := q.tcheckTypeExpr(f.XType(), 0); err != nil {
			return fmt.Errorf("%v in field %q", err, f.Name().Str(c.tm))
		}
		if banPtrTypes && f.XType().HasPointers() {
			return fmt.Errorf("check: pointer-containing type %q not allowed for field %q",
				f.XType().Str(c.tm), f.Name().Str(c.tm))
		}

		if checkDefaultZeroValue {
			innTyp := f.XType().Innermost()
			fb, err := typeBounds(c.tm, innTyp)
			if err != nil {
				return err
			}
			if (fb[0] != nil && zero.Cmp(fb[0]) < 0) || (fb[1] != nil && zero.Cmp(fb[1]) > 0) {
				return fmt.Errorf("check: default zero value is not within bounds %v for field %q",
					fb, f.Name().Str(c.tm))
			}
		}

		fieldNames[f.Name()] = true
		setPlaceholderMBoundsMType(f.AsNode())
	}

	return nil
}

func (c *Checker) checkFuncSignature(node *a.Node) error {
	n := node.AsFunc()
	if err := c.checkFields(n.In().Fields(), false, false); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in in-params for func %s", err, n.QQID().Str(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	setPlaceholderMBoundsMType(n.In().AsNode())
	if err := c.checkFields(n.Out().Fields(), false, true); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in out-params for func %s", err, n.QQID().Str(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	setPlaceholderMBoundsMType(n.Out().AsNode())
	setPlaceholderMBoundsMType(n.AsNode())

	// TODO: check somewhere that, if n.Out() is non-empty (or we are
	// suspendible), that we end with a return statement? Or is that an
	// implicit "return out"?

	qqid := n.QQID()
	if other, ok := c.funcs[qqid]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate function %s", qqid.Str(c.tm)),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Filename(),
			OtherLine:     other.Line(),
		}
	}
	c.funcs[qqid] = n

	if qqid[0] != 0 {
		// No need to populate c.localVars for built-in or used-package funcs.
		// In any case, the remaining type checking code in this function
		// doesn't handle the base.† dagger type.
		return nil
	}

	iQID := n.In().QID()
	inTyp := a.NewTypeExpr(0, iQID[0], iQID[1], nil, nil, nil)
	setPlaceholderMBoundsMType(inTyp.AsNode())
	oQID := n.Out().QID()
	outTyp := a.NewTypeExpr(0, oQID[0], oQID[1], nil, nil, nil)
	setPlaceholderMBoundsMType(outTyp.AsNode())
	localVars := typeMap{
		t.IDIn:  inTyp,
		t.IDOut: outTyp,
	}
	if qqid[1] != 0 {
		if _, ok := c.structs[t.QID{qqid[0], qqid[1]}]; !ok {
			return &Error{
				Err:      fmt.Errorf("check: no receiver struct defined for function %s", qqid.Str(c.tm)),
				Filename: n.Filename(),
				Line:     n.Line(),
			}
		}
		sTyp := a.NewTypeExpr(0, qqid[0], qqid[1], nil, nil, nil)
		setPlaceholderMBoundsMType(sTyp.AsNode())
		pTyp := a.NewTypeExpr(t.IDPtr, 0, 0, nil, nil, sTyp)
		setPlaceholderMBoundsMType(pTyp.AsNode())
		localVars[t.IDThis] = pTyp
	}
	c.localVars[qqid] = localVars
	return nil
}

func (c *Checker) checkFuncContract(node *a.Node) error {
	n := node.AsFunc()
	if len(n.Asserts()) == 0 {
		return nil
	}
	q := &checker{
		c:  c,
		tm: c.tm,
	}
	for _, o := range n.Asserts() {
		if err := q.tcheckAssert(o.AsAssert()); err != nil {
			return err
		}
	}
	return nil
}

func (c *Checker) checkFuncBody(node *a.Node) error {
	n := node.AsFunc()
	if len(n.Body()) == 0 {
		return nil
	}

	q := &checker{
		c:         c,
		tm:        c.tm,
		reasonMap: c.reasonMap,
		astFunc:   c.funcs[n.QQID()],
		localVars: c.localVars[n.QQID()],
	}

	// Fill in the TypeMap with all local variables. Note that they have
	// function scope and can be hoisted, JavaScript style, a la
	// https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/var
	if err := q.tcheckVars(n.Body()); err != nil {
		return &Error{
			Err:      err,
			Filename: q.errFilename,
			Line:     q.errLine,
		}
	}

	// TODO: check that variables are never used before they're initialized.

	for _, o := range n.Body() {
		if err := q.tcheckStatement(o); err != nil {
			return &Error{
				Err:      err,
				Filename: q.errFilename,
				Line:     q.errLine,
			}
		}
	}

	if err := q.bcheckBlock(n.Body()); err != nil {
		return &Error{
			Err:      err,
			Filename: q.errFilename,
			Line:     q.errLine,
			TMap:     c.tm,
			Facts:    q.facts,
		}
	}

	return nil
}

func (c *Checker) checkFieldMethodCollisions(node *a.Node) error {
	n := node.AsStruct()
	for _, o := range n.Fields() {
		nQID := n.QID()
		qqid := t.QQID{nQID[0], nQID[1], o.AsField().Name()}
		if _, ok := c.funcs[qqid]; ok {
			return fmt.Errorf("check: struct %q has both a field and method named %q",
				nQID.Str(c.tm), qqid[2].Str(c.tm))
		}
	}
	return nil
}

func (c *Checker) checkAllTypeChecked(node *a.Node) error {
	for _, v := range c.consts {
		if err := allTypeChecked(c.tm, v.AsNode()); err != nil {
			return err
		}
	}
	for _, v := range c.funcs {
		if err := allTypeChecked(c.tm, v.AsNode()); err != nil {
			return err
		}
	}
	for _, v := range c.statuses {
		if err := allTypeChecked(c.tm, v.AsNode()); err != nil {
			return err
		}
	}
	for _, v := range c.structs {
		if err := allTypeChecked(c.tm, v.AsNode()); err != nil {
			return err
		}
	}
	return nil
}

func allTypeChecked(tm *t.Map, n *a.Node) error {
	return n.Walk(func(o *a.Node) error {
		typ := o.MType()
		if typ == nil {
			switch o.Kind() {
			case a.KConst:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.AsConst().QID().Str(tm))
			case a.KExpr:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.AsExpr().Str(tm))
			case a.KFunc:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.AsFunc().QQID().Str(tm))
			case a.KTypeExpr:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.AsTypeExpr().Str(tm))
			case a.KStatus:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.AsStatus().QID().Str(tm))
			case a.KStruct:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.AsStruct().QID().Str(tm))
			}
			return fmt.Errorf("check: internal error: unchecked %s node", o.Kind())
		}
		if o.Kind() == a.KExpr {
			o := o.AsExpr()
			if typ.IsIdeal() && o.ConstValue() == nil {
				return fmt.Errorf("check: internal error: expression %q has ideal number type "+
					"but no const value", o.Str(tm))
			}
		}
		return nil
	})
}

type checker struct {
	c         *Checker
	tm        *t.Map
	reasonMap reasonMap
	astFunc   *a.Func
	localVars typeMap

	errFilename string
	errLine     uint32

	jumpTargets []a.Loop

	facts facts
}
