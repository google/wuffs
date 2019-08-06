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

package raczlib

import (
	"compress/zlib"
	"errors"
	"hash/adler32"
	"io"

	"github.com/google/wuffs/lib/rac"
)

var (
	errInvalidCodec      = errors.New("raczlib: invalid Codec (expected rac.CodecZlib)")
	errInvalidReadSeeker = errors.New("raczlib: invalid ReadSeeker")
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

// Reader reads a RAC file.
//
// Do not modify its exported fields after calling any of its methods.
type Reader struct {
	// ReadSeeker is where the RAC-encoded data is read from.
	//
	// It may also implement io.ReaderAt, in which case its ReadAt method will
	// be preferred over combining Read and Seek, as the former is presumably
	// more efficient. This is optional: io.ReaderAt is a stronger contract
	// than io.ReadSeeker, as multiple concurrent ReadAt calls must not
	// interfere with each other.
	//
	// For example, this type itself only implements io.ReadSeeker, not
	// io.ReaderAt, as it is not safe for concurrent use.
	//
	// Nil is an invalid value.
	ReadSeeker io.ReadSeeker

	// CompressedSize is the size of the RAC file.
	//
	// Zero is an invalid value, as an empty file is not a valid RAC file.
	CompressedSize int64

	// err is the first error encountered. It is sticky: once a non-nil error
	// occurs, all public methods will return that error.
	err error

	// reader is the low-level (Codec-agnostic) RAC reader.
	racReader rac.Reader

	// buf is a scratch buffer.
	buf [2]byte

	// cachedZlibReader lets us re-use the memory allocated for a zlib reader,
	// when decompressing multiple chunks.
	cachedZlibReader zlib.Resetter

	// These fields contain the most recently used shared dictionary.
	cachedDictionary       []byte
	cachedDictionaryCRange rac.Range
}

func (r *Reader) initialize() error {
	if r.err != nil {
		return r.err
	}
	if r.racReader.ReadSeeker != nil {
		// We're already initialized.
		return nil
	}
	if r.ReadSeeker == nil {
		r.err = errInvalidReadSeeker
		return r.err
	}

	r.racReader.ReadSeeker = r.ReadSeeker
	r.racReader.CompressedSize = r.CompressedSize
	r.racReader.MakeDecompressor = r.makeDecompressor
	r.racReader.MakeReaderContext = r.makeReaderContext
	return nil
}

// Read implements io.Reader.
func (r *Reader) Read(p []byte) (int, error) {
	if err := r.initialize(); err != nil {
		return 0, err
	}
	n, err := r.racReader.Read(p)
	if (err != nil) && (err != io.EOF) {
		r.err = err
	}
	return n, err
}

// Seek implements io.Seeker.
func (r *Reader) Seek(offset int64, whence int) (int64, error) {
	if err := r.initialize(); err != nil {
		return 0, err
	}
	n, err := r.racReader.Seek(offset, whence)
	if (err != nil) && (err != io.EOF) {
		r.err = err
	}
	return n, err
}

func (r *Reader) makeDecompressor(compressed io.Reader, rctx rac.ReaderContext) (io.Reader, error) {
	if r.cachedZlibReader != nil {
		if err := r.cachedZlibReader.Reset(compressed, rctx.Secondary); err != nil {
			if err == io.EOF {
				err = io.ErrUnexpectedEOF
			}
			r.err = err
			return nil, r.err
		}
		return r.cachedZlibReader.(io.Reader), nil
	}

	zlibReader, err := zlib.NewReaderDict(compressed, rctx.Secondary)
	if err != nil {
		if err == io.EOF {
			err = io.ErrUnexpectedEOF
		}
		r.err = err
		return nil, r.err
	}
	r.cachedZlibReader = zlibReader.(zlib.Resetter)
	return zlibReader, nil
}

func (r *Reader) makeReaderContext(chunk rac.Chunk) (rac.ReaderContext, error) {
	// For a description of the RAC+Zlib secondary-data format, see
	// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md#rac--zlib

	if chunk.Codec != rac.CodecZlib {
		r.err = errInvalidCodec
		return rac.ReaderContext{}, r.err
	}
	if !chunk.CTertiary.Empty() {
		r.err = errInvalidDictionary
		return rac.ReaderContext{}, r.err
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
	if (cRange.Size() < 6) || (cRange[1] > r.CompressedSize) || (chunk.TTag != 0xFF) {
		r.err = errInvalidDictionary
		return rac.ReaderContext{}, r.err
	}

	// Read the dictionary size.
	if err := readAt(r.ReadSeeker, r.buf[:2], cRange[0]); err != nil {
		r.err = err
		return rac.ReaderContext{}, r.err
	}
	dictSize := int64(r.buf[0]) | (int64(r.buf[1]) << 8)

	// Check the size. The +6 is for the 2 byte prefix (dictionary size) and
	// the 4 byte suffix (checksum).
	if (dictSize + 6) > cRange.Size() {
		r.err = errInvalidDictionary
		return rac.ReaderContext{}, r.err
	}

	// Allocate or re-use the cachedDictionary buffer.
	buffer := []byte(nil)
	if n := dictSize + 4; int64(cap(r.cachedDictionary)) >= n {
		buffer = r.cachedDictionary[:n]
	} else {
		buffer = make([]byte, n)
	}

	// Read the dictionary and checksum.
	if err := readAt(r.ReadSeeker, buffer, cRange[0]+2); err != nil {
		r.err = err
		return rac.ReaderContext{}, r.err
	}

	// Verify the checksum.
	dict, checksum := buffer[:dictSize], buffer[dictSize:]
	if u32BE(checksum) != adler32.Checksum(dict) {
		r.err = errInvalidDictionary
		return rac.ReaderContext{}, r.err
	}

	// Save to the MRU cache and return.
	r.cachedDictionary = dict
	r.cachedDictionaryCRange = cRange
	return rac.ReaderContext{Secondary: dict}, nil
}
