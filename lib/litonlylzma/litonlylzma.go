// Copyright 2022 The Wuffs Authors.
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

// ----------------

// Package litonlylzma provides a decoder and encoder for Literal Only LZMA, a
// subset of the LZMA compressed file format.
//
// Subset means that the Encode method's output is a valid LZMA file and can be
// decompressed by the tools available at https://www.7-zip.org/sdk.html (and
// available as /usr/bin/lzma on a Debian system).
//
// Subset also means that this codec's compression ratios are not as good as
// full-LZMA (although compression times are much faster). Moderate compression
// is still achieved, through range coding, but there are no Lempel Ziv
// back-references. The main benefit, compared to full-LZMA, is that this
// implementation is much simpler and hence easier to study. It is only a few
// hundred lines of code.
//
// Example compression numbers on a small English text file (at
// https://github.com/google/wuffs/blob/main/test/data/romeo.txt):
//  - romeo.txt             is 942 bytes (100%).
//  - romeo.txt.litonlylzma is 659 bytes  (70%).
//  - romeo.txt.xz          is 644 bytes  (68%).
//  - romeo.txt.lzma        is 598 bytes  (63%).
//  - romeo.txt.bz2         is 568 bytes  (60%).
//  - romeo.txt.zst         is 559 bytes  (59%).
//  - romeo.txt.gz          is 558 bytes  (59%).
//
// Example compression numbers on a large archive of source code (at
// https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.0.1.tar.xz):
//  - linux-5.0.1.tar             is 863313920 bytes (100%).
//  - linux-5.0.1.tar.litonlylzma is 449726070 bytes  (52%).
//  - linux-5.0.1.tar.gz          is 164575959 bytes  (19%).
//  - linux-5.0.1.tar.zst         is 156959897 bytes  (18%).
//  - linux-5.0.1.tar.bz2         is 125873134 bytes  (15%).
//  - linux-5.0.1.tar.xz          is 108233572 bytes  (13%).
//  - linux-5.0.1.tar.lzma        is 108216601 bytes  (13%).
//
// The various tools (/usr/bin/gzip, /usr/bin/xz, etc) were all ran with their
// default settings, not their "maximum compression" settings. This is why the
// linux-5.0.1.tar.xz file size above differs from what cdn.kernel.org served.
package litonlylzma

import (
	"errors"
)

var (
	// ErrUnsupportedLZMAData is potentially returned by Decode.
	ErrUnsupportedLZMAData = errors.New("litonlylzma: unsupported LZMA data")

	errInvalidLZMAData       = errors.New("litonlylzma: invalid LZMA data")
	errUnexpectedEOF         = errors.New("litonlylzma: unexpected EOF")
	errUnsupportedFileFormat = errors.New("litonlylzma: unsupported FileFormat")
)

const (
	// LZMA has a few configuration knobs but our subset-of-LZMA hard codes
	// these to LZMA's default values:
	// https://github.com/jljusten/LZMA-SDK/blob/781863cdf592da3e97420f50de5dac056ad352a5/C/LzmaEnc.h#L19-L21
	// https://github.com/jljusten/LZMA-SDK/blob/781863cdf592da3e97420f50de5dac056ad352a5/DOC/lzma-specification.txt#L43-L45
	lc = 3 // The number of Literal Context bits.
	lp = 0 // The number of Literal Position bits.
	pb = 2 // The number of Position Bits.

	lpMask = (1 << lp) - 1
	pbMask = (1 << pb) - 1
)

// lzmaHeader5 is the first 5 bytes of the LZMA header using the hard-coded
// configuration for the subset-of-LZMA that this package supports. 0x5D
// encodes the (lc, lp, pb) triple and the next four bytes hold the u32le
// dictionary size. 0x1000 is LZMA's minimum dictionary size, although our
// subset-of-LZMA does not use a dictionary (also called a "sliding window").
const lzmaHeader5 = "\x5D\x00\x10\x00\x00"

