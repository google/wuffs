// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"errors"
	"fmt"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

type Flags uint32

const (
	FlagsOnlyTypeCheck = Flags(0x0001)
)

type Error struct {
	Err           error
	Filename      string
	Line          uint32
	OtherFilename string
	OtherLine     uint32

	IDMap *t.IDMap
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
	if e.IDMap == nil {
		return s
	}
	b := append([]byte(s), ". Facts:\n"...)
	for _, f := range e.Facts {
		b = append(b, '\t')
		b = append(b, f.String(e.IDMap)...)
		b = append(b, '\n')
	}
	return string(b)
}

// TypeExprFoo is an *ast.Node MType (implicit type) that means that n is a
// boolean or a ideal numeric constant.
var (
	TypeExprBoolean     = a.NewTypeExpr(0, t.IDBool, nil, nil, nil)
	TypeExprIdealNumber = a.NewTypeExpr(0, t.IDIdeal, nil, nil, nil)
)

func numeric(n *a.TypeExpr) bool { return n == TypeExprIdealNumber || n.IsNumType() }

// TypeMap maps from variable names (as token IDs) to types.
type TypeMap map[t.ID]*a.TypeExpr

type Struct struct {
	ID     t.ID
	Struct *a.Struct
}

type Func struct {
	QID       t.QID
	Func      *a.Func
	LocalVars TypeMap
}

func Check(m *t.IDMap, flags Flags, files ...*a.File) (*Checker, error) {
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
		if id := m.ByName(r.s); id != 0 {
			rMap[id.Key()] = r.r
		}
	}
	c := &Checker{
		flags:     flags,
		idMap:     m,
		reasonMap: rMap,
		structs:   map[t.ID]Struct{},
		funcs:     map[t.QID]Func{},
	}

	for _, phase := range phases {
		for _, f := range files {
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
	{a.KUse, (*Checker).checkUse},
	{a.KStruct, (*Checker).checkStruct},
	{a.KFunc, (*Checker).checkFuncSignature},
	{a.KFunc, (*Checker).checkFuncContract},
	{a.KFunc, (*Checker).checkFuncBody},
}

type Checker struct {
	flags     Flags
	idMap     *t.IDMap
	reasonMap reasonMap
	structs   map[t.ID]Struct
	funcs     map[t.QID]Func
}

func (c *Checker) Funcs() map[t.QID]Func { return c.funcs }

func (c *Checker) checkUse(node *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkFields(fields []*a.Node, banPtrTypes bool) error {
	if len(fields) == 0 {
		return nil
	}

	q := &checker{
		c:     c,
		idMap: c.idMap,
	}
	fieldNames := map[t.ID]bool{}
	for _, n := range fields {
		f := n.Field()
		if _, ok := fieldNames[f.Name()]; ok {
			return fmt.Errorf("check: duplicate field %q", f.Name().String(c.idMap))
		}
		if err := q.tcheckTypeExpr(f.XType(), 0); err != nil {
			return fmt.Errorf("%v in field %q", err, f.Name().String(c.idMap))
		}
		if banPtrTypes {
			for x := f.XType(); x.Inner() != nil; x = x.Inner() {
				if x.PackageOrDecorator().Key() == t.KeyPtr {
					// TODO: implement nptr (nullable pointer) types.
					return fmt.Errorf("check: ptr type %q not allowed for field %q; use nptr instead",
						x.String(c.idMap), f.Name().String(c.idMap))
				}
			}
		}
		if dv := f.DefaultValue(); dv != nil {
			if f.XType().PackageOrDecorator() != 0 {
				return fmt.Errorf("check: cannot set default value for qualified type %q for field %q",
					f.XType().String(c.idMap), f.Name().String(c.idMap))
			}
			if err := q.tcheckExpr(dv, 0); err != nil {
				return err
			}
		}
		if c.flags&FlagsOnlyTypeCheck == 0 {
			if err := bcheckField(c.idMap, f); err != nil {
				return err
			}
		}
		fieldNames[f.Name()] = true
		f.Node().SetTypeChecked()
	}

	return nil
}

func (c *Checker) checkStruct(node *a.Node) error {
	n := node.Struct()
	if err := c.checkFields(n.Fields(), true); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in struct %q", err, n.Name().String(c.idMap)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	id := n.Name()
	if other, ok := c.structs[id]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate struct %q", id.String(c.idMap)),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Struct.Filename(),
			OtherLine:     other.Struct.Line(),
		}
	}
	c.structs[id] = Struct{
		ID:     id,
		Struct: n,
	}
	n.Node().SetTypeChecked()
	return nil
}

