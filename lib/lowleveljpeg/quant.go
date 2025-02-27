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

// QuantizationStandardValuesType selects which table from the JPEG
// specification to use for SetToStandardValues.
type QuantizationStandardValuesType byte

const (
	// QuantizationStandardValuesTypeLuma means to use table K.1 from the JPEG
	// spec.
	QuantizationStandardValuesTypeLuma = QuantizationStandardValuesType(0)

	// QuantizationStandardValuesTypeChroma means to use table K.2 from the
	// JPEG spec.
	QuantizationStandardValuesTypeChroma = QuantizationStandardValuesType(1)
)

// 12-bit or 16-bit depth JPEGs can use more-than-8-bit quantization factors,
// but we only support 8-bit JPEGs and so we use 8-bit quantization factors.
// The QuantizationFactors element type is uint8.

// QuantizationFactors is an 8×8 block of uint8 values, used to quantize a
// BlockI16.
//
// It is indexed in DCT (not XY) space, like a Block16.
type QuantizationFactors [64]uint8

// String returns b in human-readable form.
func (b QuantizationFactors) String() string {
	buf := [64 * 3]byte{}
	for i, value := range b {
		buf[(3*i)+0] = fmtHex[15&(value>>0x04)]
		buf[(3*i)+1] = fmtHex[15&(value>>0x00)]
		buf[(3*i)+2] = fmtSep[7&i]
	}
	return string(buf[:])
}

// IsValid returns whether b does not contain any zero-valued elements.
func (b *QuantizationFactors) IsValid() bool {
	if b == nil {
		return false
	}
	for _, v := range b {
		if v == 0 {
			return false
		}
	}
	return true
}

const (
	// MinimumQuality and MaximumQuality are the minimum and maximum
	// (inclusive) values for SetToStandardValues' quality parameter.
	MinimumQuality = 1
	MaximumQuality = 100

	// DefaultQuality is the default value used by EncoderOptions for
	// SetToStandardValues' quality parameter. It matches libjpeg's default
	// both in terms of numerical value and in behavior.
	DefaultQuality = 75

	// MinimumBaselineQuality is lowest quality level where both the cjpeg
	// program (from the libjpeg C project) and this Go package will agree on
	// the exact quantization factors.
	//
	// It's still valid to pass a SetToStandardValues quality parameter value
	// below 24. But when doing so, passing the equivalent to cjpeg will print
	// a JTRC_16BIT_TABLES warning: "quantization tables are too coarse for
	// baseline JPEG". cjpeg will therefore produce extended (instead of
	// baseline) JPEGs, using 2 bytes (instead of 1 byte) per quantization
	// factor, allowing factors above 0xFF.
	//
	// This Go package will instead clamp such quantization factors to 0xFF.
	//
	// Both approaches are viable, producing valid JPEGs. This Go package, like
	// the Go standard library's image/jpeg package, simply chooses to always
	// produce baseline (instead of extended) JPEGs, for best compatibility
	// with other JPEG decoders.
	MinimumBaselineQuality = 24
)

// SetToStandardValues sets b to one of two standard quantization tables
// (Tables K.1 or K.2 in the JPEG spec, for Luma and Chroma respectively),
// scaled by a quality parameter with the same meaning as that used by
// libjpeg's encoder.
//
// The quality parameter ranges from 1 (low quality) to 100 (high quality),
// inclusive. Values outside of that range will be clamped to be in [1, 100].
//
// Passing quality = MinimumQuality will set every element of b to 0xFF.
//
// Passing quality = MaximumQuality will set every element of b to 0x01.
func (b *QuantizationFactors) SetToStandardValues(which QuantizationStandardValuesType, quality int) {
	if b == nil {
		return
	}

	// Follow the same algorithm as in libjpeg's jcparam.c.

	if quality < 1 {
		quality = 1
	} else if quality > 100 {
		quality = 100
	}

	q := 0
	if quality < 50 {
		q = 5000 / quality
	} else {
		q = 200 - (quality * 2)
	}

	std := &standardQuantizationFactors[which&1]
	for i, v := range std {
		scaled := ((int(v) * q) + 50) / 100
		if scaled < 0x01 {
			scaled = 0x01
		} else if scaled > 0xFF {
			scaled = 0xFF
		}
		b[i] = uint8(scaled)
	}
}

var standardQuantizationFactors = [2]QuantizationFactors{
	// Table K.1 from the JPEG spec.
	QuantizationStandardValuesTypeLuma: {
		16, 11, 10, 16, 24, 40, 51, 61,
		12, 12, 14, 19, 26, 58, 60, 55,
		14, 13, 16, 24, 40, 57, 69, 56,
		14, 17, 22, 29, 51, 87, 80, 62,
		18, 22, 37, 56, 68, 109, 103, 77,
		24, 35, 55, 64, 81, 104, 113, 92,
		49, 64, 78, 87, 103, 121, 120, 101,
		72, 92, 95, 98, 112, 100, 103, 99,
	},

	// Table K.2 from the JPEG spec.
	QuantizationStandardValuesTypeChroma: {
		17, 18, 24, 47, 99, 99, 99, 99,
		18, 21, 26, 66, 99, 99, 99, 99,
		24, 26, 56, 99, 99, 99, 99, 99,
		47, 66, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
	},
}