// rangeDecoder and rangeEncoder hold the state for range coding (also known as
// arithmetic coding, roughly speaking). "range" is a keyword in Go, so in this
// implementation, what's traditionally called the "range" fields are called
// "width" instead.
//
// Conceptually, the encoded bitstream holds a high precision number (a binary
// fraction) in the range from zero to one. High precision could mean thousands
// or millions of bits, but when decoding, only 32 of these are 'paged in' at a
// time - this is the rangeDecoder.bits field. The encoder similarly works on
// only a small, fixed number of bits.
//
// When encoding, that high precision *number* isn't fully known until the end
// of the input is reached. *During* the encoding process, what's known is a
// high precision *interval* that gets narrowed down as the input bytes are
// consumed. That interval is represented by the rangeEncoder.low and
// rangeEncoder.width fields (the width is the difference between the
// interval's upper and lower bound). Both low and width are, conceptually,
// 32-bit values - they operate at the same scale - but low is an uint64
// because of potential overflow. We'll revisit that below.
//
// Width is initialized to 0xFFFF_FFFF and shrinks every time a bit is decoded
// or encoded. Similarly, low can only increase or stay the same on each bit
// step. When width drops below (1 << 24), the rangeDecoder or rangeEncoder
// 'zooms in', left-shifting (with truncation) by 8 the bits, low and width
// fields and reading a src byte or writing a dst byte of compressed data.
//
// Writing a byte may be delayed because the encoding interval could span
// multiple possible digits. To use a decimal fraction analogy, suppose that
// our interval is [0.7138872, 0.7146055] and the low field nominally holds 4
// digits. We could emit '7' and '1' but we have to hold back on emitting
// either '3' or '4' until we can narrow down the interval further. That '3'
// digit is stashed as the pendingHead field. low is set to 8872, width is set
// to (46055 - 38872) = 7183 and we progress until the width drops below 1000,
// causing a 'zoom in' event. There are three cases:
//  - if low is less than 9000 (but starts with an '8'), we can emit the '3'
//    with confidence (and then stash '8' in the pendingHead field).
//  - if low overflows 9999, we can emit the '4' with confidence (and then stash
//    low's 'thousands digit' in the pendingHead field).
//  - if low is in 9000 ..= 9999 then we could still be unsure. Keep the '3'
//    pending but extend it so that there are two pending digits: '39'. If this
//    occurs again: '399', and so on.
//
// In all three cases, 'zooming in' causes us to shift out the most significant
// digit of the low field. This method is therefore named shiftLow here, the
// same name used in other range coding implementations, such as
// https://github.com/jljusten/LZMA-SDK/blob/781863cdf592da3e97420f50de5dac056ad352a5/C/LzmaEnc.c#L572-L602
//
// The pendingEtc fields combine to hold one or more digits ('0' ..= '9'
// decimal digits in our analogy, 0x00 ..= 0xFF bytes in our actual
// implementation). There are (1 + pendingExtra) digits in total and while the
// head digit might be any value, the extra digits must be all '9' (in the
// decimal analogy) or 0xFF (in our actual implementation).
//
// Some other implementations use the name "cache" instead of "pending", and
// store a "cacheSize" field equivalent to our (1 + pendingExtra).
//
// As above, the low field is conceptually a uint32, but it is a uint64 because
// of this potential overflow. Its high 32 bits are usually zero, rarely one
// and never anything higher. The high 32 bits essentially hold an overflow
// carry bit. If set, it flips "3999etc" to "4000etc".
//
// "Low is conceptually 4 bytes; pendingHead holds another byte" is why the
// shortest LZMA payload is 5 bytes even though the first byte is always 0x00
// and seems redundant.

type rangeDecoder struct {
	src   []byte
	bits  uint32
	width uint32
}

type rangeEncoder struct {
	dst          []byte
	low          uint64
	width        uint32
	pendingHead  uint8
	pendingExtra uint64
}

