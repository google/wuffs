// Copyright 2020 The Wuffs Authors.
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

package dumbindent

import (
	"testing"
)

func TestFormatBytes(tt *testing.T) {
	testCases := []struct {
		src  string
		want string
	}{{
		// Braces.
		src:  "foo{\nbar\n    }\nbaz\n",
		want: "foo{\n  bar\n}\nbaz\n",
	}, {
		// Parentheses.
		src:  "i = (j +\nk)\np = q\n",
		want: "i = (j +\n    k)\np = q\n",
	}, {
		// Hanging assignment.
		src:  "int x =\ny;\n",
		want: "int x =\n    y;\n",
	}, {
		// Hanging assignment with slash-slash comment.
		src:  "int x = // comm.\ny;\n",
		want: "int x = // comm.\n    y;\n",
	}, {
		// Most blank lines should be dropped.
		src:  "f {\n\n\ng;\n\n\nh;\n\n\n}\n\n",
		want: "f {\n  g;\n\n  h;\n}\n",
	}, {
		// Single-quote string.
		src:  "a = '{'\nb = {\nc = 0\n",
		want: "a = '{'\nb = {\n  c = 0\n",
	}, {
		// Double-quote string.
		src:  "a = \"{\"\nb = {\nc = 0\n",
		want: "a = \"{\"\nb = {\n  c = 0\n",
	}, {
		// Label.
		src:  "if (b) {\nlabel:\nswitch (i) {\ncase 0:\nj = k\nbreak;\n}\n}\n",
		want: "if (b) {\nlabel:\n  switch (i) {\n    case 0:\n    j = k\n    break;\n  }\n}\n",
	}}

	for i, tc := range testCases {
		if g, err := FormatBytes(nil, []byte(tc.src)); err != nil {
			tt.Fatalf("i=%d, src=%q: %v", i, tc.src, err)
		} else if got := string(g); got != tc.want {
			tt.Fatalf("i=%d, src=%q:\ngot  %q\nwant %q", i, tc.src, got, tc.want)
		}
	}
}
