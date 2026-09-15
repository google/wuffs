// Copyright 2026 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

// Package suitar implements the SUITAR archive file format.
//
// SUITAR is a subset of the TAR archive file format (including its popular
// USTAR, PAX and GNU extensions). This package's API is a subset of the
// standard library's archive/tar package.
//
// Being a subset means that this package's implementation is about 600 lines
// of Go code, compared to archive/tar being about 3000, a factor of 5×.
//
// The SUITAR specification is at
// https://github.com/google/wuffs/blob/main/doc/spec/suitar-spec.md
package suitar

import (
	"errors"
	"io"
	"strings"
	"time"
	"unicode/utf8"
)

var (
	errBadFileName    = errors.New("suitar: bad file name")
	errBadHeader      = errors.New("suitar: bad header")
	errBadPadding     = errors.New("suitar: bad padding")
	errClosed         = errors.New("suitar: closed")
	errHeaderSize     = errors.New("suitar: inconsistent Header.Size and Write length")
	errHeaderTypeflag = errors.New("suitar: inconsistent Header.Typeflag for Write")
)

// lenMagic makes headerBlockTemplate[:lenMagic] SUITAR's magic signature.
const lenMagic = 12

// headerBlockTemplate is the contents of each 512-byte header block. Each
// SUITAR archive entry (a file or directory) consists of: 1 header block (with
// typeflag typeGNULongName), the file name, 1 header block (with typeflag
// TypeReg or TypeDir or TypeGNUSparse) and the file contents.
//
// '?' bytes mean that the header block's bytes can vary at that position.
// Otherwise, the bytes are hard-coded. SUITAR is a stricter subset of TAR.
const headerBlockTemplate = "\x13sUItAR\x00\xFE\xFDv1\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x000000???\x000177" + // ??? = mode bits, 0177776 = uid("nobody").
	"776\x000177776\x00\x80\x00\x00\x00" +

	"????????\x80\x00\x00\x00????" + // ???????? = physical file size, ???????? = modTime.
	"??????????\x00 ?\x00\x00\x00" + // ?????? = checksum, ? = typeflag.
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +

	"\x00ustar  \x00nobody\x00" + // "ustar  " mid-block magic signature, "nobody" ≈ uid.
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00nobody\x00" + // "nobody" ≈ gid.
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +

	// The 128-byte tail of this headerBlockTemplate only applies to
	// TypeGNUSparse blocks. Otherwise, the final 128 bytes must be NUL.
	"\x00\x00\x80\x00\x00\x00????????\x80\x00" + // ???????? = sparse anti-hole offset (and its length is 0).
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" +
	"\x00\x00\x00\x80\x00\x00\x00????????\x00" + // ???????? = sparse logical file size.
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

// Valid values for Header.Typeflag. The zero value is invalid.
const (
	TypeReg       = '0' // Regular file (with contents).
	TypeDir       = '5' // Directory (with no contents).
	TypeGNUSparse = 'S' // Sparse file (its contents are all NUL bytes).

	typeGNULongName = 'L'
)

// Valid values for Header.Mode. The zero value is invalid.
const (
	Mode644 = int64(0o644) // "rw-r--r--" mode bits, also known as permission bits.
	Mode755 = int64(0o755) // "rwxr-xr-x" mode bits, also known as permission bits.
)

// IsValidHeaderName returns whether name is a valid Header.Name field value.
func IsValidHeaderName(name string) bool {
	return (len(name) < 4096) &&
		(name != "") &&
		(name != ".") &&
		(name != "..") &&
		(name[0] != '/') &&
		(name[len(name)-1] != '/') &&
		!strings.HasPrefix(name, "./") &&
		!strings.HasPrefix(name, "../") &&
		!strings.Contains(name, "//") &&
		!strings.Contains(name, "/./") &&
		!strings.Contains(name, "/../") &&
		!containsASCIIControlCharactersOrDel(name) &&
		utf8.ValidString(name)
}

func containsASCIIControlCharactersOrDel(s string) bool {
	for i := range len(s) {
		if c := s[i]; (c < 0x20) || (c == 0x7F) {
			return true
		}
	}
	return false
}

func isAllZeroes(b []byte) bool {
	for _, c := range b {
		if c != 0 {
			return false
		}
	}
	return true
}

func readFullNoEOF(r io.Reader, b []byte) (int, error) {
	n, err := io.ReadFull(r, b)
	if err == io.EOF {
		err = io.ErrUnexpectedEOF
	}
	return n, err
}

func u64le(b []byte) uint64 {
	return (uint64(b[0]) << 56) |
		(uint64(b[1]) << 48) |
		(uint64(b[2]) << 40) |
		(uint64(b[3]) << 32) |
		(uint64(b[4]) << 24) |
		(uint64(b[5]) << 16) |
		(uint64(b[6]) << 8) |
		(uint64(b[7]) << 0)
}