// Precondition: (rEnc.width < (1 << 24)) or we are at the end of our input
// (and are flushing the rangeEncoder).
func (rEnc *rangeEncoder) shiftLow() {
	if rEnc.low < 0x0_FF00_0000 {
		rEnc.dst = append(rEnc.dst, rEnc.pendingHead+0x00)
		for ; rEnc.pendingExtra > 0; rEnc.pendingExtra-- {
			rEnc.dst = append(rEnc.dst, 0xFF)
		}
		rEnc.pendingHead = uint8(rEnc.low >> 24)
		rEnc.pendingExtra = 0
		rEnc.low = (rEnc.low << 8) & 0xFFFF_FFFF

	} else if rEnc.low < 0x1_0000_0000 {
		rEnc.pendingExtra++
		rEnc.low = (rEnc.low << 8) & 0xFFFF_FFFF

	} else {
		rEnc.dst = append(rEnc.dst, rEnc.pendingHead+0x01)
		for ; rEnc.pendingExtra > 0; rEnc.pendingExtra-- {
			rEnc.dst = append(rEnc.dst, 0x00)
		}
		rEnc.pendingHead = uint8(rEnc.low >> 24)
		rEnc.pendingExtra = 0
		rEnc.low = (rEnc.low << 8) & 0xFFFF_FFFF
	}
}

// prob is a probability (out of 100%) whose uint16 value ranges from 0 to
// (1 << 11) = 2048 inclusive. For example, prob(1024) means a 50% probability
// and prob(384) means a (384 / 2048) = 18.75% probability.
//
// Specifically, it is the probability that the next bit is 0 (instead of 1).
type prob uint16

const (
	// probBits is such that prob(1 << probBits) means a 100% probability.
	probBits = 11
	minProb  = 0
	maxProb  = 1 << probBits
)

func setProbsToOneHalf(p []prob) {
	for i := range p {
		p[i] = 1 << (probBits - 1)
	}
}

// adaptShift scales how much to adjust "the probability that the next bit is
// 0" after seeing the current bit. The probability is adaptive in that the
// current bit being 0 (or 1) increases (or decreases) the probability that the
// next bit is 0. The larger adaptShift is, the slower the adaptation.
const adaptShift = 5

func (p *prob) decodeBit(rDec *rangeDecoder) (bitValue uint32, retErr error) {
	threshold := (rDec.width >> probBits) * uint32(*p)
	if rDec.bits < threshold {
		bitValue = 0
		rDec.width = threshold
		*p += (maxProb - *p) >> adaptShift
	} else {
		bitValue = 1
		rDec.bits -= threshold
		rDec.width -= threshold
		*p -= (*p - minProb) >> adaptShift
	}

	if rDec.width < (1 << 24) {
		if len(rDec.src) <= 0 {
			return 0, errUnexpectedEOF
		}
		rDec.bits = rDec.bits<<8 | uint32(rDec.src[0])
		rDec.width <<= 8
		rDec.src = rDec.src[1:]
	}
	return bitValue, retErr
}

func (p *prob) encodeBit(rEnc *rangeEncoder, bitValue uint32) {
	threshold := (rEnc.width >> probBits) * uint32(*p)
	if bitValue == 0 {
		rEnc.width = threshold
		*p += (maxProb - *p) >> adaptShift
	} else {
		rEnc.low += uint64(threshold)
		rEnc.width -= threshold
		*p -= (*p - minProb) >> adaptShift
	}
	if rEnc.width < (1 << 24) {
		rEnc.width <<= 8
		rEnc.shiftLow()
	}
}

