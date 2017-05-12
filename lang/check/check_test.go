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
			var z u64[:123]
			var a [4]u8
			y = x as i32
		}
	`

	idMap := &token.IDMap{}

	tokens, _, err := token.Tokenize(idMap, filename, []byte(src))
	if err != nil {
		t.Fatalf("Tokenize: %v", err)
	}

	file, err := parse.Parse(idMap, filename, tokens)
	if err != nil {
		t.Fatalf("Parse: %v", err)
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
			TypeExprString(idMap, typ),
		})
	}
	sort.Slice(got, func(i, j int) bool {
		return got[i][0] < got[j][0]
	})

	want := [][2]string{
		{"a", "[4] u8"},
		{"x", "u8"},
		{"y", "i32"},
		{"z", "u64[:123]"},
	}
	if !reflect.DeepEqual(got, want) {
		t.Fatalf("\ngot  %v\nwant %v", got, want)
	}

	walk(file.Node(), func(n *ast.Node) error {
		if n.Kind() == ast.KExpr && n.Expr().MType() == nil {
			t.Errorf("expression node has no (implicit) type: n=%v", n)
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
