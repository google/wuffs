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
	errInvalidCodec          = errors.New("raczlib: invalid Codec (expected rac.CodecZlib)")
	errInvalidChunk          = errors.New("raczlib: invalid chunk")
	errInvalidChunkTooLarge  = errors.New("raczlib: invalid chunk (too large)")
	errInvalidChunkTruncated = errors.New("raczlib: invalid chunk (truncated)")
	errInvalidDictionary     = errors.New("raczlib: invalid dictionary")
	errInvalidReadSeeker     = errors.New("raczlib: invalid ReadSeeker")

	errInternalInconsistentPosition = errors.New("raczlib: internal error: inconsistent position")
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

	// racReader is the low-level (Codec-agnostic) RAC reader.
	racReader rac.Parser

	// These two fields combine for a 3-state state machine:
	//
	//  - "State A" (both fields are zero): no RAC chunk is loaded.
	//
	//  - "State B" (zlibReader is non-zero, inImplicitZeroes is zero): a RAC
	//    chunk is loaded, but not fully exhausted: decompressing the zlib
	//    stream has not hit io.EOF yet.
	//
	//  - "State C" (zlibReader is zero, inImplicitZeroes is non-zero): a RAC
	//    chunk was exhausted, and we now serve the implicit NUL bytes after a
	//    chunk's explicitly encoded data. The number of NUL bytes can be (and
	//    often is) zero.
	//
	// Calling Read may trigger state transitions (which form a cycle): "State
	// A" -> "State B" -> "State C" -> "State A" -> "State B" -> etc.
	//
	// Calling Seek may reset the state machine to "State A".
	//
	// The initial state is "State A".
	zlibReader       io.Reader
	inImplicitZeroes bool

	// buf is a scratch buffer.
	buf [2]byte

	// cachedZlibReader lets us re-use the memory allocated for a zlib reader,
	// when decompressing multiple chunks.
	cachedZlibReader zlib.Resetter

	// currChunk is an io.Reader for the current chunk, used while in "State
	// B". It serves zlib-compressed data, which the (non-nil) zlibReader turns
	// into decompressed data.
	currChunk io.LimitedReader

	// pos is the current position, in DSpace. It is the base value when Seek
	// is called with io.SeekCurrent.
	pos int64

	// dRange is, in "State B" and "State C", what part (in DSpace) of the
	// current chunk has not yet been passed on (via this type's Read method).
	//
	// Within those states, dRange[0] increases over time, as parts of the
	// chunk are decompressed and passed on, but dRange[1] does not change.
	//
	// An invariant is that ((dRange[0] <= pos) && (pos <= dRange[1])).
	//
	// If the first inequality is strict (i.e. dRange[0] < pos) then we have
	// Seek'ed to a pos that is not a chunk boundary, and satisfying the Read
	// method will first require decompressing and discarding some of the chunk
	// data, until dRange[0] reaches pos.
	//
	// If the second inequality is strict (i.e. pos < dRange[1]) and we are in
	// "State C" then we have a non-zero number of implicit NUL bytes left.
	//
	// In "State A", the dRange is empty and unused, other than trivially
	// maintaining the invariant.
	dRange rac.Range

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
	r.currChunk.R = r.ReadSeeker
	return nil
}

// Read implements io.Reader.
func (r *Reader) Read(p []byte) (int, error) {
	if err := r.initialize(); err != nil {
		return 0, err
	}
	numRead := 0

	for len(p) > 0 {
		if (r.pos < r.dRange[0]) || (r.dRange[1] < r.pos) {
			r.err = errInternalInconsistentPosition
			return numRead, r.err
		}

		readFunc := (func(*Reader, []byte) (int, error))(nil)
		switch {
		default: // "State A".
			if err := r.nextChunk(); err != nil {
				return numRead, err
			}
			continue

		case r.zlibReader != nil: // "State B".
			readFunc = (*Reader).readExplicitData

		case r.inImplicitZeroes: // "State C".
			readFunc = (*Reader).readImplicitZeroes
		}

		n, err := readFunc(r, p)
		numRead += n
		p = p[n:]
		if err != nil {
			return numRead, err
		}
	}
	return numRead, nil
}