func roundUp512(n uint64) uint64 {
	return (n + 511) &^ 511
}

type block [512]byte

func (b *block) calculateChecksum() uint32 {
	checksum := uint32(0)

	for _, c := range b[0x000:0x094] {
		checksum += uint32(c)
	}

	// The 8 checksum bytes have value 32 when calculating the checksum itself.
	checksum += 256

	for _, c := range b[0x09C:0x200] {
		checksum += uint32(c)
	}

	return checksum
}

func (b *block) setChecksum() {
	checksum := b.calculateChecksum()

	b[0x09B] = ' '
	b[0x09A] = '\x00'
	for i := range 6 {
		b[0x099-i] = '0' + byte(checksum&7)
		checksum >>= 3
	}
}

func (b *block) isValidHeader(which int) bool {
	// Compare b to the headerBlockTemplate. Blocks that aren't TypeGNUSparse
	// must end in 128 NUL bytes.
	end := 0x200
	if typeflag := b[0x09C]; typeflag != TypeGNUSparse {
		end = 0x180
	}
	for i := range end {
		if c := headerBlockTemplate[i]; (c != '?') && (c != b[i]) {
			return false
		}
	}
	if (end == 0x180) && !isAllZeroes(b[0x180:0x200]) {
		return false
	}

	// Check the mode bits.
	if (b[0x0068] == '6') && (b[0x0069] == '4') && (b[0x006A] == '4') {
		// No-op.
	} else if (b[0x0068] == '7') && (b[0x0069] == '5') && (b[0x006A] == '5') && (which == 1) {
		// No-op.
	} else {
		return false
	}

	// Check the checksum.
	checksum := b.calculateChecksum()
	for i := range 6 {
		if b[0x099-i] != ('0' + byte(checksum&7)) {
			return false
		}
		checksum >>= 3
	}

	return true
}

// Header is a single header in a SUITAR archive.
//
// It is a subset of tar.Header from the standard library's archive/tar
// package, but it is stricter about what field values are valid.
type Header struct {
	Typeflag byte
	Name     string
	Size     int64
	Mode     int64
	ModTime  time.Time
}

// Valid returns whether h is valid to pass to a Writer. Specifically:
//
//   - Typeflag must be one of three values (TypeReg, TypeDir, TypeGNUSparse).
//     In particular, it cannot be zero.
//   - Name must satisfy IsValidHeaderName.
//   - Size must be non-negative. It must be 0 if Typeflag is TypeDir.
//   - Mode must be one of two values (Mode644, Mode755). It must be Mode755 if
//     Typeflag is TypeDir.
//   - ModTime.Unix() must be a non-negative int64. In particular, ModTime must
//     be on or after 1 January 1970 and so cannot be the zero time.Time (which
//     is 1 January Year-1).
//   - Size and ModTime.Unix() must also be less than (1 << 53).
//
// A Header returned (without error) by a Reader will always be valid.
func (h *Header) Valid() bool {
	if h == nil {
		return false
	}

	switch h.Typeflag {
	default:
		return false
	case TypeDir:
		if (h.Size != 0) || (h.Mode != Mode755) {
			return false
		}
	case TypeReg, TypeGNUSparse:
		if (h.Mode != Mode644) && (h.Mode != Mode755) {
			return false
		}
	}

	const maxExcl = 1 << 53
	m := h.ModTime.Unix()
	return (0 <= h.Size) && (h.Size < maxExcl) &&
		(0 <= m) && (m < maxExcl) &&
		IsValidHeaderName(h.Name)
}

// NewWriter creates a new Writer writing to w.
func NewWriter(w io.Writer) *Writer {
	return &Writer{w: w}
}

// Writer provides sequential writing of a SUITAR archive.
type Writer struct {
	err       error
	w         io.Writer
	header    Header
	remaining int64
	bIndex    int32
	block     block
	nameBuf   [4096]byte
}

func (w *Writer) flush() error {
	if w.err != nil {
		return w.err
	} else if w.remaining != 0 {
		w.err = errHeaderSize
		return w.err
	} else if w.bIndex != 0 {
		clear(w.block[w.bIndex:])
		if _, err := w.w.Write(w.block[:]); err != nil {
			w.err = err
			return w.err
		}
		w.bIndex = 0
	}

	return nil
}

