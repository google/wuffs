// Copyright 2018 The Wuffs Authors.
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

// Package interval provides interval arithmetic on big integers. Big means
// arbitrary-precision, as per the standard math/big package.
//
// For example, if x is in the interval [3, 6] and y is in the interval [10,
// 15] then x+y is in the interval [13, 21].
//
// Such intervals may have infinite bounds. For example, if x is in the
// interval [3, +∞) and y is in the interval [-4, -2], then x*y is in the
// interval (-∞, -6].
//
// As a motivating example, if a compiler knows that the integer-typed
// variables i and j are in the intervals [0, 255] and [0, 3], and that the
// array a has 1024 elements, then it can prove that the array-index expression
// a[4*i + j] is memory-safe without needing an at-runtime bounds check.
//
// This package depends only on the standard math/big package.
package interval

import (
	"math/big"
)

func bigIntMul(i *big.Int, j *big.Int) *big.Int { return big.NewInt(0).Mul(i, j) }
func bigIntQuo(i *big.Int, j *big.Int) *big.Int { return big.NewInt(0).Quo(i, j) }

func bigIntLsh(i *big.Int, j *big.Int) *big.Int {
	if j.IsUint64() {
		if u := j.Uint64(); u <= 0xFFFFFFFF {
			return big.NewInt(0).Lsh(i, uint(u))
		}
	}

	// Fallback code path, copy-pasted to interval_test.go.
	k := big.NewInt(2)
	k.Exp(k, j, nil)
	k.Mul(i, k)
	return k
}

func bigIntRsh(i *big.Int, j *big.Int) *big.Int {
	if j.IsUint64() {
		if u := j.Uint64(); u <= 0xFFFFFFFF {
			return big.NewInt(0).Rsh(i, uint(u))
		}
	}

	// Fallback code path, copy-pasted to interval_test.go.
	k := big.NewInt(2)
	k.Exp(k, j, nil)
	k.Div(i, k) // This is explicitly Div, not Quo.
	return k
}

// biggerInt is either a non-nil *big.Int or ±∞.
type biggerInt struct {
	// extra being less than or greater than 0 means that the biggerInt is -∞
	// or +∞ respectively. extra being zero means that the biggerInt is i,
	// which should be a non-nil pointer.
	extra int32
	i     *big.Int
}

type biggerIntPair [2]biggerInt

// newBiggerIntPair returns a pair of biggerInt values, the first being +∞
// and the second being -∞.
func newBiggerIntPair() biggerIntPair { return biggerIntPair{{extra: +1}, {extra: -1}} }

// lowerMin sets x[0] to min(x[0], y).
func (x *biggerIntPair) lowerMin(y biggerInt) {
	if x[0].extra > 0 || y.extra < 0 ||
		(x[0].extra == 0 && y.extra == 0 && x[0].i.Cmp(y.i) > 0) {
		x[0] = y
	}
}

// raiseMax sets x[1] to max(x[1], y).
func (x *biggerIntPair) raiseMax(y biggerInt) {
	if x[1].extra < 0 || y.extra > 0 ||
		(x[1].extra == 0 && y.extra == 0 && x[1].i.Cmp(y.i) < 0) {
		x[1] = y
	}
}

func (x *biggerIntPair) toIntRange() IntRange {
	if x[0].extra > 0 || x[1].extra < 0 {
		return empty()
	}
	return IntRange{x[0].i, x[1].i}
}

func (x *biggerIntPair) fromIntRange(y IntRange) {
	if y[0] != nil {
		x[0] = biggerInt{i: big.NewInt(0).Set(y[0])}
	} else {
		x[0] = biggerInt{extra: -1}
	}
	if y[1] != nil {
		x[1] = biggerInt{i: big.NewInt(0).Set(y[1])}
	} else {
		x[1] = biggerInt{extra: +1}
	}
}

// IntRange is an integer interval. The array elements are the minimum and
// maximum values, inclusive (if non-nil) on both ends. A nil element means
// unbounded: negative or positive infinity.
//
// The zero value (zero in the Go sense, not in the integer sense) is a valid,
// infinitely sized interval, unbounded at both ends.
//
// A subtle point is that an interval's minimum or maximum can be infinite, but
// if an integer value i is known to be within such an interval, i's possible
// values are arbitrarily large but not infinite. Specifically, 0*i is
// unambiguously always equal to 0.
//
// It is also valid for the first element to be greater than the second
// element. This represents an empty interval. There is more than one
// representation of an empty interval.
//
// "Int" abbreviates "Integer" (the same as for big.Int), not "Interval".
// Similarly, we use "Range" instead of "Interval" to avoid unnecessary
// confusion, even though this type is indeed an integer interval.
type IntRange [2]*big.Int

