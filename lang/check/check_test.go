// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package check

import (
	"bytes"
	"fmt"
	"math/big"
	"reflect"
	"sort"
	"strings"
	"testing"

	"github.com/google/wuffs/lang/builtin"
	"github.com/google/wuffs/lang/parse"
	"github.com/google/wuffs/lang/render"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

func compareToWuffsfmt(tm *t.Map, tokens []t.Token, comments []string, src string) error {
	buf := &bytes.Buffer{}
	if err := render.Render(buf, tm, tokens, comments); err != nil {
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

func TestCheck(tt *testing.T) {
	const filename = "test.wuffs"
	src := strings.TrimSpace(`
		packageid "test"

		pri struct foo(
			i i32,
		)

		pri func foo.bar()() {
			var x u8
			var y i32 = +2
			var z u64[..123]
			var a array[4] u8
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

			while:label p == q,
				pre true,
				inv true,
				post p != q,
			{
				// Redundant, but shows the labeled jump syntax.
				continue:label
			}
		}
	`) + "\n"

	tm := &t.Map{}

	tokens, comments, err := t.Tokenize(tm, filename, []byte(src))
	if err != nil {
		tt.Fatalf("Tokenize: %v", err)
	}

	file, err := parse.Parse(tm, filename, tokens, nil)
	if err != nil {
		tt.Fatalf("Parse: %v", err)
	}

	if err := compareToWuffsfmt(tm, tokens, comments, src); err != nil {
		tt.Fatalf("compareToWuffsfmt: %v", err)
	}

	c, err := Check(tm, []*a.File{file}, nil)
	if err != nil {
		tt.Fatalf("Check: %v", err)
	}

	if len(c.funcs) != 1 {
		tt.Fatalf("Funcs: got %d elements, want 1", len(c.funcs))
	}
	fooBar := (*a.Func)(nil)
	fooBarLocalVars := typeMap(nil)
	for _, f := range c.funcs {
		fooBar = f
		break
	}
	for _, v := range c.localVars {
		fooBarLocalVars = v
		break
	}

	if got, want := fooBar.QQID().Str(tm), "foo.bar"; got != want {
		tt.Fatalf("Funcs[0] name: got %q, want %q", got, want)
	}

	got := [][2]string(nil)
	for id, typ := range fooBarLocalVars {
		got = append(got, [2]string{
			id.Str(tm),
			typ.Str(tm),
		})
	}
	sort.Slice(got, func(i, j int) bool {
		return got[i][0] < got[j][0]
	})

	want := [][2]string{
		{"a", "array[4] u8"},
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
		tt.Fatalf("\ngot  %v\nwant %v", got, want)
	}
}

func TestConstValues(tt *testing.T) {
	const filename = "test.wuffs"
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

	tm := &t.Map{}
	for s, wantInt64 := range testCases {
		src := "packageid \"test\"\npri func foo()() {\n\t" + s + "\n}\n"

		tokens, _, err := t.Tokenize(tm, filename, []byte(src))
		if err != nil {
			tt.Errorf("%q: Tokenize: %v", s, err)
			continue
		}

		file, err := parse.Parse(tm, filename, tokens, nil)
		if err != nil {
			tt.Errorf("%q: Parse: %v", s, err)
			continue
		}

		c, err := Check(tm, []*a.File{file}, nil)
		if err != nil {
			tt.Errorf("%q: Check: %v", s, err)
			continue
		}

		if len(c.funcs) != 1 {
			tt.Errorf("%q: Funcs: got %d elements, want 1", s, len(c.funcs))
			continue
		}
		foo := (*a.Func)(nil)
		for _, f := range c.funcs {
			foo = f
			break
		}
		body := foo.Body()
		if len(body) != 1 {
			tt.Errorf("%q: Body: got %d elements, want 1", s, len(body))
			continue
		}
		if body[0].Kind() != a.KVar {
			tt.Errorf("%q: Body[0]: got %s, want %s", s, body[0].Kind(), a.KVar)
			continue
		}

		got := body[0].Var().Value().ConstValue()
		want := big.NewInt(wantInt64)
		if got == nil || want == nil || got.Cmp(want) != 0 {
			tt.Errorf("%q: got %v, want %v", s, got, want)
			continue
		}
	}
}

func TestBitMask(tt *testing.T) {
	testCases := [][2]uint64{
		{0, 0},
		{1, 1},
		{2, 3},
		{3, 3},
		{4, 7},
		{5, 7},
		{50, 63},
		{500, 511},
		{5000, 8191},
		{50000, 65535},
		{500000, 524287},
		{1<<7 - 2, 1<<7 - 1},
		{1<<7 - 1, 1<<7 - 1},
		{1<<7 + 0, 1<<8 - 1},
		{1<<7 + 1, 1<<8 - 1},
		{1<<8 - 2, 1<<8 - 1},
		{1<<8 - 1, 1<<8 - 1},
		{1<<8 + 0, 1<<9 - 1},
		{1<<8 + 1, 1<<9 - 1},
		{1<<32 - 2, 1<<32 - 1},
		{1<<32 - 1, 1<<32 - 1},
		{1<<32 + 0, 1<<33 - 1},
		{1<<32 + 1, 1<<33 - 1},
		{1<<64 - 2, 1<<64 - 1},
		{1<<64 - 1, 1<<64 - 1},
	}

	for _, tc := range testCases {
		got := bitMask(big.NewInt(0).SetUint64(tc[0]).BitLen())
		want := big.NewInt(0).SetUint64(tc[1])
		if got.Cmp(want) != 0 {
			tt.Errorf("roundUpToPowerOf2Minus1(%v): got %v, want %v", tc[0], got, tc[1])
		}
	}
}

func TestBuiltInTypeMap(tt *testing.T) {
	if got, want := len(builtInTypeMap), len(builtin.Types); got != want {
		tt.Fatalf("lengths: got %d, want %d", got, want)
	}
	tm := t.Map{}
	for _, s := range builtin.Types {
		id := tm.ByName(s)
		if id == 0 {
			tt.Fatalf("ID(%q): got 0, want non-0", s)
		}
		if _, ok := builtInTypeMap[id]; !ok {
			tt.Fatalf("no builtInTypeMap entry for %q", s)
		}
	}
}