// WriteHeader writes h and prepares to accept the file's contents (as w is
// also an io.Writer).
func (w *Writer) WriteHeader(h *Header) error {
	if err := w.flush(); err != nil {
		return err
	} else if !h.Valid() {
		w.err = errBadHeader
		return w.err
	}
	w.header = *h

	n := len(w.header.Name)
	copy(w.nameBuf[:], w.header.Name)
	w.nameBuf[n] = 0

	initBlock(&w.block, typeGNULongName, int64(n+1), Mode644, 0)
	if _, err := w.w.Write(w.block[:]); err != nil {
		w.err = err
		return w.err
	}

	w.remaining = int64(n + 1)

	if _, err := w.Write(w.nameBuf[:n+1]); err != nil {
		w.err = err
		return w.err
	} else if err = w.flush(); err != nil {
		w.err = err
		return w.err
	}

	initBlock(&w.block, w.header.Typeflag, w.header.Size, w.header.Mode, w.header.ModTime.Unix())
	if _, err := w.w.Write(w.block[:]); err != nil {
		w.err = err
		return w.err
	}

	w.remaining = w.header.Size
	if w.header.Typeflag == TypeGNUSparse {
		w.remaining = 0
	}

	return nil
}

func initBlock(b *block, typeflag byte, size int64, mode int64, modTime int64) {
	// 0o0177776 octal is 65534, which is Debian's UID/GID for "nobody".
	const uidForNobody = "0177776"

	clear(b[:])
	copy(b[:], headerBlockTemplate[:lenMagic])

	modeBits := "0000644"
	if mode == Mode755 {
		modeBits = "0000755"
	}
	copy(b[0x064:], modeBits)
	copy(b[0x06C:], uidForNobody)
	copy(b[0x074:], uidForNobody)

	physicalSize := size
	if typeflag == TypeGNUSparse {
		physicalSize = 0
	}
	setI64(b, 0x07C, physicalSize)

	setI64(b, 0x088, modTime)
	b[0x09C] = typeflag
	copy(b[0x101:], "ustar  ")
	copy(b[0x109:], "nobody")
	copy(b[0x129:], "nobody")

	if typeflag == TypeGNUSparse {
		setI64(b, 0x182, size)
		setI64(b, 0x18E, 0)
		setI64(b, 0x1E3, size)
	}

	b.setChecksum()
}

func setI64(b *block, offset int, value int64) {
	// The 0x80 uses base-256 instead of octal, allowing file sizes >= 8GiB.
	b[offset] = 0x80
	for i := range 8 {
		b[(offset+11)-i] = byte(value)
		value >>= 8
	}
}

// Write satisfies io.Writer.
func (w *Writer) Write(b []byte) (int, error) {
	if w.err != nil {
		return 0, w.err
	} else if (w.header.Typeflag != TypeReg) && (w.header.Typeflag != TypeGNUSparse) {
		w.err = errHeaderTypeflag
		return 0, w.err
	} else if len(b) == 0 {
		return 0, nil
	}

	tooMuch := int64(len(b)) > w.remaining
	if tooMuch {
		b = b[:w.remaining]
	}

	ret := 0
	for len(b) > 0 {
		if w.bIndex == 0 {
			split := len(b) &^ 511
			prefix, suffix := b[:split], b[split:]

			if len(prefix) > 0 {
				n, err := w.w.Write(prefix)
				w.remaining -= int64(n)
				ret += n
				if err != nil {
					w.err = err
					break
				}
			}

			if len(suffix) > 0 {
				w.bIndex = int32(copy(w.block[:], suffix))
				w.remaining -= int64(w.bIndex)
				ret += int(w.bIndex)
			}

			b = nil
			break
		}

		n := copy(w.block[w.bIndex:], b)
		w.bIndex += int32(n)
		w.remaining -= int64(n)
		ret += n
		b = b[n:]

		if int(w.bIndex) < len(w.block) {
			continue
		} else if _, err := w.w.Write(w.block[:]); err != nil {
			w.err = err
			break
		}
	}

	if tooMuch && (w.err == nil) {
		w.err = errHeaderSize
	}

	return ret, w.err
}

// Close satisfies io.Closer. It closes the entire archive, not just one entry.
func (w *Writer) Close() error {
	if err := w.flush(); err != nil {
		return err
	}
	clear(w.nameBuf[:1024])
	if _, err := w.w.Write(w.nameBuf[:1024]); err != nil {
		w.err = err
		return err
	}
	w.err = errClosed
	return nil
}

// NewReader creates a new Reader reading from r.
func NewReader(r io.Reader) *Reader {
	return &Reader{r: r}
}

// Reader provides sequential reading of a SUITAR archive.
type Reader struct {
	err        error
	r          io.Reader
	remaining  int64
	numPadding int32
	sparse     bool
	block      block
	nameBuf    [4096]byte
}

