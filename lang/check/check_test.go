// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"bytes"
	"fmt"
	"reflect"
	"sort"
	"strings"
	"testing"

	"github.com/google/puffs/lang/ast"
	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/render"
	"github.com/google/puffs/lang/token"
)

func compareToPuffsfmt(idMap *token.IDMap, tokens []token.Token, src string) error {
	buf := &bytes.Buffer{}
	if err := render.Render(buf, idMap, tokens, nil); err != nil {
		return err
	}
	got := strings.Split(buf.String(), "\n")
	want := strings.Split(src, "\n")
	for line := 1; len(got) != 0 || len(want) != 0; line++ {
		if len(got) == 0 {
			w := strings.TrimSpace(want[0])
			return fmt.Errorf("difference at line %d:\n%s\n%s", line, "", w)
		}
		if len(want) == 0 {
			g := strings.TrimSpace(got[0])
			return fmt.Errorf("difference at line %d:\n%s\n%s", line, g, "")
		}
		if g, w := strings.TrimSpace(got[0]), strings.TrimSpace(want[0]); g != w {
			return fmt.Errorf("difference at line %d:\n%s\n%s", line, g, w)
		}
		got = got[1:]
		want = want[1:]
	}
	return nil
}

func TestCheck(t *testing.T) {
	const filename = "foo.puffs"
	src := strings.TrimSpace(`
		func bar()() {
			var x u8
			var y i32 = +2
			var z u64[:123]
			var a[4] u8
			var b bool

			x = 0
			y = -y
			b = not true

			y = x as i32
		}
	`) + "\n"

	idMap := &token.IDMap{}

	tokens, _, err := token.Tokenize(idMap, filename, []byte(src))
	if err != nil {
		t.Fatalf("Tokenize: %v", err)
	}

	file, err := parse.Parse(idMap, filename, tokens)
	if err != nil {
		t.Fatalf("Parse: %v", err)
	}

	if err := compareToPuffsfmt(idMap, tokens, src); err != nil {
		t.Fatalf("compareToPuffsfmt: %v", err)
	}

	c, err := Check(idMap, file)
	if err != nil {
		t.Fatalf("Check: %v", err)
	}

	funcs := c.Funcs()
	if len(funcs) != 1 {
		t.Fatalf("Funcs: got %d elements, want 1", len(funcs))
	}
	bar := Func{}
	for _, f := range funcs {
		bar = f
		break
	}

	if got, want := idMap.ByID(bar.QID[1]), "bar"; got != want {
		t.Fatalf("Funcs[0] name: got %q, want %q", got, want)
	}

	got := [][2]string(nil)
	for id, typ := range bar.LocalVars {
		got = append(got, [2]string{
			idMap.ByID(id),
			typ.String(idMap),
		})
	}
	sort.Slice(got, func(i, j int) bool {
		return got[i][0] < got[j][0]
	})

	want := [][2]string{
		{"a", "[4] u8"},
		{"b", "bool"},
		{"x", "u8"},
		{"y", "i32"},
		{"z", "u64[:123]"},
	}
	if !reflect.DeepEqual(got, want) {
		t.Fatalf("\ngot  %v\nwant %v", got, want)
	}

	walk(file.Node(), func(n *ast.Node) error {
		if n.Kind() == ast.KExpr {
			e := n.Expr()
			if typ := e.MType(); typ == nil {
				t.Errorf("expression node %q has no (implicit) type", n.Expr().String(idMap))
			} else if typ == TypeExprIdealNumber && e.ConstValue() == nil {
				t.Errorf("expression node %q has ideal number type but no const value", n.Expr().String(idMap))
			}
		}
		return nil
	})
}

func walk(n *ast.Node, f func(*ast.Node) error) error {
	if err := f(n); err != nil {
		return err
	}
	for _, m := range n.Raw().SubNodes() {
		if m != nil {
			if err := walk(m, f); err != nil {
				return err
			}
		}
	}
	for _, l := range n.Raw().SubLists() {
		for _, m := range l {
			if m != nil {
				if err := walk(m, f); err != nil {
					return err
				}
			}
		}
	}
	return nil
}
