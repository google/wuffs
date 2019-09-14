// Copyright 2019 The Wuffs Authors.
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

// Package raczlib provides access to RAC (Random Access Compression) files
// with the Zlib compression codec.
//
// The RAC specification is at
// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md
package raczlib

import (
	"bytes"
	"compress/zlib"
	"errors"
	"hash/adler32"
	"io"

	"github.com/google/wuffs/lib/compression"
	"github.com/google/wuffs/lib/rac"
	"github.com/google/wuffs/lib/zlibcut"
)

var (
	errInvalidCodec      = errors.New("raczlib: invalid codec")
	errInvalidDictionary = errors.New("raczlib: invalid dictionary")
)

func u32BE(b []byte) uint32 {
	_ = b[3] // Early bounds check to guarantee safety of reads below.
	return (uint32(b[0]) << 24) | (uint32(b[1]) << 16) | (uint32(b[2]) << 8) | (uint32(b[3]))
}

// suffix32K returns the last 32 KiB of b, or if b is shorter than that, it
// returns just b.
//
// The Zlib format only supports up to 32 KiB of history or shared dictionary.
func suffix32K(b []byte) []byte {
	const n = 32 * 1024
	if len(b) > n {
		return b[len(b)-n:]
	}
	return b
}

// CodecReader specializes a rac.Reader to decode Zlib-compressed chunks.
type CodecReader struct {
	// cachedZlibReader lets us re-use the memory allocated for a zlib reader,
	// when decompressing multiple chunks.
	cachedZlibReader compression.Reader

	// These fields contain the most recently used shared dictionary.
	cachedDictionary       []byte
	cachedDictionaryCRange rac.Range

	// buf is a scratch buffer.
	buf [2]byte
}

// Close implements rac.CodecReader.
func (r *CodecReader) Close() error {
	return nil
}

// Accepts implements rac.CodecReader.
func (r *CodecReader) Accepts(c rac.Codec) bool {
	return c == rac.CodecZlib
}

// Clone implements rac.CodecReader.
func (r *CodecReader) Clone() rac.CodecReader {
	return &CodecReader{}
}

// MakeReaderContext implements rac.CodecReader.
func (r *CodecReader) MakeReaderContext(rs io.ReadSeeker, chunk rac.Chunk) (rac.ReaderContext, error) {
	// For a description of the RAC+Zlib secondary-data format, see
	// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md#rac--zlib

	if !chunk.CTertiary.Empty() {
		return rac.ReaderContext{}, errInvalidDictionary
	}
	if chunk.CSecondary.Empty() {
		return rac.ReaderContext{}, nil
	}
	cRange := chunk.CSecondary

	// Load from the MRU cache, if it was loaded from the same cRange.
	if (cRange == r.cachedDictionaryCRange) && !cRange.Empty() {
		return rac.ReaderContext{Secondary: r.cachedDictionary}, nil
	}

	// Check the cRange size and the tTag.
	if (cRange.Size() < 6) || (chunk.TTag != 0xFF) {
		return rac.ReaderContext{}, errInvalidDictionary
	}

	// Read the dictionary size.
	if _, err := rs.Seek(cRange[0], io.SeekStart); err != nil {
		return rac.ReaderContext{}, errInvalidDictionary
	}
	if _, err := io.ReadFull(rs, r.buf[:2]); err != nil {
		return rac.ReaderContext{}, errInvalidDictionary
	}
	dictSize := int64(r.buf[0]) | (int64(r.buf[1]) << 8)

	// Check the size. The +6 is for the 2 byte prefix (dictionary size) and
	// the 4 byte suffix (checksum).
	if (dictSize + 6) > cRange.Size() {
		return rac.ReaderContext{}, errInvalidDictionary
	}

	// Allocate or re-use the cachedDictionary buffer.
	buffer := []byte(nil)
	if n := dictSize + 4; int64(cap(r.cachedDictionary)) >= n {
		buffer = r.cachedDictionary[:n]
		// Invalidate the cached dictionary, as we are re-using its memory.
		r.cachedDictionaryCRange = rac.Range{}
	} else {
		buffer = make([]byte, n)
	}

	// Read the dictionary and checksum.
	if _, err := io.ReadFull(rs, buffer); err != nil {
		return rac.ReaderContext{}, errInvalidDictionary
	}

	// Verify the checksum.
	dict, checksum := buffer[:dictSize], buffer[dictSize:]
	if u32BE(checksum) != adler32.Checksum(dict) {
		return rac.ReaderContext{}, errInvalidDictionary
	}

	// Save to the MRU cache and return.
	r.cachedDictionary = dict
	r.cachedDictionaryCRange = cRange
	return rac.ReaderContext{Secondary: dict}, nil
}