// Next advances to the next entry in the SUITAR archive, preparing to read the
// file's contents (as r is also an io.Reader).
func (r *Reader) Next() (Header, error) {
	if r.err != nil {
		return Header{}, r.err
	} else if r.remaining > 0 {
		if _, err := io.Copy(io.Discard, r); err != nil {
			r.err = err
			return Header{}, r.err
		}
	}

	if _, err := readFullNoEOF(r.r, r.block[:]); err != nil {
		r.err = err
		return Header{}, r.err
	} else if r.block[0x09C] == 0 {
		// SUITAR ends with 2 blocks (1024 bytes) of zeroes.
		if !isAllZeroes(r.block[:]) {
			r.err = errBadHeader
			return Header{}, r.err
		} else if _, err := readFullNoEOF(r.r, r.block[:]); err != nil {
			r.err = err
			return Header{}, r.err
		} else if !isAllZeroes(r.block[:]) {
			r.err = errBadHeader
			return Header{}, r.err
		}
		r.err = io.EOF
		return Header{}, r.err
	}

	nameLenInclNul, err := parseBlock0(&r.block)
	if err != nil {
		r.err = err
		return Header{}, r.err
	}

	n1 := nameLenInclNul - 1
	n2 := roundUp512(nameLenInclNul)
	if _, err := readFullNoEOF(r.r, r.nameBuf[:n2]); err != nil {
		r.err = err
		return Header{}, r.err
	}
	name := string(r.nameBuf[:n1])
	if !IsValidHeaderName(name) || !isAllZeroes(r.nameBuf[n1:n2]) {
		r.err = errBadFileName
		return Header{}, r.err
	}

	if _, err := readFullNoEOF(r.r, r.block[:0x200]); err != nil {
		return Header{}, err
	}
	typeflag, size, mode, modTime, err := parseBlock1(&r.block)
	if err != nil {
		return Header{}, err
	}

	r.remaining = size
	r.numPadding = int32(roundUp512(uint64(size)) - uint64(size))
	r.sparse = typeflag == TypeGNUSparse

	return Header{
		Typeflag: typeflag,
		Name:     name,
		Size:     size,
		Mode:     mode,
		ModTime:  time.Unix(modTime, 0),
	}, nil
}

func parseBlock0(b *block) (uint64, error) {
	if !b.isValidHeader(0) {
		return 0, errBadHeader
	}

	size := u64le(b[0x080:])
	modTime := u64le(b[0x08C:])
	if (size < 2) || (4097 <= size) {
		return 0, errBadFileName
	} else if (modTime != 0) || (b[0x09C] != 'L') {
		return 0, errBadHeader
	}

	return size, nil
}

func parseBlock1(b *block) (byte, int64, int64, int64, error) {
	if !b.isValidHeader(1) {
		return 0, 0, 0, 0, errBadHeader
	}

	typeflag := b[0x09C]
	if (typeflag != TypeReg) && (typeflag != TypeDir) && (typeflag != TypeGNUSparse) {
		return 0, 0, 0, 0, errBadHeader
	}

	mode := Mode644
	if b[0x068] == '7' {
		mode = Mode755
	}
	size := int64(u64le(b[0x080:]))
	if size < 0 {
		return 0, 0, 0, 0, errBadHeader
	}
	modTime := int64(u64le(b[0x08C:]))
	if modTime < 0 {
		return 0, 0, 0, 0, errBadHeader
	}

	if typeflag == TypeDir {
		if (size != 0) || (mode != Mode755) {
			return 0, 0, 0, 0, errBadHeader
		}
	} else if typeflag == TypeGNUSparse {
		if size != 0 {
			return 0, 0, 0, 0, errBadHeader
		}
		size0 := int64(u64le(b[0x186:]))
		size1 := int64(u64le(b[0x1E7:]))
		if (size0 != size1) || (size1 < 0) {
			return 0, 0, 0, 0, errBadHeader
		}
		size = size1
	}

	return typeflag, size, mode, modTime, nil
}

// Read satisfies io.Reader.
func (r *Reader) Read(b []byte) (int, error) {
	if r.err != nil {
		return 0, r.err
	} else if r.remaining == 0 {
		return 0, io.EOF
	} else if len(b) == 0 {
		return 0, nil
	}

	b = b[:int(min(r.remaining, int64(len(b))))]
	if r.sparse {
		n := len(b)
		clear(b)
		r.remaining -= int64(n)
		if r.remaining == 0 {
			return n, io.EOF
		}
		return n, nil
	}

	n, err := r.r.Read(b)
	r.remaining -= int64(n)
	if r.remaining == 0 {
		if r.numPadding > 0 {
			padding := r.block[:r.numPadding]
			_, readFullErr := readFullNoEOF(r.r, padding)
			if err == nil {
				err = readFullErr
				if err == nil {
					for _, c := range padding {
						if c != 0 {
							err = errBadPadding
							break
						}
					}
				}
			}
			r.numPadding = 0
		}
		if (err == nil) || (err == io.EOF) {
			return n, io.EOF
		}

	} else if err == io.EOF {
		err = io.ErrUnexpectedEOF
	}

	r.err = err
	return n, r.err
}