// String returns a string representation of x.
func (x IntRange) String() string {
	if x.Empty() {
		return "<empty>"
	}
	buf := []byte(nil)
	if x[0] == nil {
		buf = append(buf, "(-∞, "...)
	} else {
		buf = append(buf, '[')
		buf = x[0].Append(buf, 10)
		buf = append(buf, ", "...)
	}
	if x[1] == nil {
		buf = append(buf, "+∞)"...)
	} else {
		buf = x[1].Append(buf, 10)
		buf = append(buf, ']')
	}
	return string(buf)
}

// empty returns an empty IntRange: one that contains no elements.
func empty() IntRange {
	return IntRange{big.NewInt(+1), big.NewInt(-1)}
}

// ContainsNegative returns whether x contains at least one negative value.
func (x IntRange) ContainsNegative() bool {
	if x[0] == nil {
		return true
	}
	if x[0].Sign() >= 0 {
		return false
	}
	return x[1] == nil || x[0].Cmp(x[1]) <= 0
}

// ContainsPositive returns whether x contains at least one positive value.
func (x IntRange) ContainsPositive() bool {
	if x[1] == nil {
		return true
	}
	if x[1].Sign() <= 0 {
		return false
	}
	return x[0] == nil || x[0].Cmp(x[1]) <= 0
}

// ContainsZero returns whether x contains zero.
func (x IntRange) ContainsZero() bool {
	return (x[0] == nil || x[0].Sign() <= 0) &&
		(x[1] == nil || x[1].Sign() >= 0)
}

// Eq returns whether x equals y.
func (x IntRange) Eq(y IntRange) bool {
	if xe, ye := x.Empty(), y.Empty(); xe || ye {
		return xe == ye
	}
	if x0, y0 := x[0] != nil, y[0] != nil; x0 != y0 {
		return false
	} else if x0 && x[0].Cmp(y[0]) != 0 {
		return false
	}
	if x1, y1 := x[1] != nil, y[1] != nil; x1 != y1 {
		return false
	} else if x1 && x[1].Cmp(y[1]) != 0 {
		return false
	}
	return true
}

// Empty returns whether x is empty.
func (x IntRange) Empty() bool {
	return x[0] != nil && x[1] != nil && x[0].Cmp(x[1]) > 0
}

// justZero returns whether x is the [0, 0] interval, containing exactly one
// element: the integer zero.
func (x IntRange) justZero() bool {
	return x[0] != nil && x[1] != nil && x[0].Sign() == 0 && x[1].Sign() == 0
}

// split splits x into negative, zero and positive sub-intervals. The IntRange
// values returned may be empty, which means that x does not contain any
// negative or positive elements.
func (x IntRange) split() (neg IntRange, pos IntRange, negEmpty bool, hasZero bool, posEmpty bool) {
	if x[0] != nil && x[0].Sign() > 0 {
		return empty(), x, true, false, x.Empty()
	}
	if x[1] != nil && x[1].Sign() < 0 {
		return x, empty(), x.Empty(), false, true
	}

	neg[0] = x[0]
	neg[1] = big.NewInt(-1)
	if x[1] != nil && x[1].Cmp(neg[1]) < 0 {
		neg[1] = x[1]
	}

	pos[0] = big.NewInt(+1)
	if x[0] != nil && x[0].Cmp(pos[0]) > 0 {
		pos[0] = x[0]
	}
	pos[1] = x[1]

	return neg, pos, neg.Empty(), x.ContainsZero(), pos.Empty()
}

// Add returns z = x + y.
func (x IntRange) Add(y IntRange) (z IntRange) {
	if x.Empty() || y.Empty() {
		return empty()
	}
	if x[0] != nil && y[0] != nil {
		z[0] = big.NewInt(0).Add(x[0], y[0])
	}
	if x[1] != nil && y[1] != nil {
		z[1] = big.NewInt(0).Add(x[1], y[1])
	}
	return z
}

