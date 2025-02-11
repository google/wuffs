// Copyright 2025 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

package lowleveljpeg

// ArrayNBlockT are arrays of N BlockT values. N is the number of blocks in a
// JPEG MCU (Mininum Coded Unit).
//
//   - N = 1 is for ColorTypeGray.
//   - N = 3 is for ColorTypeYCbCr444.
//   - N = 6 is for ColorTypeYCbCr420.
type (
	Array1BlockU8  [1]BlockU8
	Array1BlockI16 [1]BlockI16
	Array3BlockU8  [3]BlockU8
	Array3BlockI16 [3]BlockI16
	Array6BlockU8  [6]BlockU8
	Array6BlockI16 [6]BlockI16
)

// SetToNeutral calls SetToNeutral on each element.
func (b *Array1BlockU8) SetToNeutral() {
	if b == nil {
		return
	}
	for i := range b {
		b[i].SetToNeutral()
	}
}

// SetToNeutral calls SetToNeutral on each element.
func (b *Array1BlockI16) SetToNeutral() {
	if b == nil {
		return
	}
	for i := range b {
		b[i].SetToNeutral()
	}
}

// SetToNeutral calls SetToNeutral on each element.
func (b *Array3BlockU8) SetToNeutral() {
	if b == nil {
		return
	}
	for i := range b {
		b[i].SetToNeutral()
	}
}

// SetToNeutral calls SetToNeutral on each element.
func (b *Array3BlockI16) SetToNeutral() {
	if b == nil {
		return
	}
	for i := range b {
		b[i].SetToNeutral()
	}
}

// SetToNeutral calls SetToNeutral on each element.
func (b *Array6BlockU8) SetToNeutral() {
	if b == nil {
		return
	}
	for i := range b {
		b[i].SetToNeutral()
	}
}

// SetToNeutral calls SetToNeutral on each element.
func (b *Array6BlockI16) SetToNeutral() {
	if b == nil {
		return
	}
	for i := range b {
		b[i].SetToNeutral()
	}
}

// InverseDCTFrom calls InverseDCTFrom pairwise on dst and src elements.
func (dst *Array1BlockU8) InverseDCTFrom(src *Array1BlockI16) {
	if dst == nil {
		return
	}
	for i := range dst {
		dst[i].InverseDCTFrom(&src[i])
	}
}

// ForwardDCTFrom calls ForwardDCTFrom pairwise on dst and src elements.
func (dst *Array1BlockI16) ForwardDCTFrom(src *Array1BlockU8) {
	if dst == nil {
		return
	}
	for i := range dst {
		dst[i].ForwardDCTFrom(&src[i])
	}
}

// InverseDCTFrom calls InverseDCTFrom pairwise on dst and src elements.
func (dst *Array3BlockU8) InverseDCTFrom(src *Array3BlockI16) {
	if dst == nil {
		return
	}
	for i := range dst {
		dst[i].InverseDCTFrom(&src[i])
	}
}

// ForwardDCTFrom calls ForwardDCTFrom pairwise on dst and src elements.
func (dst *Array3BlockI16) ForwardDCTFrom(src *Array3BlockU8) {
	if dst == nil {
		return
	}
	for i := range dst {
		dst[i].ForwardDCTFrom(&src[i])
	}
}

// InverseDCTFrom calls InverseDCTFrom pairwise on dst and src elements.
func (dst *Array6BlockU8) InverseDCTFrom(src *Array6BlockI16) {
	if dst == nil {
		return
	}
	for i := range dst {
		dst[i].InverseDCTFrom(&src[i])
	}
}

// ForwardDCTFrom calls ForwardDCTFrom pairwise on dst and src elements.
func (dst *Array6BlockI16) ForwardDCTFrom(src *Array6BlockU8) {
	if dst == nil {
		return
	}
	for i := range dst {
		dst[i].ForwardDCTFrom(&src[i])
	}
}

// Array2QuantizationFactors is an array of 2 QuantizationFactors. The first
// one is for Luma and the second one is for Chroma.
type Array2QuantizationFactors [2]QuantizationFactors

// SetToStandardValues calls SetToStandardValues on each element.
func (b *Array2QuantizationFactors) SetToStandardValues(quality int) {
	if b == nil {
		return
	}
	b[0].SetToStandardValues(0, quality)
	b[1].SetToStandardValues(1, quality)
}