func (c *Checker) checkFuncSignature(node *a.Node) error {
	n := node.Func()
	if err := c.checkFields(n.In().Fields(), false); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in in-params for func %q", err, n.Name().String(c.idMap)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	n.In().Node().SetTypeChecked()
	if err := c.checkFields(n.Out().Fields(), false); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in out-params for func %q", err, n.Name().String(c.idMap)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	n.Out().Node().SetTypeChecked()

	qid := n.QID()
	if other, ok := c.funcs[qid]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate function %q", qid.String(c.idMap)),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Func.Filename(),
			OtherLine:     other.Func.Line(),
		}
	}

	inTyp := a.NewTypeExpr(0, n.In().Name(), nil, nil, nil)
	inTyp.Node().SetTypeChecked()
	outTyp := a.NewTypeExpr(0, n.Out().Name(), nil, nil, nil)
	outTyp.Node().SetTypeChecked()
	localVars := TypeMap{
		t.IDIn:  inTyp,
		t.IDOut: outTyp,
	}
	if qid[0] != 0 {
		if _, ok := c.structs[qid[0]]; !ok {
			return &Error{
				Err:      fmt.Errorf("check: no receiver struct defined for function %q", qid.String(c.idMap)),
				Filename: n.Filename(),
				Line:     n.Line(),
			}
		}
		sTyp := a.NewTypeExpr(0, qid[0], nil, nil, nil)
		sTyp.Node().SetTypeChecked()
		pTyp := a.NewTypeExpr(t.IDPtr, 0, nil, nil, sTyp)
		pTyp.Node().SetTypeChecked()
		localVars[t.IDThis] = pTyp
	}
	c.funcs[qid] = Func{
		QID:       qid,
		Func:      n,
		LocalVars: localVars,
	}
	return nil
}

func (c *Checker) checkFuncContract(node *a.Node) error {
	n := node.Func()
	if len(n.Asserts()) == 0 {
		return nil
	}
	q := &checker{
		c:     c,
		idMap: c.idMap,
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
		idMap:     c.idMap,
		reasonMap: c.reasonMap,
		f:         c.funcs[n.QID()],
	}

	// Fill in the TypeMap with all local variables. Note that they have
	// function scope and can be hoisted, JavaScript style, a la
	// https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/var
	for _, o := range n.Body() {
		if err := q.tcheckVars(o); err != nil {
			return &Error{
				Err:      err,
				Filename: q.errFilename,
				Line:     q.errLine,
			}
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

	// Run bounds checks.
	if c.flags&FlagsOnlyTypeCheck == 0 {
		for _, o := range n.Body() {
			if err := q.bcheckStatement(o); err != nil {
				return &Error{
					Err:      err,
					Filename: q.errFilename,
					Line:     q.errLine,
					IDMap:    c.idMap,
					Facts:    q.facts,
				}
			}
		}
	}

	n.Node().SetTypeChecked()
	if err := n.Node().Walk(func(o *a.Node) error {
		if !o.TypeChecked() {
			return fmt.Errorf("check: internal error: unchecked %s node", o.Kind())
		}
		if o.Kind() == a.KExpr {
			o := o.Expr()
			if typ := o.MType(); typ == nil {
				return fmt.Errorf("check: internal error: expression %q has no (implicit) type",
					o.String(q.idMap))
			} else if typ == TypeExprIdealNumber && o.ConstValue() == nil {
				return fmt.Errorf("check: internal error: expression %q has ideal number type "+
					"but no const value", o.String(q.idMap))
			}
		}
		return nil
	}); err != nil {
		return err
	}

	return nil
}

type checker struct {
	c         *Checker
	idMap     *t.IDMap
	reasonMap reasonMap
	f         Func

	errFilename string
	errLine     uint32

	facts facts
}