// readExplicitData serves the zlib-compressed data in a chunk.
func (r *Reader) readExplicitData(p []byte) (int, error) {
	// If the chunk started before r.pos, discard the opening bytes of the
	// chunk's decompressed data.
	for r.pos > r.dRange[0] {
		discardBuffer := p
		discardBufferLen := r.pos - r.dRange[0]
		if int64(len(discardBuffer)) > discardBufferLen {
			discardBuffer = discardBuffer[:discardBufferLen]
		}

		n, err := r.zlibReader.Read(discardBuffer)
		r.dRange[0] += int64(n)
		if err == io.EOF {
			return n, r.transitionFromStateBToStateC()
		}
		if err != nil {
			r.err = err
			return 0, r.err
		}
	}

	// Delegate to the zlib reader.
	n, err := r.zlibReader.Read(p)
	if size := r.dRange.Size(); int64(n) > size {
		n = int(size)
		err = errInvalidChunkTooLarge
	}
	r.pos += int64(n)
	r.dRange[0] += int64(n)
	if err == io.EOF {
		return n, r.transitionFromStateBToStateC()
	} else if err == io.ErrUnexpectedEOF {
		err = errInvalidChunkTruncated
	}
	if err != nil {
		r.err = err
	}
	return n, err
}

func (r *Reader) transitionFromStateBToStateC() error {
	if c, ok := r.zlibReader.(io.Closer); ok {
		if err := c.Close(); err != nil {
			if err == io.EOF {
				err = io.ErrUnexpectedEOF
			}
			r.err = err
			return r.err
		}
	}
	r.zlibReader = nil
	r.inImplicitZeroes = true
	return nil
}

// readImplicitZeroes serves the implicit NUL bytes after a chunk's explicit
// data. As
// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md#decompressing-a-leaf-node
// says, "The Codec may produce fewer bytes than the DRange size. In that case,
// the remaining bytes (in DSpace) are set to NUL (memset to zero)."
func (r *Reader) readImplicitZeroes(p []byte) (int, error) {
	// If the chunk's explicit data finished before r.pos, discard some of the
	// implicit NULs.
	if r.dRange[0] < r.pos {
		r.dRange[0] = r.pos
	}

	// The next r.dRange.Size() bytes are all implicitly zero.
	n := r.dRange.Size()
	if int64(len(p)) > n {
		p = p[:n]
	}
	for i := range p {
		p[i] = 0
	}

	// Update the cursors, check for exhaustion and return.
	r.pos += int64(len(p))
	r.dRange[0] += int64(len(p))
	if r.dRange.Empty() {
		// Transition from "State C" to "State A".
		r.inImplicitZeroes = false
	}
	return len(p), nil
}

// nextChunk loads the next independently compressed chunk. It transitions from
// "State A" to "State B".
//
// It may return io.EOF, in which case the Reader stays in "State A", and the
// r.err "sticky error" field stays nil.
func (r *Reader) nextChunk() error {
	chunk, err := r.racReader.NextChunk()
	if err == io.EOF {
		return io.EOF
	} else if err != nil {
		r.err = err
		return r.err
	}
	if chunk.Codec != rac.CodecZlib {
		r.err = errInvalidCodec
		return r.err
	}
	if chunk.DRange.Empty() || chunk.CPrimary.Empty() || !chunk.CTertiary.Empty() {
		r.err = errInvalidChunk
		return r.err
	}

	dict := []byte(nil)
	if !chunk.CSecondary.Empty() {
		if dict, err = r.loadDictionary(chunk.CSecondary, chunk.TTag); err != nil {
			return err
		}
	}

	if _, err := r.ReadSeeker.Seek(chunk.CPrimary[0], io.SeekStart); err != nil {
		if err == io.EOF {
			err = io.ErrUnexpectedEOF
		}
		r.err = err
		return r.err
	}
	r.currChunk.N = chunk.CPrimary.Size()
	r.dRange = chunk.DRange

	if r.cachedZlibReader != nil {
		if err := r.cachedZlibReader.Reset(&r.currChunk, dict); err != nil {
			if err == io.EOF {
				err = io.ErrUnexpectedEOF
			}
			r.err = err
			return r.err
		}
		r.zlibReader = r.cachedZlibReader.(io.ReadCloser)
	} else {
		r.zlibReader, err = zlib.NewReaderDict(&r.currChunk, dict)
		if err != nil {
			if err == io.EOF {
				err = io.ErrUnexpectedEOF
			}
			r.err = err
			return r.err
		}
		r.cachedZlibReader = r.zlibReader.(zlib.Resetter)
	}
	return nil
}