// Sub returns z = x - y.
func (x IntRange) Sub(y IntRange) (z IntRange) {
	if x.Empty() || y.Empty() {
		return empty()
	}
	if x[0] != nil && y[1] != nil && (x[1] != nil || y[0] != nil) {
		z[0] = big.NewInt(0).Sub(x[0], y[1])
	}
	if x[1] != nil && y[0] != nil && (x[0] != nil || y[1] != nil) {
		z[1] = big.NewInt(0).Sub(x[1], y[0])
	}
	return z
}

// Mul returns z = x * y.
func (x IntRange) Mul(y IntRange) (z IntRange) {
	return x.mulLsh(y, false)
}

// Lsh returns z = x << y.
//
// ok is false (and z will be IntRange{nil, nil}) if x is non-empty and y
// contains at least one negative value, as it's invalid to shift by a negative
// number. Otherwise, ok is true.
func (x IntRange) Lsh(y IntRange) (z IntRange, ok bool) {
	if !x.Empty() && y.ContainsNegative() {
		return IntRange{}, false
	}
	return x.mulLsh(y, true), true
}

func (x IntRange) mulLsh(y IntRange, shift bool) (z IntRange) {
	if x.Empty() || y.Empty() {
		return empty()
	}
	if x.justZero() || (!shift && y.justZero()) {
		return IntRange{big.NewInt(0), big.NewInt(0)}
	}

	combine := bigIntMul
	if shift {
		combine = bigIntLsh
	}

	ret := newBiggerIntPair()

	// Split x and y into negative, zero and positive parts.
	negX, posX, negXEmpty, zeroX, posXEmpty := x.split()
	negY, posY, negYEmpty, zeroY, posYEmpty := y.split()

	if zeroY && shift {
		ret.fromIntRange(x)
	} else if (zeroY && !shift) || zeroX {
		ret[0] = biggerInt{i: big.NewInt(0)}
		ret[1] = biggerInt{i: big.NewInt(0)}
	}

	if !negXEmpty {
		if !negYEmpty {
			// x is negative and y is negative, so x op y is positive.
			//
			// If op is << instead of * then we have previously checked that y
			// is non-negative, so this should be unreachable.
			ret.lowerMin(biggerInt{i: combine(negX[1], negY[1])})
			if negX[0] == nil || negY[0] == nil {
				ret.raiseMax(biggerInt{extra: +1})
			} else {
				ret.raiseMax(biggerInt{i: combine(negX[0], negY[0])})
			}
		}

		if !posYEmpty {
			// x is negative and y is positive, so x op y is negative.
			if negX[0] == nil || posY[1] == nil {
				ret.lowerMin(biggerInt{extra: -1})
			} else {
				ret.lowerMin(biggerInt{i: combine(negX[0], posY[1])})
			}
			ret.raiseMax(biggerInt{i: combine(negX[1], posY[0])})
		}
	}

	if !posXEmpty {
		if !negYEmpty {
			// x is positive and y is negative, so x op y is negative.
			//
			// If op is << instead of * then we have previously checked that y
			// is non-negative, so this should be unreachable.
			if posX[1] == nil || negY[0] == nil {
				ret.lowerMin(biggerInt{extra: -1})
			} else {
				ret.lowerMin(biggerInt{i: combine(posX[1], negY[0])})
			}
			ret.raiseMax(biggerInt{i: combine(posX[0], negY[1])})
		}

		if !posYEmpty {
			// x is positive and y is positive, so x op y is positive.
			ret.lowerMin(biggerInt{i: combine(posX[0], posY[0])})
			if posX[1] == nil || posY[1] == nil {
				ret.raiseMax(biggerInt{extra: +1})
			} else {
				ret.raiseMax(biggerInt{i: combine(posX[1], posY[1])})
			}
		}
	}

	return ret.toIntRange()
}