// byteProbs hold probabilities for coding 8 bits (1 byte) of data. It is a 256
// element array such that:
//  - The 0th element is unused.
//  - The 1st element holds the probability that the 7th bit (the highest, most
//    significant bit) is 0.
//  - The 2nd element holds the probability that the 6th bit is 0, conditional
//    on the high bit being 0.
//  - The 3rd element holds the probability that the 6th bit is 0, conditional
//    on the high bit being 1.
//  - The 4th element holds the probability that the 5th bit is 0, conditional
//    on the high two bits being 00.
//  - The 5th element holds the probability that the 5th bit is 0, conditional
//    on the high two bits being 01.
//  - The 6th element holds the probability that the 5th bit is 0, conditional
//    on the high two bits being 10.
//  - The 7th element holds the probability that the 5th bit is 0, conditional
//    on the high two bits being 11.
//  - The 8th element holds the probability that the 4th bit is 0, conditional
//    on the high three bits being 000.
//  - etc
//  - The 255th element holds the probability that the 0th bit (the lowest,
//    least significant bit) is 0, conditional on the high seven bits being
//    1111111.
//
// Put another way, the 256 elements' value of N, as in "it's a probability for
// the Nth bit", looks like this (when arranged in 8 rows of 32 elements):
//
//   u7665555444444443333333333333333
//   22222222222222222222222222222222
//   11111111111111111111111111111111
//   11111111111111111111111111111111
//   00000000000000000000000000000000
//   00000000000000000000000000000000
//   00000000000000000000000000000000
//   00000000000000000000000000000000
//
// The 'u' means that the 0th element is unused.
type byteProbs [0x100]prob

func (p *byteProbs) decodeByte(rDec *rangeDecoder) (byteValue byte, retErr error) {
	index := uint32(1)
	for index < 0x100 {
		if bitValue, err := p[index].decodeBit(rDec); err != nil {
			return 0, err
		} else {
			index = index<<1 | bitValue
		}
	}
	return byte(index), nil
}

func (p *byteProbs) encodeByte(rEnc *rangeEncoder, byteValue byte) {
	b := uint32(byteValue)
	index := uint32(1)
	for i := 7; i >= 0; i-- {
		bitValue := (b >> uint(i)) & 1
		p[index].encodeBit(rEnc, bitValue)
		index = index<<1 | bitValue
	}
}

// FileFormat is a compressed file format, either the original LZMA format
// itself or something that builds on or varies that.
//
// Each valid FileFormatXxx value does not distinguish between the full file
// format (as spoken by numerous Xxx software tools) and the subset-of-Xxx
// implemented by this package. Specifically, FileFormatLZMA.Encode will
// produce compressed data that is valid full-LZMA (and is also valid
// subset-of-LZMA). FileFormatLZMA.Decode can decode subset-of-LZMA (but not
// full-LZMA).
//
// This package currently defines only one valid FileFormatXxx value:
// FileFormatLZMA. Future versions of this package could define e.g.
// FileFormatLZMA2 or FileFormatXz such that FileFormatXz.Encode would produce
// valid full-XZ data: a valid XZ file (that could be decoded by official XZ
// tools) that used the LZMA compression filter, but elected not to use the
// full capabilities of that filter (using LZ literals only, not matches).
type FileFormat uint32

const (
	FileFormatInvalid = FileFormat(0)
	FileFormatLZMA    = FileFormat(1)
)