// CodecWriter specializes a rac.Writer to encode Zlib-compressed chunks.
type CodecWriter struct {
	// These fields help reduce memory allocation by re-using previous byte
	// buffers or the zlib.Writer.
	compressed bytes.Buffer
	zlibWriter *zlib.Writer
	stash      []byte
}

// Close implements rac.CodecWriter.
func (w *CodecWriter) Close() error {
	return nil
}

// Clone implements rac.CodecWriter.
func (w *CodecWriter) Clone() rac.CodecWriter {
	return &CodecWriter{}
}

// Compress implements rac.CodecWriter.
func (w *CodecWriter) Compress(p []byte, q []byte, resourcesData [][]byte) (
	codec rac.Codec, compressed []byte, secondaryResource int, tertiaryResource int, retErr error) {

	secondaryResource = rac.NoResourceUsed

	// Compress p+q without any shared dictionary.
	baseline, err := w.compress(p, q, nil)
	if err != nil {
		return 0, nil, 0, 0, err
	}
	// If there aren't any potential shared dictionaries, or if the baseline
	// compressed form is small, just return the baseline.
	const minBaselineSizeToConsiderDictionaries = 256
	if (len(resourcesData) == 0) || (len(baseline) < minBaselineSizeToConsiderDictionaries) {
		return rac.CodecZlib, baseline, secondaryResource, rac.NoResourceUsed, nil
	}

	// w.stash keeps a copy of the best compression so far. Every call to
	// w.compress can clobber the bytes previously returned by w.compress.
	w.stash = append(w.stash[:0], baseline...)
	compressed = w.stash

	// Only use a shared dictionary if it results in something smaller than
	// 98.4375% (an arbitrary heuristic threshold) of the baseline. Otherwise,
	// it's arguably not worth the additional complexity.
	threshold := (len(baseline) / 64) * 63

	for i, data := range resourcesData {
		candidate, err := w.compress(p, q, suffix32K(data))
		if err != nil {
			return 0, nil, 0, 0, err
		}
		if n := len(candidate); (n >= threshold) || (n >= len(compressed)) {
			continue
		}
		secondaryResource = i

		// As an optimization, if the final candidate is the winner, we don't
		// have to copy its contents to w.stash.
		if i == len(resourcesData)-1 {
			compressed = candidate
		} else {
			w.stash = append(w.stash[:0], candidate...)
			compressed = w.stash
		}
	}
	return rac.CodecZlib, compressed, secondaryResource, rac.NoResourceUsed, nil
}

func (w *CodecWriter) compress(p []byte, q []byte, dict []byte) ([]byte, error) {
	w.compressed.Reset()
	zw, err := (*zlib.Writer)(nil), error(nil)
	if len(dict) != 0 {
		// A zlib.Writer doesn't have a Reset(etc, dict) method, so we have to
		// allocate a new zlib.Writer.
		zw, err = zlib.NewWriterLevelDict(&w.compressed, zlib.BestCompression, dict)
		if err != nil {
			return nil, err
		}
	} else if w.zlibWriter == nil {
		zw, err = zlib.NewWriterLevelDict(&w.compressed, zlib.BestCompression, nil)
		if err != nil {
			return nil, err
		}
		w.zlibWriter = zw
	} else {
		zw = w.zlibWriter
		zw.Reset(&w.compressed)
	}

	if len(p) > 0 {
		if _, err := zw.Write(p); err != nil {
			return nil, err
		}
	}
	if len(q) > 0 {
		if _, err := zw.Write(q); err != nil {
			return nil, err
		}
	}

	if err := zw.Close(); err != nil {
		return nil, err
	}
	return w.compressed.Bytes(), nil
}

// CanCut implements rac.CodecWriter.
func (w *CodecWriter) CanCut() bool {
	return true
}

// Cut implements rac.CodecWriter.
func (w *CodecWriter) Cut(codec rac.Codec, encoded []byte, maxEncodedLen int) (encodedLen int, decodedLen int, retErr error) {
	if codec != rac.CodecZlib {
		return 0, 0, errInvalidCodec
	}
	return zlibcut.Cut(nil, encoded, maxEncodedLen)
}

// WrapResource implements rac.CodecWriter.
func (w *CodecWriter) WrapResource(raw []byte) ([]byte, error) {
	// For a description of the RAC+Zlib secondary-data format, see
	// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md#rac--zlib
	raw = suffix32K(raw)
	wrapped := make([]byte, len(raw)+6)
	wrapped[0] = uint8(len(raw) >> 0)
	wrapped[1] = uint8(len(raw) >> 8)
	copy(wrapped[2:], raw)
	a := adler32.Checksum(raw)
	wrapped[len(wrapped)-4] = uint8(a >> 24)
	wrapped[len(wrapped)-3] = uint8(a >> 16)
	wrapped[len(wrapped)-2] = uint8(a >> 8)
	wrapped[len(wrapped)-1] = uint8(a >> 0)
	return wrapped, nil
}
