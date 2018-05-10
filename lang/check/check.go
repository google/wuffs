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
	"math/big"
	"path"

	"github.com/google/wuffs/lang/base38"
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
			f.Node().SetTypeChecked()
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
	{a.KStruct, (*Checker).checkImplicitResetMethod},
	{a.KInvalid, (*Checker).checkStructCycles},
	{a.KStruct, (*Checker).checkStructFields},
	{a.KFunc, (*Checker).checkFuncSignature},
	{a.KFunc, (*Checker).checkFuncContract},
	{a.KFunc, (*Checker).checkFuncBody},
	{a.KStruct, (*Checker).checkFieldMethodCollisions},
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

	builtInFuncs      map[t.QQID]*a.Func
	builtInSliceFuncs map[t.QQID]*a.Func
	unsortedStructs   []*a.Struct
}

func (c *Checker) PackageID() uint32 { return c.packageID }

func (c *Checker) checkPackageID(node *a.Node) error {
	n := node.PackageID()
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
	return nil
}

func (c *Checker) checkPackageIDExists(node *a.Node) error {
	if c.otherPackageID == nil {
		return fmt.Errorf("check: missing packageid declaration")
	}
	return nil
}

func (c *Checker) checkUse(node *a.Node) error {
	usePath := node.Use().Path()
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
		if err := n.Raw().SetPackage(c.tm, baseName); err != nil {
			return err
		}

		duplicate := ""
		switch n.Kind() {
		case a.KConst:
			n := n.Const()
			qid := n.QID()
			if _, ok := c.consts[qid]; ok {
				duplicate = qid.Str(c.tm)
			} else {
				c.consts[qid] = n
			}
		case a.KFunc:
			n := n.Func()
			qqid := n.QQID()
			if _, ok := c.funcs[qqid]; ok {
				duplicate = qqid.Str(c.tm)
			} else {
				c.funcs[qqid] = n
			}
		case a.KStatus:
			n := n.Status()
			qid := n.QID()
			if _, ok := c.statuses[qid]; ok {
				duplicate = qid.Str(c.tm)
			} else {
				c.statuses[qid] = n
			}
		case a.KStruct:
			n := n.Struct()
			qid := n.QID()
			if _, ok := c.structs[qid]; ok {
				duplicate = qid.Str(c.tm)
			} else {
				c.structs[qid] = n
			}
		}

		if duplicate != "" {
			return fmt.Errorf("check: duplicate name %s", duplicate)
		}
	}
	c.useBaseNames[baseName] = struct{}{}
	return nil
}

func (c *Checker) checkStatus(node *a.Node) error {
	n := node.Status()
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
	n.Node().SetTypeChecked()
	return nil
}

func (c *Checker) checkConst(node *a.Node) error {
	n := node.Const()
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
	nMin, nMax, err := q.bcheckTypeExpr(typ)
	if err != nil {
		return err
	}
	if nMin == nil || nMax == nil {
		return fmt.Errorf("check: invalid const type %q for %s", n.XType().Str(c.tm), qid.Str(c.tm))
	}
	if err := c.checkConstElement(n.Value(), nMin, nMax, nLists); err != nil {
		return fmt.Errorf("check: %v for %s", err, qid.Str(c.tm))
	}
	n.Node().SetTypeChecked()
	return nil
}

func (c *Checker) checkConstElement(n *a.Expr, nMin *big.Int, nMax *big.Int, nLists int) error {
	if nLists > 0 {
		nLists--
		if n.Operator() != t.IDDollar {
			return fmt.Errorf("invalid const value %q", n.Str(c.tm))
		}
		for _, o := range n.Args() {
			if err := c.checkConstElement(o.Expr(), nMin, nMax, nLists); err != nil {
				return err
			}
		}
		return nil
	}
	if cv := n.ConstValue(); cv == nil || cv.Cmp(nMin) < 0 || cv.Cmp(nMax) > 0 {
		return fmt.Errorf("invalid const value %q not within [%v..%v]", n.Str(c.tm), nMin, nMax)
	}
	return nil
}

func (c *Checker) checkStructDecl(node *a.Node) error {
	n := node.Struct()
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
	return nil
}

func (c *Checker) checkImplicitResetMethod(node *a.Node) error {
	n := node.Struct()
	in := a.NewStruct(0, n.Filename(), n.Line(), t.IDIn, nil)
	out := a.NewStruct(0, n.Filename(), n.Line(), t.IDOut, nil)
	f := a.NewFunc(0, n.Filename(), n.Line(), n.QID()[1], t.IDReset, in, out, nil, nil)
	return c.checkFuncSignature(f.Node())
}

