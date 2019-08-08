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

// readAt calls ReadAt if it is available, otherwise it falls back to calling
// Seek and then ReadFull. Calling ReadAt is presumably slightly more
// efficient, e.g. one syscall instead of two.
func readAt(r io.ReadSeeker, p []byte, offset int64) error {
	if a, ok := r.(io.ReaderAt); ok && false {
		n, err := a.ReadAt(p, offset)
		if (n == len(p)) && (err == io.EOF) {
			err = nil
		}
		return err
	}
	if _, err := r.Seek(offset, io.SeekStart); err != nil {
		return err
	}
	_, err := io.ReadFull(r, p)
	return err
}

// CodecReader specializes a rac.Reader to decode Zlib-compressed chunks.
type CodecReader struct {
	// cachedZlibReader lets us re-use the memory allocated for a zlib reader,
	// when decompressing multiple chunks.
	cachedZlibReader zlib.Resetter

	// These fields contain the most recently used shared dictionary.
	cachedDictionary       []byte
	cachedDictionaryCRange rac.Range

	// buf is a scratch buffer.
	buf [2]byte
}

// Accepts implements rac.CodecReader.
func (r *CodecReader) Accepts(c rac.Codec) bool {
	return c == rac.CodecZlib
}

// MakeDecompressor implements rac.CodecReader.
func (r *CodecReader) MakeDecompressor(
	compressed io.Reader, rctx rac.ReaderContext) (io.Reader, error) {

	if r.cachedZlibReader != nil {
		if err := r.cachedZlibReader.Reset(compressed, rctx.Secondary); err != nil {
			return nil, err
		}
		return r.cachedZlibReader.(io.Reader), nil
	}

	zlibReader, err := zlib.NewReaderDict(compressed, rctx.Secondary)
	if err != nil {
		return nil, err
	}
	r.cachedZlibReader = zlibReader.(zlib.Resetter)
	return zlibReader, nil
}

// MakeReaderContext implements rac.CodecReader.
func (r *CodecReader) MakeReaderContext(
	rs io.ReadSeeker, compressedSize int64, chunk rac.Chunk) (rac.ReaderContext, error) {

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
	if (cRange.Size() < 6) || (cRange[1] > compressedSize) || (chunk.TTag != 0xFF) {
		return rac.ReaderContext{}, errInvalidDictionary
	}

	// Read the dictionary size.
	if err := readAt(rs, r.buf[:2], cRange[0]); err != nil {
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
	if err := readAt(rs, buffer, cRange[0]+2); err != nil {
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
	compressed bytes.Buffer
	zlibWriter *zlib.Writer
}

// Compress implements rac.CodecWriter.
func (w *CodecWriter) Compress(p []byte, q []byte) (rac.Codec, []byte, error) {
	w.compressed.Reset()

	if w.zlibWriter == nil {
		zw, err := zlib.NewWriterLevel(&w.compressed, zlib.BestCompression)
		if err != nil {
			return 0, nil, err
		}
		w.zlibWriter = zw
	} else {
		w.zlibWriter.Reset(&w.compressed)
	}

	if len(p) > 0 {
		if _, err := w.zlibWriter.Write(p); err != nil {
			return 0, nil, err
		}
	}
	if len(q) > 0 {
		if _, err := w.zlibWriter.Write(q); err != nil {
			return 0, nil, err
		}
	}
	if err := w.zlibWriter.Close(); err != nil {
		return 0, nil, err
	}
	return rac.CodecZlib, w.compressed.Bytes(), nil
}

// Cut implements rac.CodecWriter.
func (w *CodecWriter) Cut(codec rac.Codec, encoded []byte, maxEncodedLen int) (encodedLen int, decodedLen int, retErr error) {
	if codec != rac.CodecZlib {
		return 0, 0, errInvalidCodec
	}
	return zlibcut.Cut(nil, encoded, maxEncodedLen)
}
