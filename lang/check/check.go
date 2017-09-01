// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

//go:generate go run gen.go

package check

import (
	"errors"
	"fmt"
	"math/big"

	"github.com/google/puffs/lang/base38"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
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
		b = append(b, f.String(e.TMap)...)
		b = append(b, '\n')
	}
	return string(b)
}

func trimQuotes(s string) string {
	if len(s) >= 2 && s[0] == '"' && s[len(s)-1] == '"' {
		return s[1 : len(s)-1]
	}
	return s
}

// typeExprFoo is an *ast.Node MType (implicit type).
var (
	typeExprBool   = a.NewTypeExpr(0, t.IDBool, nil, nil, nil)
	typeExprIdeal  = a.NewTypeExpr(0, t.IDDoubleZ, nil, nil, nil)
	typeExprList   = a.NewTypeExpr(0, t.IDDollar, nil, nil, nil)
	typeExprStatus = a.NewTypeExpr(0, t.IDStatus, nil, nil, nil)
	typeExprU64    = a.NewTypeExpr(0, t.IDU64, nil, nil, nil)

	// TODO: delete this.
	typeExprPlaceholder   = a.NewTypeExpr(0, t.IDU8, nil, nil, nil)
	typeExprPlaceholder16 = a.NewTypeExpr(0, t.IDU16, nil, nil, nil)
	typeExprPlaceholder32 = a.NewTypeExpr(0, t.IDU32, nil, nil, nil)
)

// TypeMap maps from variable names (as token IDs) to types.
type TypeMap map[t.ID]*a.TypeExpr

type Const struct {
	ID    t.ID // ID of the const name.
	Const *a.Const
}

type Func struct {
	QID       t.QID // Qualified ID of the func name.
	Func      *a.Func
	LocalVars TypeMap
}

type Status struct {
	ID     t.ID // ID of the status message.
	Status *a.Status
}

type Struct struct {
	ID     t.ID // ID of the struct name.
	Struct *a.Struct
}

func Check(tm *t.Map, files ...*a.File) (*Checker, error) {
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
			rMap[id.Key()] = r.r
		}
	}
	c := &Checker{
		tm:        tm,
		reasonMap: rMap,
		packageID: base38.Max + 1,
		consts:    map[t.ID]Const{},
		funcs:     map[t.QID]Func{},
		statuses:  map[t.ID]Status{},
		structs:   map[t.ID]Struct{},
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
	// TODO: check consts, funcs, structs and uses for name collisions.
}

type reason func(q *checker, n *a.Assert) error

type reasonMap map[t.Key]reason

type Checker struct {
	tm        *t.Map
	reasonMap reasonMap

	packageID      uint32
	otherPackageID *a.PackageID

	consts   map[t.ID]Const
	funcs    map[t.QID]Func
	statuses map[t.ID]Status
	structs  map[t.ID]Struct

	unsortedStructs []*a.Struct
}

func (c *Checker) PackageID() uint32         { return c.packageID }
func (c *Checker) Consts() map[t.ID]Const    { return c.consts }
func (c *Checker) Funcs() map[t.QID]Func     { return c.funcs }
func (c *Checker) Statuses() map[t.ID]Status { return c.statuses }
func (c *Checker) Structs() map[t.ID]Struct  { return c.structs }

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
	raw := n.ID().String(c.tm)
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

func (c *Checker) checkUse(node *a.Node) error {
	// TODO.
	return nil
}

func (c *Checker) checkStatus(node *a.Node) error {
	n := node.Status()
	id := n.Message()
	if other, ok := c.statuses[id]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate status %q", trimQuotes(id.String(c.tm))),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Status.Filename(),
			OtherLine:     other.Status.Line(),
		}
	}
	c.statuses[id] = Status{
		ID:     id,
		Status: n,
	}
	n.Node().SetTypeChecked()
	return nil
}

func (c *Checker) checkConst(node *a.Node) error {
	n := node.Const()
	id := n.Name()
	if other, ok := c.consts[id]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate const %q", id.String(c.tm)),
			Filename:      n.Filename(),
			Line:          n.Line(),
			OtherFilename: other.Const.Filename(),
			OtherLine:     other.Const.Line(),
		}
	}
	c.consts[id] = Const{
		ID:    id,
		Const: n,
	}

	q := &checker{
		c:  c,
		tm: c.tm,
	}
	if err := q.tcheckTypeExpr(n.XType(), 0); err != nil {
		return fmt.Errorf("%v in const %q", err, id.String(c.tm))
	}
	if err := q.tcheckExpr(n.Value(), 0); err != nil {
		return fmt.Errorf("%v in const %q", err, id.String(c.tm))
	}

	nLists := 0
	typ := n.XType()
	for typ.Decorator().Key() == t.KeyOpenBracket {
		if nLists == a.MaxTypeExprDepth {
			return fmt.Errorf("check: type expression recursion depth too large")
		}
		nLists++
		typ = typ.Inner()
	}
	if typ.Decorator() != 0 {
		return fmt.Errorf("check: invalid const type %q for %q", n.XType().String(c.tm), id.String(c.tm))
	}
	nMin, nMax, err := q.bcheckTypeExpr(typ)
	if err != nil {
		return err
	}
	if nMin == nil || nMax == nil {
		return fmt.Errorf("check: invalid const type %q for %q", n.XType().String(c.tm), id.String(c.tm))
	}
	if err := c.checkConstElement(n.Value(), nMin, nMax, nLists); err != nil {
		return fmt.Errorf("check: %v for %q", err, id.String(c.tm))
	}
	n.Node().SetTypeChecked()
	return nil
}

