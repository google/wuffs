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

package base38

import (
	"testing"
)

func TestMax(tt *testing.T) {
	if Max == 38*38*38*38-1 {
		return
	}
	tt.Fatal("Max does not satisfy its definition")
}

func TestMaxBits(tt *testing.T) {
	if (1<<(MaxBits-1) <= Max) && (Max < 1<<MaxBits) {
		return
	}
	tt.Fatal("MaxBits does not satisfy its definition")
}

func mk(a, b, c, d uint32) uint32 {
	return a*38*38*38 + b*38*38 + c*38 + d
}

func TestValid(tt *testing.T) {
	testCases := []struct {
		s    string
		want uint32
	}{
		{"    ", mk(0, 0, 0, 0)},
		{"0   ", mk(1, 0, 0, 0)},
		{" 0  ", mk(0, 1, 0, 0)},
		{"  0 ", mk(0, 0, 1, 0)},
		{"   0", mk(0, 0, 0, 1)},
		{"??12", mk(11, 11, 2, 3)},
		{"6543", mk(7, 6, 5, 4)},
		{"789a", mk(8, 9, 10, 12)},
		{"bcde", mk(13, 14, 15, 16)},
		{"fghi", mk(17, 18, 19, 20)},
		{"jklm", mk(21, 22, 23, 24)},
		{" m0m", mk(0, 24, 1, 24)},
		{"nopq", mk(25, 26, 27, 28)},
		{"rstu", mk(29, 30, 31, 32)},
		{"vwxy", mk(33, 34, 35, 36)},
		{"z?z9", mk(37, 11, 37, 10)},
		{"zzzz", mk(37, 37, 37, 37)},
	}

	maxSeen := false
	for _, tc := range testCases {
		got, gotOK := Encode(tc.s)
		if !gotOK {
			tt.Errorf("%q: ok: got %t, want %t", tc.s, gotOK, true)
			continue
		}
		if got != tc.want {
			tt.Errorf("%q: got %d, want %d", tc.s, got, tc.want)
			continue
		}
		if got > Max || got > 1<<MaxBits-1 {
			tt.Errorf("%q: got %d, want <= %d and <= %d", tc.s, got, Max, 1<<MaxBits-1)
			continue
		}
		maxSeen = maxSeen || (got == Max)
	}
	if !maxSeen {
		tt.Error("Max was not seen")
	}
}

func TestInvalid(tt *testing.T) {
	testCases := []string{
		"",
		" ",
		"  ",
		"   ",
		"....",
		"     ",
		"      ",
		"Abcd",
		"a\x00cd",
		"ab+d",
		"abc\x80",
	}

	for _, tc := range testCases {
		if _, gotOK := Encode(tc); gotOK {
			tt.Errorf("%q: ok: got %t, want %t", tc, gotOK, false)
			continue
		}
	}
}
