// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"reflect"
	"sort"
	"testing"

	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/token"
)

func TestCheck(t *testing.T) {
	const filename = "foo.puffs"
	const src = `
		func bar()() {
			x = 1 as u8
			y = 2 as i32
		}
	`

	idMap := &token.IDMap{}

	tokens, _, err := token.Tokenize([]byte(src), idMap, filename)
	if err != nil {
		t.Fatalf("Tokenize: %v", err)
	}

	node, err := parse.ParseFile(tokens, idMap, filename)
	if err != nil {
		t.Fatalf("ParseFile: %v", err)
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
			TypeString(typ, idMap),
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
}