func (c *Checker) checkStructCycles(_ *a.Node) error {
	if _, ok := a.TopologicalSortStructs(c.unsortedStructs); !ok {
		return fmt.Errorf("check: cyclical struct definitions")
	}
	return nil
}

func (c *Checker) checkStructFields(node *a.Node) error {
	n := node.Struct()
	if err := c.checkFields(n.Fields(), true); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in struct %s", err, n.QID().Str(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	n.Node().SetTypeChecked()
	return nil
}

func (c *Checker) checkFields(fields []*a.Node, banPtrTypes bool) error {
	if len(fields) == 0 {
		return nil
	}

	q := &checker{
		c:  c,
		tm: c.tm,
	}
	fieldNames := map[t.ID]bool{}
	for _, n := range fields {
		f := n.Field()
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
		if dv := f.DefaultValue(); dv != nil {
			if f.XType().Decorator() != 0 {
				return fmt.Errorf("check: cannot set default value for type %q for field %q",
					f.XType().Str(c.tm), f.Name().Str(c.tm))
			}
			if err := q.tcheckExpr(dv, 0); err != nil {
				return err
			}
		}
		if err := bcheckField(c.tm, f); err != nil {
			return err
		}
		fieldNames[f.Name()] = true
		f.Node().SetTypeChecked()
	}

	return nil
}

func (c *Checker) checkFuncSignature(node *a.Node) error {
	n := node.Func()
	if err := c.checkFields(n.In().Fields(), false); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in in-params for func %s", err, n.QQID().Str(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	n.In().Node().SetTypeChecked()
	if err := c.checkFields(n.Out().Fields(), false); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in out-params for func %s", err, n.QQID().Str(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	n.Out().Node().SetTypeChecked()

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

	iQID := n.In().QID()
	inTyp := a.NewTypeExpr(0, iQID[0], iQID[1], nil, nil, nil)
	inTyp.Node().SetTypeChecked()
	oQID := n.Out().QID()
	outTyp := a.NewTypeExpr(0, oQID[0], oQID[1], nil, nil, nil)
	outTyp.Node().SetTypeChecked()
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
		sTyp.Node().SetTypeChecked()
		pTyp := a.NewTypeExpr(t.IDPtr, 0, 0, nil, nil, sTyp)
		pTyp.Node().SetTypeChecked()
		localVars[t.IDThis] = pTyp
	}
	c.funcs[qqid] = n
	c.localVars[qqid] = localVars
	return nil
}

func (c *Checker) checkFuncContract(node *a.Node) error {
	n := node.Func()
	if len(n.Asserts()) == 0 {
		return nil
	}
	q := &checker{
		c:  c,
		tm: c.tm,
	}
	for _, o := range n.Asserts() {
		if err := q.tcheckAssert(o.Assert()); err != nil {
			return err
		}
	}
	return nil
}

func (c *Checker) checkFuncBody(node *a.Node) error {
	n := node.Func()
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

	// Assign ConstValue's (if applicable) and MType's to each Expr.
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

	n.Node().SetTypeChecked()
	if err := n.Node().Walk(func(o *a.Node) error {
		if !o.TypeChecked() {
			switch o.Kind() {
			case a.KExpr:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.Expr().Str(q.tm))
			case a.KTypeExpr:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.TypeExpr().Str(q.tm))
			}
			return fmt.Errorf("check: internal error: unchecked %s node", o.Kind())
		}
		if o.Kind() == a.KExpr {
			o := o.Expr()
			if typ := o.MType(); typ == nil {
				return fmt.Errorf("check: internal error: expression %q has no (implicit) type",
					o.Str(q.tm))
			} else if typ.IsIdeal() && o.ConstValue() == nil {
				return fmt.Errorf("check: internal error: expression %q has ideal number type "+
					"but no const value", o.Str(q.tm))
			}
		}
		return nil
	}); err != nil {
		return err
	}

	return nil
}

func (c *Checker) checkFieldMethodCollisions(node *a.Node) error {
	n := node.Struct()
	for _, o := range n.Fields() {
		nQID := n.QID()
		qqid := t.QQID{nQID[0], nQID[1], o.Field().Name()}
		if _, ok := c.funcs[qqid]; ok {
			return fmt.Errorf("check: struct %q has both a field and method named %q",
				nQID.Str(c.tm), qqid[2].Str(c.tm))
		}
	}
	return nil
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