func (c *Checker) checkConstElement(n *a.Expr, nMin *big.Int, nMax *big.Int, nLists int) error {
	if nLists > 0 {
		nLists--
		if n.ID0().Key() != t.KeyDollar {
			return fmt.Errorf("invalid const value %q", n.String(c.tm))
		}
		for _, o := range n.Args() {
			if err := c.checkConstElement(o.Expr(), nMin, nMax, nLists); err != nil {
				return err
			}
		}
		return nil
	}
	if cv := n.ConstValue(); cv == nil || cv.Cmp(nMin) < 0 || cv.Cmp(nMax) > 0 {
		return fmt.Errorf("invalid const value %q not within [%v..%v]", n.String(c.tm), nMin, nMax)
	}
	return nil
}

func (c *Checker) checkStructDecl(node *a.Node) error {
	n := node.Struct()
	id := n.Name()
	if other, ok := c.structs[id]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate struct %q", id.String(c.tm)),
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
	c.unsortedStructs = append(c.unsortedStructs, n)
	return nil
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
			Err:      fmt.Errorf("%v in struct %q", err, n.Name().String(c.tm)),
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
			return fmt.Errorf("check: duplicate field %q", f.Name().String(c.tm))
		}
		if err := q.tcheckTypeExpr(f.XType(), 0); err != nil {
			return fmt.Errorf("%v in field %q", err, f.Name().String(c.tm))
		}
		if banPtrTypes {
			for x := f.XType(); x.Inner() != nil; x = x.Inner() {
				if x.Decorator().Key() == t.KeyPtr {
					// TODO: implement nptr (nullable pointer) types.
					return fmt.Errorf("check: ptr type %q not allowed for field %q; use nptr instead",
						x.String(c.tm), f.Name().String(c.tm))
				}
			}
		}
		if dv := f.DefaultValue(); dv != nil {
			if f.XType().Decorator() != 0 {
				return fmt.Errorf("check: cannot set default value for qualified type %q for field %q",
					f.XType().String(c.tm), f.Name().String(c.tm))
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
			Err:      fmt.Errorf("%v in in-params for func %q", err, n.Name().String(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	n.In().Node().SetTypeChecked()
	if err := c.checkFields(n.Out().Fields(), false); err != nil {
		return &Error{
			Err:      fmt.Errorf("%v in out-params for func %q", err, n.Name().String(c.tm)),
			Filename: n.Filename(),
			Line:     n.Line(),
		}
	}
	n.Out().Node().SetTypeChecked()

	qid := n.QID()
	if other, ok := c.funcs[qid]; ok {
		return &Error{
			Err:           fmt.Errorf("check: duplicate function %q", qid.String(c.tm)),
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
				Err:      fmt.Errorf("check: no receiver struct defined for function %q", qid.String(c.tm)),
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
		f:         c.funcs[n.QID()],
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
					o.Kind(), o.Expr().String(q.tm))
			case a.KTypeExpr:
				return fmt.Errorf("check: internal error: unchecked %s node %q",
					o.Kind(), o.TypeExpr().String(q.tm))
			}
			return fmt.Errorf("check: internal error: unchecked %s node", o.Kind())
		}
		if o.Kind() == a.KExpr {
			o := o.Expr()
			if typ := o.MType(); typ == nil {
				return fmt.Errorf("check: internal error: expression %q has no (implicit) type",
					o.String(q.tm))
			} else if typ.IsIdeal() && o.ConstValue() == nil {
				return fmt.Errorf("check: internal error: expression %q has ideal number type "+
					"but no const value", o.String(q.tm))
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
		qid := t.QID{n.Name(), o.Field().Name()}
		if _, ok := c.funcs[qid]; ok {
			return fmt.Errorf("check: struct %q has both a field and method named %q",
				qid[0].String(c.tm), qid[1].String(c.tm))
		}
	}
	return nil
}

type checker struct {
	c         *Checker
	tm        *t.Map
	reasonMap reasonMap
	f         Func

	errFilename string
	errLine     uint32

	jumpTargets []*a.While

	facts facts
}