// Decode converts src from the Literal Only LZMA compressed file format,
// appending the decoding to dst.
//
// f should be FileFormatLZMA but future versions of this package may support
// other file formats.
//
// It is valid to pass a nil dst, like the way it's valid to pass nil to the
// built-in append function.
//
// Decode also returns the suffix of src that was not used during decoding.
//
// It can also return partial results. The appendedDst and remainingSrc return
// values may be non-zero even if retErr is non-zero.
//
// It returns ErrUnsupportedLZMAData if the source bytes look like LZMA
// formatted data that is outside the subset-of-LZMA that this package
// implements.
func (f FileFormat) Decode(dst []byte, src []byte) (appendedDst []byte, remainingSrc []byte, retErr error) {
	if f != FileFormatLZMA {
		return dst, nil, errUnsupportedFileFormat
	}

	// LZMA consists of a 13 byte header and then at least a 5 byte payload
	// (range coded data). The 13 byte header is:
	//  - 1 byte for the (lc, lp, pb) triple with max values (8, 4, 4),
	//  - 4 bytes u32le dictionary size and
	//  - 8 bytes i64le uncompressed size.
	if (len(src) < 18) || (src[0] >= (9 * 5 * 5)) {
		return dst, nil, errInvalidLZMAData
	} else if (src[0] != lzmaHeader5[0]) ||
		(src[1] != lzmaHeader5[1]) ||
		(src[2] != lzmaHeader5[2]) ||
		(src[3] != lzmaHeader5[3]) ||
		(src[4] != lzmaHeader5[4]) ||
		(src[13] != 0x00) {
		return dst, nil, ErrUnsupportedLZMAData
	}

	size := uint64(0)
	for i := 0; i < 8; i++ {
		size |= uint64(src[5+i]) << uint(8*i)
	}
	if int64(size) < -1 {
		return dst, nil, errInvalidLZMAData
	} else if int64(size) == -1 {
		return dst, nil, ErrUnsupportedLZMAData
	}

	rDec := rangeDecoder{
		src: src[18:],
		bits: (uint32(src[14]) << 24) |
			(uint32(src[15]) << 16) |
			(uint32(src[16]) << 8) |
			(uint32(src[17]) << 0),
		width: 0xFFFF_FFFF,
	}

	posProbs := [1 << pb]prob{}
	setProbsToOneHalf(posProbs[:])

	litProbs := [1 << (lc + lp)]byteProbs{}
	for ij := range litProbs {
		setProbsToOneHalf(litProbs[ij][:])
	}

	pos := uint32(0)
	prev := byte(0)
	curr := byte(0)
	for ; size > 0; size-- {
		if bitValue, err := posProbs[pos&pbMask].decodeBit(&rDec); err != nil {
			return dst, nil, err
		} else if bitValue != 0 {
			// A full-LZMA decoder would implement Lempel Ziv matches (or the
			// End of Stream marker) here. Our simpler subset-of-LZMA decoder
			// only uses literals.
			return dst, rDec.src, ErrUnsupportedLZMAData
		}

		i := (pos & lpMask) << lc
		j := uint32(prev) >> (8 - lc)
		if curr, retErr = litProbs[i|j].decodeByte(&rDec); retErr != nil {
			return dst, rDec.src, retErr
		}
		dst = append(dst, curr)
		pos++
		prev = curr
	}

	return dst, rDec.src, nil
}

// Encode converts src to the Literal Only LZMA compressed file format,
// appending the encoding to dst.
//
// f should be FileFormatLZMA but future versions of this package may support
// other file formats.
//
// It is valid to pass a nil dst, like the way it's valid to pass nil to the
// built-in append function.
func (f FileFormat) Encode(dst []byte, src []byte) (appendedDst []byte, retErr error) {
	if f != FileFormatLZMA {
		return nil, errUnsupportedFileFormat
	}

	dst = append(dst, lzmaHeader5...)
	size := len(src)
	for i := 0; i < 8; i++ {
		dst = append(dst, byte(size))
		size >>= 8
	}

	rEnc := rangeEncoder{
		dst:   dst,
		width: 0xFFFF_FFFF,
	}

	posProbs := [1 << pb]prob{}
	setProbsToOneHalf(posProbs[:])

	litProbs := [1 << (lc + lp)]byteProbs{}
	for ij := range litProbs {
		setProbsToOneHalf(litProbs[ij][:])
	}

	pos := uint32(0)
	prev := byte(0)
	for _, curr := range src {
		// A full-LZMA encoder would choose between Lempel Ziv literals and
		// matches. Our simpler subset-of-LZMA encoder only uses literals, and
		// it is not a streaming encoder (it does not need to emit an End of
		// Stream marker), so we always encode a 0 bit here.
		posProbs[pos&pbMask].encodeBit(&rEnc, 0)

		i := (pos & lpMask) << lc
		j := uint32(prev) >> (8 - lc)
		litProbs[i|j].encodeByte(&rEnc, curr)
		pos++
		prev = curr
	}

	// Flush the rangeEncoder.
	for i := 0; i < 5; i++ {
		rEnc.shiftLow()
	}

	return rEnc.dst, nil
}
