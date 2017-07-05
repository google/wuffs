// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// Package base38 converts a 4-byte string, each byte in [ 0-9?a-z], to a base
// 38 number.
package base38

const (
	// Max is the inclusive upper bound for the value returned by Encode. Its
	// value equals (38*38*38*38 - 1).
	Max = 2085135
	// MaxBits is the number of bits required to represent Max. It satisfies
	// (1<<(MaxBits-1) <= Max) && (Max < 1<<MaxBits).
	MaxBits = 21
)

// Encode encodes a 4-byte string as a uint32 in the range [0, Max].
//
// Each byte must be ' ', be in the range '0'-'9', be '?' or be in the range
// 'a'-'z'.
//
// The string "    " is mapped to zero.
func Encode(s string) (u uint32, ok bool) {
	if len(s) == 4 {
		for i := 0; i < 4; i++ {
			x := uint32(table[s[i]])
			if x == 0 {
				break
			}
			u += x - 1
			if i == 3 {
				return u, true
			}
			u *= 38
		}
	}
	return 0, false
}

var table = [256]uint8{
	' ': 1,
	'0': 2,
	'1': 3,
	'2': 4,
	'3': 5,
	'4': 6,
	'5': 7,
	'6': 8,
	'7': 9,
	'8': 10,
	'9': 11,
	'?': 12,
	'a': 13,
	'b': 14,
	'c': 15,
	'd': 16,
	'e': 17,
	'f': 18,
	'g': 19,
	'h': 20,
	'i': 21,
	'j': 22,
	'k': 23,
	'l': 24,
	'm': 25,
	'n': 26,
	'o': 27,
	'p': 28,
	'q': 29,
	'r': 30,
	's': 31,
	't': 32,
	'u': 33,
	'v': 34,
	'w': 35,
	'x': 36,
	'y': 37,
	'z': 38,
}
