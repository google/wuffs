// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"errors"
	"fmt"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
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

func Check(m *t.IDMap, files ...*a.File) (*Checker, error) {
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

	c := &Checker{
		idMap:   m,
		structs: map[t.ID]Struct{},
		funcs:   map[t.QID]Func{},
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
	idMap   *t.IDMap
	structs map[t.ID]Struct
	funcs   map[t.QID]Func
}

func (c *Checker) Funcs() map[t.QID]Func { return c.funcs }

func (c *Checker) checkUse(n *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkFields(fields []*a.Node) error {
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
			return fmt.Errorf("check: duplicate field %q", c.idMap.ByID(f.Name()))
		}
		if err := q.tcheckTypeExpr(f.XType(), 0); err != nil {
			return fmt.Errorf("%v in field %q", err, c.idMap.ByID(f.Name()))
		}
		fieldNames[f.Name()] = true
		f.Node().SetTypeChecked()
	}

	// TODO: bounds checking.

	return nil
}

func (c *Checker) checkStruct(n *a.Node) error {
	s := n.Struct()
	if err := c.checkFields(s.Fields()); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in struct %q", err, c.idMap.ByID(s.Name())),
			Filename: s.Filename(),
			Line:     s.Line(),
		}
	}
	id := s.Name()
	if other, ok := c.structs[id]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate struct %q", c.idMap.ByID(id)),
			Filename:      s.Filename(),
			Line:          s.Line(),
			OtherFilename: other.Struct.Filename(),
			OtherLine:     other.Struct.Line(),
		}
	}
	c.structs[id] = Struct{
		ID:     id,
		Struct: s,
	}
	s.Node().SetTypeChecked()
	return nil
}

func (c *Checker) checkFuncSignature(n *a.Node) error {
	f := n.Func()
	if err := c.checkFields(f.In().Fields()); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in in-params for func %q", err, c.idMap.ByID(f.Name())),
			Filename: f.Filename(),
			Line:     f.Line(),
		}
	}
	f.In().Node().SetTypeChecked()
	if err := c.checkFields(f.Out().Fields()); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in out-params for func %q", err, c.idMap.ByID(f.Name())),
			Filename: f.Filename(),
			Line:     f.Line(),
		}
	}
	f.Out().Node().SetTypeChecked()

	qid := f.QID()
	if other, ok := c.funcs[qid]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate function %q", qid.String(c.idMap)),
			Filename:      f.Filename(),
			Line:          f.Line(),
			OtherFilename: other.Func.Filename(),
			OtherLine:     other.Func.Line(),
		}
	}

	inTyp := a.NewTypeExpr(0, f.In().Name(), nil, nil, nil)
	inTyp.Node().SetTypeChecked()
	outTyp := a.NewTypeExpr(0, f.Out().Name(), nil, nil, nil)
	outTyp.Node().SetTypeChecked()
	localVars := TypeMap{
		t.IDIn:  inTyp,
		t.IDOut: outTyp,
	}
	if qid[0] != 0 {
		if _, ok := c.structs[qid[0]]; !ok {
			return &Error{
				Err:      fmt.Errorf("check: no receiver struct defined for function %q", qid.String(c.idMap)),
				Filename: f.Filename(),
				Line:     f.Line(),
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
		Func:      f,
		LocalVars: localVars,
	}
	return nil
}

func (c *Checker) checkFuncContract(n *a.Node) error {
	as := n.Func().Asserts()
	if len(as) == 0 {
		return nil
	}
	q := &checker{
		c:     c,
		idMap: c.idMap,
	}
	for _, a := range as {
		if err := q.tcheckAssert(a.Assert()); err != nil {
			return err
		}
	}
	return nil
}

func (c *Checker) checkFuncBody(n *a.Node) error {
	f := n.Func()
	q := &checker{
		c:     c,
		idMap: c.idMap,
		f:     c.funcs[f.QID()],
	}

	// Fill in the TypeMap with all local variables. Note that they have
	// function scope and can be hoisted, JavaScript style, a la
	// https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/var
	for _, m := range f.Body() {
		if err := q.tcheckVars(m); err != nil {
			return &Error{
				Err:      err,
				Filename: q.errFilename,
				Line:     q.errLine,
			}
		}
	}

	// TODO: check that variables are never used before they're initialized.

	// Assign ConstValue's (if applicable) and MType's to each Expr.
	for _, m := range f.Body() {
		if err := q.tcheckStatement(m); err != nil {
			return &Error{
				Err:      err,
				Filename: q.errFilename,
				Line:     q.errLine,
			}
		}
	}

	// Run bounds checks.
	for _, m := range f.Body() {
		if err := q.bcheckStatement(m); err != nil {
			return &Error{
				Err:      err,
				Filename: q.errFilename,
				Line:     q.errLine,
				IDMap:    c.idMap,
				Facts:    q.facts,
			}
		}
	}

	f.Node().SetTypeChecked()
	if err := f.Node().Walk(func(n *a.Node) error {
		if !n.TypeChecked() {
			return fmt.Errorf("check: internal error: unchecked %s node", n.Kind())
		}
		if n.Kind() == a.KExpr {
			e := n.Expr()
			if typ := e.MType(); typ == nil {
				return fmt.Errorf("check: internal error: expression %q has no (implicit) type",
					e.String(q.idMap))
			} else if typ == TypeExprIdealNumber && e.ConstValue() == nil {
				return fmt.Errorf("check: internal error: expression %q has ideal number type "+
					"but no const value", e.String(q.idMap))
			}
		}
		return nil
	}); err != nil {
		return err
	}

	return nil
}

type checker struct {
	c     *Checker
	idMap *t.IDMap
	f     Func

	errFilename string
	errLine     uint32

	facts []*a.Expr
}
