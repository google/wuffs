// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"reflect"
	"sort"
	"testing"

	"github.com/google/puffs/lang/ast"
	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/token"
)

func TestCheck(t *testing.T) {
	const filename = "foo.puffs"
	const src = `
		func bar()() {
			var x u8
			var y i32 = 2
		}
	`

	idMap := &token.IDMap{}

	tokens, _, err := token.Tokenize(idMap, filename, []byte(src))
	if err != nil {
		t.Fatalf("Tokenize: %v", err)
	}

	node, err := parse.Parse(idMap, filename, tokens)
	if err != nil {
		t.Fatalf("Parse: %v", err)
	}

	c, err := Check(idMap, node)
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
			TypeString(idMap, typ),
		})
	}
	sort.Slice(got, func(i, j int) bool {
		return got[i][0] < got[j][0]
	})

	want := [][2]string{
		{"x", "u8"},
		{"y", "i32"},
	}
	if !reflect.DeepEqual(got, want) {
		t.Fatalf("\ngot  %v\nwant %v", got, want)
	}

	walk(node, func(n *ast.Node) error {
		if n.Kind == ast.KExpr && n.Type == nil {
			t.Errorf("expression node has no type: n=%v", n)
		}
		return nil
	})
}

func walk(n *ast.Node, f func(*ast.Node) error) error {
	if n == nil {
		return nil
	}
	if err := f(n); err != nil {
		return err
	}
	for _, m := range [...]*ast.Node{n.LHS, n.MHS, n.RHS} {
		if err := walk(m, f); err != nil {
			return err
		}
	}
	for _, l := range [...][]*ast.Node{n.List0, n.List1, n.List2} {
		for _, m := range l {
			if err := walk(m, f); err != nil {
				return err
			}
		}
	}
	return nil
}