// loadDictionary loads a dictionary given a chunk's CSecondary range.
//
// For a description of the RAC+Zlib secondary-data format, see
// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md#rac--zlib
func (r *Reader) loadDictionary(cRange rac.Range, tTag uint8) ([]byte, error) {
	// Load from the MRU cache, if it was loaded from the same cRange.
	if (cRange == r.cachedDictionaryCRange) && !cRange.Empty() {
		return r.cachedDictionary, nil
	}
	r.cachedDictionaryCRange = rac.Range{}

	// Check the cRange size and the tTag.
	if (cRange.Size() < 6) || (cRange[1] > r.CompressedSize) || (tTag != 0xFF) {
		r.err = errInvalidDictionary
		return nil, r.err
	}

	// Read the dictionary size.
	if err := readAt(r.ReadSeeker, r.buf[:2], cRange[0]); err != nil {
		r.err = err
		return nil, r.err
	}
	dictSize := int64(r.buf[0]) | (int64(r.buf[1]) << 8)

	// Check the size. The +6 is for the 2 byte prefix (dictionary size) and
	// the 4 byte suffix (checksum).
	if (dictSize + 6) > cRange.Size() {
		r.err = errInvalidDictionary
		return nil, r.err
	}

	// Allocate or re-use the cachedDictionary buffer.
	if n := dictSize + 4; int64(cap(r.cachedDictionary)) >= n {
		r.cachedDictionary = r.cachedDictionary[:n]
	} else {
		r.cachedDictionary = make([]byte, n)
	}

	// Read the dictionary and checksum.
	if err := readAt(r.ReadSeeker, r.cachedDictionary, cRange[0]+2); err != nil {
		r.err = err
		return nil, r.err
	}

	// Verify the checksum and trim the cachedDictionary buffer.
	checksum := r.cachedDictionary[dictSize:]
	r.cachedDictionary = r.cachedDictionary[:dictSize]
	if u32BE(checksum) != adler32.Checksum(r.cachedDictionary) {
		r.err = errInvalidDictionary
		return nil, r.err
	}

	// Save to the MRU cache and return.
	r.cachedDictionary = r.cachedDictionary
	r.cachedDictionaryCRange = cRange
	return r.cachedDictionary, nil
}

// Seek implements io.Seeker.
func (r *Reader) Seek(offset int64, whence int) (int64, error) {
	if err := r.initialize(); err != nil {
		return 0, err
	}

	pos := r.pos
	switch whence {
	case io.SeekStart:
		pos = offset
	case io.SeekCurrent:
		pos += offset
	case io.SeekEnd:
		end, err := r.racReader.DecompressedSize()
		if err != nil {
			r.err = err
			return 0, r.err
		}
		pos = end + offset
	default:
		return 0, errors.New("raczlib.Reader.Seek: invalid whence")
	}

	if r.pos != pos {
		if pos < 0 {
			r.err = errors.New("raczlib.Reader.Seek: negative position")
			return 0, r.err
		}
		if err := r.racReader.SeekToChunkContaining(pos); err != nil {
			r.err = err
			return 0, r.err
		}
		r.pos = pos

		// Maintain the dRange/pos invariant.
		r.dRange[0] = pos
		r.dRange[1] = pos

		// Reset to "State A".
		r.zlibReader = nil
		r.inImplicitZeroes = false
	}
	return r.pos, nil
}
