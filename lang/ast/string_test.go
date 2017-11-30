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

package ast_test

import (
	"testing"

	"github.com/google/wuffs/lang/parse"
	"github.com/google/wuffs/lang/token"
)

func TestString(t *testing.T) {
	const filename = "test.puffs"
	testCases := []string{
		"1",
		"x",

		"f()",
		"f(a:i)",
		"f(a:i, b:j)",
		"f(a:i, b:j) + 1",
		"f(a:i, b:j)(c:k)",
		"f(a:i, b:j)(c:k, d:l, e:m + 2) + 3",

		"x[i]",
		"x[i][j]",
		"x[i:j]",
		"x[i](arg:j)",
		"x(arg:i)[j]",

		"x.y",
		"x.y.z.a",

		"+x",
		"-(x + y)",
		"not x",
		"not not x",
		"+++-x",

		"x + 42",
		"x and (y < z)",
		"x & (y as u8)",
		"x * ((a / b) - (i / j))",

		"x + y + z",
		"x + (i * j.k[l] * (-m << 4) * (n & o(o0:p, o1:q[:r.s + 5]))) + z",

		"x as bool",
		"x as u32",
		"x as T",
		"x as T",
		"x as T[i..]",
		"x as T[..j]",
		"x as T[i..j]",
		"x as pkg.T",
		"x as ptr T",
		"x as [4] T",
		"x as [8 + (2 * N)] ptr [4] ptr pkg.T[i..j]",
	}

	tm := &token.Map{}
	for _, tc := range testCases {
		tokens, _, err := token.Tokenize(tm, filename, []byte(tc))
		if err != nil {
			t.Errorf("Tokenize(%q): %v", tc, err)
			continue
		}
		expr, err := parse.ParseExpr(tm, filename, tokens)
		if err != nil {
			t.Errorf("ParseExpr(%q): %v", tc, err)
			continue
		}
		got := expr.String(tm)
		if got != tc {
			t.Errorf("got %q, want %q", got, tc)
			continue
		}
	}
}
