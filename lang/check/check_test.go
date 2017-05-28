// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package check

import (
	"bytes"
	"fmt"
	"math/big"
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
	const filename = "test.puffs"
	src := strings.TrimSpace(`
		pri struct foo(
			i i32,
		)

		pri func foo.bar()() {
			var x u8
			var y i32 = +2
			var z u64[..123]
			var a[4] u8
			var b bool

			x = 0
			x = 1 + (x * 0)
			y = -y - 1
			y = this.i
			b = not true

			y = x as i32

			var p i32
			var q i32[0..8]

			assert true

			while p == q,
				post p != q,
			{
			}
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

	c, err := Check(idMap, 0, file)
	if err != nil {
		t.Fatalf("Check: %v", err)
	}

	funcs := c.Funcs()
	if len(funcs) != 1 {
		t.Fatalf("Funcs: got %d elements, want 1", len(funcs))
	}
	fooBar := Func{}
	for _, f := range funcs {
		fooBar = f
		break
	}

	if got, want := fooBar.QID.String(idMap), "foo.bar"; got != want {
		t.Fatalf("Funcs[0] name: got %q, want %q", got, want)
	}

	got := [][2]string(nil)
	for id, typ := range fooBar.LocalVars {
		got = append(got, [2]string{
			id.String(idMap),
			typ.String(idMap),
		})
	}
	sort.Slice(got, func(i, j int) bool {
		return got[i][0] < got[j][0]
	})

	want := [][2]string{
		{"a", "[4] u8"},
		{"b", "bool"},
		{"in", "in"},
		{"out", "out"},
		{"p", "i32"},
		{"q", "i32[0..8]"},
		{"this", "ptr foo"},
		{"x", "u8"},
		{"y", "i32"},
		{"z", "u64[..123]"},
	}
	if !reflect.DeepEqual(got, want) {
		t.Fatalf("\ngot  %v\nwant %v", got, want)
	}
}

func TestConstValues(t *testing.T) {
	const filename = "test.puffs"
	testCases := map[string]int64{
		"var i i32 = 42": 42,

		"var i i32 = +7": +7,
		"var i i32 = -7": -7,

		"var b bool = false":         0,
		"var b bool = not false":     1,
		"var b bool = not not false": 0,

		"var i i32 = 10  + 3": 13,
		"var i i32 = 10  - 3": 7,
		"var i i32 = 10  * 3": 30,
		"var i i32 = 10  / 3": 3,
		"var i i32 = 10 << 3": 80,
		"var i i32 = 10 >> 3": 1,
		"var i i32 = 10  & 3": 2,
		"var i i32 = 10 &^ 3": 8,
		"var i i32 = 10  | 3": 11,
		"var i i32 = 10  ^ 3": 9,

		"var b bool = 10 != 3": 1,
		"var b bool = 10  < 3": 0,
		"var b bool = 10 <= 3": 0,
		"var b bool = 10 == 3": 0,
		"var b bool = 10 >= 3": 1,
		"var b bool = 10  > 3": 1,

		"var b bool = false and true": 0,
		"var b bool = false  or true": 1,
	}

	idMap := &token.IDMap{}
	for s, wantInt64 := range testCases {
		src := "pri func foo()() {\n\t" + s + "\n}\n"

		tokens, _, err := token.Tokenize(idMap, filename, []byte(src))
		if err != nil {
			t.Errorf("%q: Tokenize: %v", s, err)
			continue
		}

		file, err := parse.Parse(idMap, filename, tokens)
		if err != nil {
			t.Errorf("%q: Parse: %v", s, err)
			continue
		}

		c, err := Check(idMap, 0, file)
		if err != nil {
			t.Errorf("%q: Check: %v", s, err)
			continue
		}

		funcs := c.Funcs()
		if len(funcs) != 1 {
			t.Errorf("%q: Funcs: got %d elements, want 1", s, len(funcs))
			continue
		}
		foo := Func{}
		for _, f := range funcs {
			foo = f
			break
		}
		body := foo.Func.Body()
		if len(body) != 1 {
			t.Errorf("%q: Body: got %d elements, want 1", s, len(body))
			continue
		}
		if body[0].Kind() != ast.KVar {
			t.Errorf("%q: Body[0]: got %s, want %s", s, body[0].Kind(), ast.KVar)
			continue
		}

		got := body[0].Var().Value().ConstValue()
		want := big.NewInt(wantInt64)
		if got == nil || want == nil || got.Cmp(want) != 0 {
			t.Errorf("%q: got %v, want %v", s, got, want)
			continue
		}
	}
}
