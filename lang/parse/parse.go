// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package parse

// TODO: write a formal grammar for the language.

import (
	"fmt"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

func Parse(m *t.IDMap, filename string, src []t.Token) (*a.Node, error) {
	p := &parser{
		src:      src,
		m:        m,
		filename: filename,
	}
	if len(src) > 0 {
		p.lastLine = src[len(src)-1].Line
	}
	return p.parseFile()
}

type parser struct {
	src      []t.Token
	m        *t.IDMap
	filename string
	lastLine uint32
}

func (p *parser) line() uint32 {
	if len(p.src) != 0 {
		return p.src[0].Line
	}
	return p.lastLine
}

func (p *parser) peekID() t.ID {
	if len(p.src) > 0 {
		return p.src[0].ID
	}
	return 0
}

func (p *parser) parseFile() (*a.Node, error) {
	decls := []*a.Node(nil)
	for len(p.src) > 0 {
		d, err := p.parseTopLevelDecl()
		if err != nil {
			return nil, err
		}
		decls = append(decls, d)
	}
	return &a.Node{
		Kind:     a.KFile,
		Filename: p.filename,
		List0:    decls,
	}, nil
}

func (p *parser) parseTopLevelDecl() (*a.Node, error) {
	line := p.src[0].Line
	switch p.src[0].ID {
	case t.IDFunc:
		flags := a.Flags(0)
		p.src = p.src[1:]
		id0, id1, err := p.parseQualifiedIdent()
		if err != nil {
			return nil, err
		}
		if p.peekID() == t.IDQuestion {
			flags |= a.FlagsSuspendible
			p.src = p.src[1:]
		}
		inParams, err := p.parseList((*parser).parseParam)
		if err != nil {
			return nil, err
		}
		outParams, err := p.parseList((*parser).parseParam)
		if err != nil {
			return nil, err
		}
		block, err := p.parseBlock()
		if err != nil {
			return nil, err
		}
		if x := p.peekID(); x != t.IDSemicolon {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected (implicit) \";\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		return &a.Node{
			Kind:     a.KFunc,
			Flags:    flags,
			Filename: p.filename,
			Line:     line,
			ID0:      id0,
			ID1:      id1,
			List0:    inParams,
			List1:    outParams,
			List2:    block,
		}, nil

	case t.IDStruct:
		flags := a.Flags(0)
		p.src = p.src[1:]
		id1, err := p.parseIdent()
		if err != nil {
			return nil, err
		}
		if p.peekID() == t.IDQuestion {
			flags |= a.FlagsSuspendible
			p.src = p.src[1:]
		}
		list0, err := p.parseList((*parser).parseParam)
		if err != nil {
			return nil, err
		}
		if x := p.peekID(); x != t.IDSemicolon {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected (implicit) \";\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		return &a.Node{
			Kind:     a.KStruct,
			Flags:    flags,
			Filename: p.filename,
			Line:     line,
			ID1:      id1,
			List0:    list0,
		}, nil

	case t.IDUse:
		p.src = p.src[1:]
		id1 := p.peekID()
		if !id1.IsStrLiteral() {
			got := p.m.ByID(id1)
			return nil, fmt.Errorf("parse: expected string literal, got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		if x := p.peekID(); x != t.IDSemicolon {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected (implicit) \";\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		return &a.Node{
			Kind:     a.KUse,
			Filename: p.filename,
			Line:     line,
			ID1:      id1,
		}, nil

	}
	return nil, fmt.Errorf("parse: unrecognized top level declaration at %s:%d", p.filename, p.src[0].Line)
}

// parseQualifiedIdent parses "foo.bar" or "bar".
func (p *parser) parseQualifiedIdent() (t.ID, t.ID, error) {
	x, err := p.parseIdent()
	if err != nil {
		return 0, 0, err
	}

	if p.peekID() != t.IDDot {
		return 0, x, nil
	}
	p.src = p.src[1:]

	y, err := p.parseIdent()
	if err != nil {
		return 0, 0, err
	}
	return x, y, nil
}

func (p *parser) parseIdent() (t.ID, error) {
	if len(p.src) == 0 {
		return 0, fmt.Errorf("parse: expected identifier at %s:%d", p.filename, p.line())
	}
	x := p.src[0]
	if !x.IsIdent() {
		got := p.m.ByToken(x)
		return 0, fmt.Errorf("parse: expected identifier, got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]
	return x.ID, nil
}

func (p *parser) parseList(parseFunc func(*parser) (*a.Node, error)) ([]*a.Node, error) {
	if x := p.peekID(); x != t.IDOpenParen {
		got := p.m.ByID(x)
		return nil, fmt.Errorf("parse: expected \"(\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]

	ret := []*a.Node(nil)
	for len(p.src) > 0 {
		if p.src[0].ID == t.IDCloseParen {
			p.src = p.src[1:]
			return ret, nil
		}

		elem, err := parseFunc(p)
		if err != nil {
			return nil, err
		}
		ret = append(ret, elem)

		switch x := p.peekID(); x {
		case t.IDCloseParen:
			p.src = p.src[1:]
			return ret, nil
		case t.IDComma:
			p.src = p.src[1:]
		default:
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected \")\", got %q at %s:%d", got, p.filename, p.line())
		}
	}
	return nil, fmt.Errorf("parse: expected \")\" at %s:%d", p.filename, p.line())
}

func (p *parser) parseParam() (*a.Node, error) {
	id, err := p.parseIdent()
	if err != nil {
		return nil, err
	}
	lhs, err := p.parseType()
	if err != nil {
		return nil, err
	}
	return &a.Node{
		Kind: a.KParam,
		ID1:  id,
		LHS:  lhs,
	}, nil
}

func (p *parser) parseType() (*a.Node, error) {
	switch p.peekID() {
	case t.IDPtr:
		p.src = p.src[1:]
		rhs, err := p.parseType()
		if err != nil {
			return nil, err
		}
		return &a.Node{
			Kind: a.KType,
			ID0:  t.IDPtr,
			RHS:  rhs,
		}, nil
	}

	if x := p.peekID(); x == t.IDOpenBracket {
		p.src = p.src[1:]
		lhs, err := p.parseExpr()
		if err != nil {
			return nil, err
		}
		if x := p.peekID(); x != t.IDCloseBracket {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected \"]\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		rhs, err := p.parseType()
		if err != nil {
			return nil, err
		}
		return &a.Node{
			Kind: a.KType,
			ID0:  t.IDOpenBracket,
			LHS:  lhs,
			RHS:  rhs,
		}, nil
	}

	id0, id1, err := p.parseQualifiedIdent()
	if err != nil {
		return nil, err
	}

	mhs, rhs := (*a.Node)(nil), (*a.Node)(nil)
	if x := p.peekID(); x == t.IDOpenBracket {
		_, mhs, rhs, err = p.parseRangeOrIndex(false)
		if err != nil {
			return nil, err
		}
	}

	return &a.Node{
		Kind: a.KType,
		ID0:  id0,
		ID1:  id1,
		MHS:  mhs,
		RHS:  rhs,
	}, nil
}

// parseRangeOrIndex parses "[i:j]", "[i:]", "[:j]" and "[:]". If allowIndex,
// it also parses "[x]". The returned op is t.IDColon for a range and
// t.IDOpenBracket for an index.
func (p *parser) parseRangeOrIndex(allowIndex bool) (op t.ID, mhs *a.Node, rhs *a.Node, err error) {
	if x := p.peekID(); x != t.IDOpenBracket {
		got := p.m.ByID(x)
		return 0, nil, nil, fmt.Errorf("parse: expected \"[\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]

	if p.peekID() != t.IDColon {
		mhs, err = p.parseExpr()
		if err != nil {
			return 0, nil, nil, err
		}
	}

	switch x := p.peekID(); {
	case x == t.IDColon:
		p.src = p.src[1:]

	case x == t.IDCloseBracket && allowIndex:
		p.src = p.src[1:]
		return t.IDOpenBracket, nil, mhs, nil

	default:
		expected := `":"`
		if allowIndex {
			expected = `":" or "]"`
		}
		got := p.m.ByID(x)
		return 0, nil, nil, fmt.Errorf("parse: expected %s, got %q at %s:%d", expected, got, p.filename, p.line())
	}

	if p.peekID() != t.IDCloseBracket {
		rhs, err = p.parseExpr()
		if err != nil {
			return 0, nil, nil, err
		}
	}

	if x := p.peekID(); x != t.IDCloseBracket {
		got := p.m.ByID(x)
		return 0, nil, nil, fmt.Errorf("parse: expected \"]\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]

	return t.IDColon, mhs, rhs, nil
}

func (p *parser) parseBlock() ([]*a.Node, error) {
	if x := p.peekID(); x != t.IDOpenCurly {
		got := p.m.ByID(x)
		return nil, fmt.Errorf("parse: expected \"{\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]

	block := []*a.Node(nil)
	for len(p.src) > 0 {
		if p.src[0].ID == t.IDCloseCurly {
			p.src = p.src[1:]
			return block, nil
		}

		s, err := p.parseStatement()
		if err != nil {
			return nil, err
		}
		block = append(block, s)

		if x := p.peekID(); x != t.IDSemicolon {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected (implicit) \";\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
	}
	return nil, fmt.Errorf("parse: expected \"}\" at %s:%d", p.filename, p.line())
}

func (p *parser) parseStatement() (*a.Node, error) {
	line := uint32(0)
	if len(p.src) > 0 {
		line = p.src[0].Line
	}
	n, err := p.parseStatement1()
	if n != nil {
		n.Filename = p.filename
		n.Line = line
	}
	return n, err
}

func (p *parser) parseStatement1() (*a.Node, error) {
	switch x := p.peekID(); x {
	case t.IDAssert:
		p.src = p.src[1:]
		rhs, err := p.parseExpr()
		if err != nil {
			return nil, err
		}
		return &a.Node{
			Kind: a.KAssert,
			RHS:  rhs,
		}, nil

	case t.IDBreak:
		p.src = p.src[1:]
		return &a.Node{
			Kind: a.KBreak,
		}, nil

	case t.IDContinue:
		p.src = p.src[1:]
		return &a.Node{
			Kind: a.KContinue,
		}, nil

	case t.IDFor:
		p.src = p.src[1:]
		lhs, err := (*a.Node)(nil), error(nil)
		if p.peekID() != t.IDOpenCurly {
			lhs, err = p.parseExpr()
			if err != nil {
				return nil, err
			}
		}
		block, err := p.parseBlock()
		if err != nil {
			return nil, err
		}
		return &a.Node{
			Kind:  a.KFor,
			LHS:   lhs,
			List2: block,
		}, nil

	case t.IDIf:
		return p.parseIf()

	case t.IDReturn:
		p.src = p.src[1:]
		lhs, err := (*a.Node)(nil), error(nil)
		if p.peekID() != t.IDSemicolon {
			lhs, err = p.parseExpr()
			if err != nil {
				return nil, err
			}
		}
		return &a.Node{
			Kind: a.KReturn,
			LHS:  lhs,
		}, nil

	case t.IDVar:
		p.src = p.src[1:]
		id, err := p.parseIdent()
		if err != nil {
			return nil, err
		}
		lhs, err := p.parseType()
		if err != nil {
			return nil, err
		}
		rhs := (*a.Node)(nil)
		if p.peekID() == t.IDEq {
			p.src = p.src[1:]
			rhs, err = p.parseExpr()
			if err != nil {
				return nil, err
			}
		}
		return &a.Node{
			Kind: a.KVar,
			ID1:  id,
			LHS:  lhs,
			RHS:  rhs,
		}, nil
	}

	lhs, err := p.parseExpr()
	if err != nil {
		return nil, err
	}

	if op := p.peekID(); op.IsAssign() {
		p.src = p.src[1:]
		rhs, err := p.parseExpr()
		if err != nil {
			return nil, err
		}
		return &a.Node{
			Kind: a.KAssign,
			ID0:  op,
			LHS:  lhs,
			RHS:  rhs,
		}, nil
	}

	return lhs, nil
}

func (p *parser) parseIf() (*a.Node, error) {
	if x := p.peekID(); x != t.IDIf {
		got := p.m.ByID(x)
		return nil, fmt.Errorf("parse: expected \"if\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]
	lhs, err := p.parseExpr()
	if err != nil {
		return nil, err
	}
	list1, err := p.parseBlock()
	if err != nil {
		return nil, err
	}
	rhs, list2 := (*a.Node)(nil), ([]*a.Node)(nil)
	if p.peekID() == t.IDElse {
		p.src = p.src[1:]
		if p.peekID() == t.IDIf {
			rhs, err = p.parseIf()
			if err != nil {
				return nil, err
			}
		} else {
			list2, err = p.parseBlock()
			if err != nil {
				return nil, err
			}
		}
	}
	return &a.Node{
		Kind:  a.KIf,
		LHS:   lhs,
		RHS:   rhs,
		List1: list1,
		List2: list2,
	}, nil
}

func (p *parser) parseExpr() (*a.Node, error) {
	lhs, err := p.parseOperand()
	if err != nil {
		return nil, err
	}
	if x := p.peekID(); x.IsBinaryOp() {
		p.src = p.src[1:]
		parseFunc := (*parser).parseOperand
		if x == t.IDAs {
			parseFunc = (*parser).parseType
		}
		rhs, err := parseFunc(p)
		if err != nil {
			return nil, err
		}

		if !x.IsAssociativeOp() || x != p.peekID() {
			id0 := x.BinaryForm()
			if id0 == 0 {
				return nil, fmt.Errorf("parse: internal error: no binary form for token.Key 0x%#02x", x.Key())
			}
			return &a.Node{
				Kind: a.KExpr,
				ID0:  id0,
				LHS:  lhs,
				RHS:  rhs,
			}, nil
		}

		list0 := []*a.Node{lhs, rhs}
		for p.peekID() == x {
			p.src = p.src[1:]
			arg, err := p.parseOperand()
			if err != nil {
				return nil, err
			}
			list0 = append(list0, arg)
		}
		id0 := x.AssociativeForm()
		if id0 == 0 {
			return nil, fmt.Errorf("parse: internal error: no associative form for token.Key 0x%#02x", x.Key())
		}
		return &a.Node{
			Kind:  a.KExpr,
			ID0:   id0,
			List0: list0,
		}, nil
	}
	return lhs, nil
}

func (p *parser) parseOperand() (*a.Node, error) {
	switch x := p.peekID(); {
	case x == t.IDOpenParen:
		p.src = p.src[1:]
		expr, err := p.parseExpr()
		if err != nil {
			return nil, err
		}
		if x := p.peekID(); x != t.IDCloseParen {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected \")\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		return expr, nil

	case x.IsUnaryOp():
		p.src = p.src[1:]
		rhs, err := p.parseOperand()
		if err != nil {
			return nil, err
		}
		id0 := x.UnaryForm()
		if id0 == 0 {
			return nil, fmt.Errorf("parse: internal error: no unary form for token.Key 0x%#02x", x.Key())
		}
		return &a.Node{
			Kind: a.KExpr,
			ID0:  id0,
			RHS:  rhs,
		}, nil

	case x.IsLiteral():
		p.src = p.src[1:]
		return &a.Node{
			Kind: a.KExpr,
			ID1:  x,
		}, nil
	}

	id, err := p.parseIdent()
	if err != nil {
		return nil, err
	}
	lhs := &a.Node{
		Kind: a.KExpr,
		ID1:  id,
	}

	for {
		flags := a.Flags(0)
		switch p.peekID() {
		default:
			return lhs, nil

		case t.IDQuestion:
			p.src = p.src[1:]
			flags |= a.FlagsSuspendible
			fallthrough

		case t.IDOpenParen:
			list0, err := p.parseList((*parser).parseExpr)
			if err != nil {
				return nil, err
			}
			lhs = &a.Node{
				Kind:  a.KExpr,
				Flags: flags,
				ID0:   t.IDOpenParen,
				LHS:   lhs,
				List0: list0,
			}

		case t.IDOpenBracket:
			id0, mhs, rhs, err := p.parseRangeOrIndex(true)
			if err != nil {
				return nil, err
			}
			lhs = &a.Node{
				Kind: a.KExpr,
				ID0:  id0,
				LHS:  lhs,
				MHS:  mhs,
				RHS:  rhs,
			}

		case t.IDDot:
			p.src = p.src[1:]
			id, err := p.parseIdent()
			if err != nil {
				return nil, err
			}
			lhs = &a.Node{
				Kind: a.KExpr,
				ID0:  t.IDDot,
				ID1:  id,
				LHS:  lhs,
			}
		}
	}
}
