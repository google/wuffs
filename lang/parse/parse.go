// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package parse

// TODO: write a formal grammar for the language.

import (
	"fmt"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

func Parse(m *t.IDMap, filename string, src []t.Token) (*a.File, error) {
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

func (p *parser) parseFile() (*a.File, error) {
	topLevelDecls := []*a.Node(nil)
	for len(p.src) > 0 {
		d, err := p.parseTopLevelDecl()
		if err != nil {
			return nil, err
		}
		topLevelDecls = append(topLevelDecls, d)
	}
	return a.NewFile(p.filename, topLevelDecls), nil
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
		if id0 != 0 && id0.IsBuiltIn() {
			return nil, fmt.Errorf("parse: built-in %q used for func receiver at %s:%d",
				p.m.ByID(id0), p.filename, p.line())
		}
		if id1.IsBuiltIn() {
			return nil, fmt.Errorf("parse: built-in %q used for func name at %s:%d",
				p.m.ByID(id1), p.filename, p.line())
		}
		if p.peekID() == t.IDQuestion {
			flags |= a.FlagsSuspendible
			p.src = p.src[1:]
		}
		inParams, err := p.parseList((*parser).parseParamNode)
		if err != nil {
			return nil, err
		}
		outParams, err := p.parseList((*parser).parseParamNode)
		if err != nil {
			return nil, err
		}
		body, err := p.parseBlock()
		if err != nil {
			return nil, err
		}
		if x := p.peekID(); x != t.IDSemicolon {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected (implicit) \";\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		return a.NewFunc(flags, p.filename, line, id0, id1, inParams, outParams, body).Node(), nil

	case t.IDStruct:
		flags := a.Flags(0)
		p.src = p.src[1:]
		name, err := p.parseIdent()
		if err != nil {
			return nil, err
		}
		if name.IsBuiltIn() {
			return nil, fmt.Errorf("parse: built-in %q used for struct name at %s:%d",
				p.m.ByID(name), p.filename, p.line())
		}
		if p.peekID() == t.IDQuestion {
			flags |= a.FlagsSuspendible
			p.src = p.src[1:]
		}
		fields, err := p.parseList((*parser).parseParamNode)
		if err != nil {
			return nil, err
		}
		if x := p.peekID(); x != t.IDSemicolon {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected (implicit) \";\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		return a.NewStruct(flags, p.filename, line, name, fields).Node(), nil

	case t.IDUse:
		p.src = p.src[1:]
		path := p.peekID()
		if !path.IsStrLiteral() {
			got := p.m.ByID(path)
			return nil, fmt.Errorf("parse: expected string literal, got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		if x := p.peekID(); x != t.IDSemicolon {
			got := p.m.ByID(x)
			return nil, fmt.Errorf("parse: expected (implicit) \";\", got %q at %s:%d", got, p.filename, p.line())
		}
		p.src = p.src[1:]
		return a.NewUse(p.filename, line, path).Node(), nil

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

func (p *parser) parseList(parseElem func(*parser) (*a.Node, error)) ([]*a.Node, error) {
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

		elem, err := parseElem(p)
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

func (p *parser) parseParamNode() (*a.Node, error) {
	name, err := p.parseIdent()
	if err != nil {
		return nil, err
	}
	typ, err := p.parseTypeExpr()
	if err != nil {
		return nil, err
	}
	return a.NewParam(name, typ).Node(), nil
}

func (p *parser) parseTypeExpr() (*a.TypeExpr, error) {
	switch p.peekID() {
	case t.IDPtr:
		p.src = p.src[1:]
		rhs, err := p.parseTypeExpr()
		if err != nil {
			return nil, err
		}
		return a.NewTypeExpr(t.IDPtr, 0, nil, nil, rhs), nil
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
		rhs, err := p.parseTypeExpr()
		if err != nil {
			return nil, err
		}
		return a.NewTypeExpr(t.IDOpenBracket, 0, lhs, nil, rhs), nil
	}

	pkg, name, err := p.parseQualifiedIdent()
	if err != nil {
		return nil, err
	}

	lhs, mhs := (*a.Expr)(nil), (*a.Expr)(nil)
	if x := p.peekID(); x == t.IDOpenBracket {
		_, lhs, mhs, err = p.parseRangeOrIndex(false)
		if err != nil {
			return nil, err
		}
	}

	return a.NewTypeExpr(pkg, name, lhs, mhs, nil), nil
}

// parseRangeOrIndex parses "[i:j]", "[i:]", "[:j]" and "[:]". If allowIndex,
// it also parses "[x]". The returned op is t.IDColon for a range and
// t.IDOpenBracket for an index.
func (p *parser) parseRangeOrIndex(allowIndex bool) (op t.ID, ei *a.Expr, ej *a.Expr, err error) {
	if x := p.peekID(); x != t.IDOpenBracket {
		got := p.m.ByID(x)
		return 0, nil, nil, fmt.Errorf("parse: expected \"[\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]

	if p.peekID() != t.IDColon {
		ei, err = p.parseExpr()
		if err != nil {
			return 0, nil, nil, err
		}
	}

	switch x := p.peekID(); {
	case x == t.IDColon:
		p.src = p.src[1:]

	case x == t.IDCloseBracket && allowIndex:
		p.src = p.src[1:]
		return t.IDOpenBracket, nil, ei, nil

	default:
		expected := `":"`
		if allowIndex {
			expected = `":" or "]"`
		}
		got := p.m.ByID(x)
		return 0, nil, nil, fmt.Errorf("parse: expected %s, got %q at %s:%d", expected, got, p.filename, p.line())
	}

	if p.peekID() != t.IDCloseBracket {
		ej, err = p.parseExpr()
		if err != nil {
			return 0, nil, nil, err
		}
	}

	if x := p.peekID(); x != t.IDCloseBracket {
		got := p.m.ByID(x)
		return 0, nil, nil, fmt.Errorf("parse: expected \"]\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]

	return t.IDColon, ei, ej, nil
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
		n.Raw().SetFilenameLine(p.filename, line)
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
		return a.NewAssert(rhs).Node(), nil

	case t.IDBreak:
		p.src = p.src[1:]
		return a.NewBreak().Node(), nil

	case t.IDContinue:
		p.src = p.src[1:]
		return a.NewContinue().Node(), nil

	case t.IDFor:
		p.src = p.src[1:]
		condition, err := (*a.Expr)(nil), error(nil)
		if p.peekID() != t.IDOpenCurly {
			condition, err = p.parseExpr()
			if err != nil {
				return nil, err
			}
		}
		body, err := p.parseBlock()
		if err != nil {
			return nil, err
		}
		return a.NewFor(condition, body).Node(), nil

	case t.IDIf:
		o, err := p.parseIf()
		return o.Node(), err

	case t.IDReturn:
		p.src = p.src[1:]
		value, err := (*a.Expr)(nil), error(nil)
		if p.peekID() != t.IDSemicolon {
			value, err = p.parseExpr()
			if err != nil {
				return nil, err
			}
		}
		return a.NewReturn(value).Node(), nil

	case t.IDVar:
		p.src = p.src[1:]
		id, err := p.parseIdent()
		if err != nil {
			return nil, err
		}
		typ, err := p.parseTypeExpr()
		if err != nil {
			return nil, err
		}
		value := (*a.Expr)(nil)
		if p.peekID() == t.IDEq {
			p.src = p.src[1:]
			value, err = p.parseExpr()
			if err != nil {
				return nil, err
			}
		}
		return a.NewVar(id, typ, value).Node(), nil
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
		return a.NewAssign(op, lhs, rhs).Node(), nil
	}

	return lhs.Node(), nil
}

func (p *parser) parseIf() (*a.If, error) {
	if x := p.peekID(); x != t.IDIf {
		got := p.m.ByID(x)
		return nil, fmt.Errorf("parse: expected \"if\", got %q at %s:%d", got, p.filename, p.line())
	}
	p.src = p.src[1:]
	condition, err := p.parseExpr()
	if err != nil {
		return nil, err
	}
	bodyIfTrue, err := p.parseBlock()
	if err != nil {
		return nil, err
	}
	elseIf, bodyIfFalse := (*a.If)(nil), ([]*a.Node)(nil)
	if p.peekID() == t.IDElse {
		p.src = p.src[1:]
		if p.peekID() == t.IDIf {
			elseIf, err = p.parseIf()
			if err != nil {
				return nil, err
			}
		} else {
			bodyIfFalse, err = p.parseBlock()
			if err != nil {
				return nil, err
			}
		}
	}
	return a.NewIf(condition, elseIf, bodyIfTrue, bodyIfFalse), nil
}

func (p *parser) parseExprNode() (*a.Node, error) {
	n, err := p.parseExpr()
	return n.Node(), err
}

func (p *parser) parseExpr() (*a.Expr, error) {
	lhs, err := p.parseOperand()
	if err != nil {
		return nil, err
	}
	if x := p.peekID(); x.IsBinaryOp() {
		p.src = p.src[1:]
		rhs := (*a.Node)(nil)
		if x == t.IDAs {
			o, err := p.parseTypeExpr()
			if err != nil {
				return nil, err
			}
			rhs = o.Node()
		} else {
			o, err := p.parseOperand()
			if err != nil {
				return nil, err
			}
			rhs = o.Node()
		}

		if !x.IsAssociativeOp() || x != p.peekID() {
			op := x.BinaryForm()
			if op == 0 {
				return nil, fmt.Errorf("parse: internal error: no binary form for token.Key 0x%#02x", x.Key())
			}
			return a.NewExpr(0, op, 0, lhs.Node(), nil, rhs, nil), nil
		}

		args := []*a.Node{lhs.Node(), rhs}
		for p.peekID() == x {
			p.src = p.src[1:]
			arg, err := p.parseOperand()
			if err != nil {
				return nil, err
			}
			args = append(args, arg.Node())
		}
		op := x.AssociativeForm()
		if op == 0 {
			return nil, fmt.Errorf("parse: internal error: no associative form for token.Key 0x%#02x", x.Key())
		}
		return a.NewExpr(0, op, 0, nil, nil, nil, args), nil
	}
	return lhs, nil
}

func (p *parser) parseOperand() (*a.Expr, error) {
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
		op := x.UnaryForm()
		if op == 0 {
			return nil, fmt.Errorf("parse: internal error: no unary form for token.Key 0x%#02x", x.Key())
		}
		return a.NewExpr(0, op, 0, nil, nil, rhs.Node(), nil), nil

	case x.IsLiteral():
		p.src = p.src[1:]
		return a.NewExpr(0, 0, x, nil, nil, nil, nil), nil
	}

	id, err := p.parseIdent()
	if err != nil {
		return nil, err
	}
	lhs := a.NewExpr(0, 0, id, nil, nil, nil, nil)

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
			args, err := p.parseList(func(p *parser) (*a.Node, error) {
				n, err := p.parseExpr()
				return n.Node(), err
			})
			if err != nil {
				return nil, err
			}
			return a.NewExpr(flags, t.IDOpenParen, 0, lhs.Node(), nil, nil, args), nil

		case t.IDOpenBracket:
			id0, mhs, rhs, err := p.parseRangeOrIndex(true)
			if err != nil {
				return nil, err
			}
			lhs = a.NewExpr(0, id0, 0, lhs.Node(), mhs.Node(), rhs.Node(), nil)

		case t.IDDot:
			p.src = p.src[1:]
			selector, err := p.parseIdent()
			if err != nil {
				return nil, err
			}
			lhs = a.NewExpr(0, t.IDDot, selector, lhs.Node(), nil, nil, nil)
		}
	}
}