// Quo returns z = x / y. Like the big.Int.Quo method (and unlike the
// big.Int.Div method), it truncates towards zero.
//
// ok is false (and z will be IntRange{nil, nil}) if x is non-empty and y
// contains zero, as it's invalid to divide by zero. Otherwise, ok is true.
func (x IntRange) Quo(y IntRange) (z IntRange, ok bool) {
	if x.Empty() || y.Empty() {
		return empty(), true
	}
	if y.ContainsZero() {
		return IntRange{}, false
	}
	if x.justZero() {
		return IntRange{big.NewInt(0), big.NewInt(0)}, true
	}

	ret := newBiggerIntPair()

	// Split x and y into negative, zero and positive parts.
	negX, posX, negXEmpty, zeroX, posXEmpty := x.split()
	negY, posY, negYEmpty, _, posYEmpty := y.split()

	if zeroX {
		ret[0] = biggerInt{i: big.NewInt(0)}
		ret[1] = biggerInt{i: big.NewInt(0)}
	}

	if !negXEmpty {
		if !negYEmpty {
			// x is negative and y is negative, so x / y is non-negative.
			if negX[0] == nil {
				ret.raiseMax(biggerInt{extra: +1})
			} else {
				ret.raiseMax(biggerInt{i: bigIntQuo(negX[0], negY[1])})
			}
			if negY[0] == nil {
				ret.lowerMin(biggerInt{i: big.NewInt(0)})
			} else {
				ret.lowerMin(biggerInt{i: bigIntQuo(negX[1], negY[0])})
			}
		}

		if !posYEmpty {
			// x is negative and y is positive, so x / y is non-positive.
			if negX[0] == nil {
				ret.lowerMin(biggerInt{extra: -1})
			} else {
				ret.lowerMin(biggerInt{i: bigIntQuo(negX[0], posY[0])})
			}
			if posY[1] == nil {
				ret.raiseMax(biggerInt{i: big.NewInt(0)})
			} else {
				ret.raiseMax(biggerInt{i: bigIntQuo(negX[1], posY[1])})
			}
		}
	}

	if !posXEmpty {
		if !negYEmpty {
			// x is positive and y is negative, so x / y is non-positive.
			if posX[1] == nil {
				ret.lowerMin(biggerInt{extra: -1})
			} else {
				ret.lowerMin(biggerInt{i: bigIntQuo(posX[1], negY[1])})
			}
			if negY[0] == nil {
				ret.raiseMax(biggerInt{i: big.NewInt(0)})
			} else {
				ret.raiseMax(biggerInt{i: bigIntQuo(posX[0], negY[0])})
			}
		}

		if !posYEmpty {
			// x is positive and y is positive, so x / y is non-negative.
			if posX[1] == nil {
				ret.raiseMax(biggerInt{extra: +1})
			} else {
				ret.raiseMax(biggerInt{i: bigIntQuo(posX[1], posY[0])})
			}
			if posY[1] == nil {
				ret.lowerMin(biggerInt{i: big.NewInt(0)})
			} else {
				ret.lowerMin(biggerInt{i: bigIntQuo(posX[0], posY[1])})
			}
		}
	}

	return ret.toIntRange(), true
}

// Rsh returns z = x >> y.
//
// ok is false (and z will be IntRange{nil, nil}) if x is non-empty and y
// contains at least one negative value, as it's invalid to shift by a negative
// number. Otherwise, ok is true.
func (x IntRange) Rsh(y IntRange) (z IntRange, ok bool) {
	if x.Empty() || y.Empty() {
		return empty(), true
	}
	if y.ContainsNegative() {
		return IntRange{}, false
	}
	if x.justZero() {
		return IntRange{big.NewInt(0), big.NewInt(0)}, true
	}

	ret := newBiggerIntPair()

	// Split x and y into negative and zero-or-positive parts.
	negX, posX, negXEmpty, zeroX, posXEmpty := x.split()

	if zeroX {
		ret[0] = biggerInt{i: big.NewInt(0)}
		ret[1] = biggerInt{i: big.NewInt(0)}
	}

	if !negXEmpty {
		// x is negative and y is positive, so x >> y is non-positive.
		if negX[0] == nil {
			ret.lowerMin(biggerInt{extra: -1})
		} else {
			ret.lowerMin(biggerInt{i: bigIntRsh(negX[0], y[0])})
		}
		if y[1] == nil {
			ret.raiseMax(biggerInt{i: big.NewInt(-1)})
		} else {
			ret.raiseMax(biggerInt{i: bigIntRsh(negX[1], y[1])})
		}
	}

	if !posXEmpty {
		// x is positive and y is positive, so x >> y is non-negative.
		if y[1] == nil {
			ret.lowerMin(biggerInt{i: big.NewInt(0)})
		} else {
			ret.lowerMin(biggerInt{i: bigIntRsh(posX[0], y[1])})
		}
		if posX[1] == nil {
			ret.raiseMax(biggerInt{extra: +1})
		} else {
			ret.raiseMax(biggerInt{i: bigIntRsh(posX[1], y[0])})
		}
	}

	return ret.toIntRange(), true
}
