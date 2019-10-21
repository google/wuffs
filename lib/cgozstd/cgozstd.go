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

// Package cgozstd wraps the C "zstd" library.
package cgozstd

/*
#cgo pkg-config: libzstd
#include "zstd.h"
#include "zstd_errors.h"

#include <stdint.h>

// --------
#if (ZSTD_VERSION_MAJOR < 1) || (ZSTD_VERSION_MINOR < 3)

int32_t cgozstd_compress_start(ZSTD_CCtx* z,
		uint8_t* dict_ptr,
		uint32_t dict_len,
		int compression_level) {
	if (dict_len > 0) {
		return -1;
	}
	return ZSTD_getErrorCode(ZSTD_initCStream(z, compression_level));
}

int32_t cgozstd_decompress_start(ZSTD_DCtx* z,
		uint8_t* dict_ptr,
		uint32_t dict_len) {
	if (dict_len > 0) {
		return -1;
	}
	return ZSTD_getErrorCode(ZSTD_initDStream(z));
}

#else

// TODO: don't use the unsupported ZSTD_initFoo_usingDict API.
ZSTDLIB_API size_t ZSTD_initCStream_usingDict(
		ZSTD_CCtx* z,
		const void* dict_ptr,
		size_t dict_len,
		int compression_level);
ZSTDLIB_API size_t ZSTD_initDStream_usingDict(
		ZSTD_DCtx* z,
		const void* dict_ptr,
		size_t dict_len);

int32_t cgozstd_compress_start(ZSTD_CCtx* z,
		uint8_t* dict_ptr,
		uint32_t dict_len,
		int compression_level) {
	return ZSTD_getErrorCode(ZSTD_initCStream_usingDict(
			z, dict_ptr, dict_len, compression_level));
}

int32_t cgozstd_decompress_start(ZSTD_DCtx* z,
		uint8_t* dict_ptr,
		uint32_t dict_len) {
	return ZSTD_getErrorCode(ZSTD_initDStream_usingDict(
			z, dict_ptr, dict_len));
}

#endif
// --------

typedef struct {
	uint32_t ndst;
	uint32_t nsrc;
	uint32_t eof;
} advances;

int32_t cgozstd_compress(ZSTD_CCtx* z,
		advances* a,
		uint8_t* dst_ptr,
		uint32_t dst_len,
		uint8_t* src_ptr,
		uint32_t src_len) {
	ZSTD_outBuffer ob;
	ob.dst = dst_ptr;
	ob.size = dst_len;
	ob.pos = 0;

	ZSTD_inBuffer ib;
	ib.src = src_ptr;
	ib.size = src_len;
	ib.pos = 0;

	size_t result = ZSTD_compressStream(z, &ob, &ib);

	a->ndst = ob.pos;
	a->nsrc = ib.pos;
	a->eof = 0;

	return ZSTD_getErrorCode(result);
}

int32_t cgozstd_compress_end(ZSTD_CCtx* z,
		advances* a,
		uint8_t* dst_ptr,
		uint32_t dst_len) {
	ZSTD_outBuffer ob;
	ob.dst = dst_ptr;
	ob.size = dst_len;
	ob.pos = 0;

	size_t result = ZSTD_endStream(z, &ob);

	a->ndst = ob.pos;
	a->nsrc = 0;
	a->eof = (result == 0) ? 1 : 0;

	return ZSTD_getErrorCode(result);
}

int32_t cgozstd_decompress(ZSTD_DCtx* z,
		advances* a,
		uint8_t* dst_ptr,
		uint32_t dst_len,
		uint8_t* src_ptr,
		uint32_t src_len) {
	ZSTD_outBuffer ob;
	ob.dst = dst_ptr;
	ob.size = dst_len;
	ob.pos = 0;

	ZSTD_inBuffer ib;
	ib.src = src_ptr;
	ib.size = src_len;
	ib.pos = 0;

	size_t result = ZSTD_decompressStream(z, &ob, &ib);

	a->ndst = ob.pos;
	a->nsrc = ib.pos;
	a->eof = (result == 0) ? 1 : 0;

	return ZSTD_getErrorCode(result);
}
*/
import "C"

import (
	"errors"
	"io"
	"unsafe"

	"github.com/google/wuffs/lib/compression"
)

const cgoEnabled = true

// maxLen avoids overflow concerns when converting C and Go integer types.
const maxLen = 1 << 30

var (
	errMissingResetCall    = errors.New("cgozstd: missing Reset call")
	errNilIOReader         = errors.New("cgozstd: nil io.Reader")
	errNilIOWriter         = errors.New("cgozstd: nil io.Writer")
	errNilReceiver         = errors.New("cgozstd: nil receiver")
	errOutOfMemory         = errors.New("cgozstd: out of memory")
	errZstdVersionTooSmall = errors.New("cgozstd: zstd version too small (1.3 minimum)")
)

type errCode int32

func (e errCode) Error() string {
	if s := C.GoString(C.ZSTD_getErrorString(C.ZSTD_ErrorCode(e))); s != "" {
		return "cgozstd: " + s
	}
	return "cgozstd: unknown error"
}

// ReaderRecycler can lessen the new memory allocated when calling Reader.Reset
// on a bound Reader.
//
// It is not safe to use a ReaderRecycler and a Reader concurrently.
type ReaderRecycler struct {
	z      *C.ZSTD_DCtx
	closed bool
}

// Bind lets r re-use the memory that is manually managed by c. Call c.Close to
// free that memory.
func (c *ReaderRecycler) Bind(r *Reader) {
	r.recycler = c
}

// Close implements io.Closer.
func (c *ReaderRecycler) Close() error {
	c.closed = true
	if c.z != nil {
		C.ZSTD_freeDCtx(c.z)
		c.z = nil
	}
	return nil
}

// Reader decompresses from the zstd format.
//
// The zero value is not usable until Reset is called.
type Reader struct {
	buf  [65536]byte
	i, j uint32
	r    io.Reader

	dictionary []byte

	readErr error
	zstdErr error

	recycler *ReaderRecycler

	z *C.ZSTD_DCtx
	a C.advances
}

// Reset implements compression.Reader.
func (r *Reader) Reset(reader io.Reader, dictionary []byte) error {
	if r == nil {
		return errNilReceiver
	}
	if err := r.Close(); err != nil {
		return err
	}
	if reader == nil {
		return errNilIOReader
	}
	if len(dictionary) > maxLen {
		dictionary = dictionary[len(dictionary)-maxLen:]
	}

	r.r = reader
	r.dictionary = dictionary
	return nil
}

// Close implements compression.Reader.
func (r *Reader) Close() error {
	if r == nil {
		return errNilReceiver
	}
	if r.r == nil {
		return nil
	}
	r.i = 0
	r.j = 0
	r.r = nil
	r.readErr = nil
	r.zstdErr = nil
	if r.z != nil {
		if (r.recycler != nil) && !r.recycler.closed && (r.recycler.z == nil) {
			r.recycler.z, r.z = r.z, nil
		} else {
			C.ZSTD_freeDCtx(r.z)
			r.z = nil
		}
	}
	return nil
}

// Read implements compression.Reader.
func (r *Reader) Read(p []byte) (int, error) {
	if r == nil {
		return 0, errNilReceiver
	}
	if r.r == nil {
		return 0, errMissingResetCall
	}

	if r.z == nil {
		if (r.recycler != nil) && !r.recycler.closed && (r.recycler.z != nil) {
			r.z, r.recycler.z = r.recycler.z, nil
		} else {
			r.z = C.ZSTD_createDStream()
			if r.z == nil {
				return 0, errOutOfMemory
			}
		}

		e := errCode(0)
		if len(r.dictionary) == 0 {
			e = errCode(C.cgozstd_decompress_start(r.z,
				(*C.uint8_t)(nil),
				(C.uint32_t)(0),
			))
		} else {
			e = errCode(C.cgozstd_decompress_start(r.z,
				(*C.uint8_t)(unsafe.Pointer(&r.dictionary[0])),
				(C.uint32_t)(len(r.dictionary)),
			))
		}
		if e < 0 {
			r.zstdErr = errZstdVersionTooSmall
			return 0, r.zstdErr
		} else if e != 0 {
			r.zstdErr = e
			return 0, r.zstdErr
		}
	}

	if len(p) > maxLen {
		p = p[:maxLen]
	}

	for numRead := 0; ; {
		if r.zstdErr != nil {
			return numRead, r.zstdErr
		}
		if len(p) == 0 {
			return numRead, nil
		}

		if r.i >= r.j {
			if r.readErr != nil {
				return numRead, r.readErr
			}

			n, err := r.r.Read(r.buf[:])
			if err == io.EOF {
				err = io.ErrUnexpectedEOF
			}
			r.i, r.j, r.readErr = 0, uint32(n), err
			continue
		}

		e := errCode(C.cgozstd_decompress(r.z, &r.a,
			(*C.uint8_t)(unsafe.Pointer(&p[0])),
			(C.uint32_t)(len(p)),
			(*C.uint8_t)(unsafe.Pointer(&r.buf[r.i])),
			(C.uint32_t)(r.j-r.i),
		))

		numRead += int(r.a.ndst)
		p = p[int(r.a.ndst):]

		r.i += uint32(r.a.nsrc)

		if e == 0 {
			if r.a.eof == 0 {
				continue
			}
			r.zstdErr = io.EOF
		} else {
			r.zstdErr = e
		}
		return numRead, r.zstdErr
	}
}

// WriterRecycler can lessen the new memory allocated when calling Writer.Reset
// on a bound Writer.
//
// It is not safe to use a WriterRecycler and a Writer concurrently.
type WriterRecycler struct {
	z      *C.ZSTD_CCtx
	closed bool
}

// Bind lets w re-use the memory that is manually managed by c. Call c.Close to
// free that memory.
func (c *WriterRecycler) Bind(w *Writer) {
	w.recycler = c
}

// Close implements io.Closer.
func (c *WriterRecycler) Close() error {
	c.closed = true
	if c.z != nil {
		C.ZSTD_freeCCtx(c.z)
		c.z = nil
	}
	return nil
}

// Writer compresses to the zstd format.
//
// Compressed bytes may be buffered and not sent to the underlying io.Writer
// until Close is called.
//
// The zero value is not usable until Reset is called.
type Writer struct {
	buf   [65536]byte
	j     uint32
	w     io.Writer
	level compression.Level

	dictionary []byte

	writeErr error

	recycler *WriterRecycler

	z *C.ZSTD_CCtx
	a C.advances
}

func (w *Writer) zstdCompressionLevel() int32 {
	return w.level.Interpolate(1, 2, 3, 15, 22)
}

// Reset implements compression.Writer.
func (w *Writer) Reset(writer io.Writer, dictionary []byte, level compression.Level) error {
	if w == nil {
		return errNilReceiver
	}
	w.close()
	if writer == nil {
		return errNilIOWriter
	}
	if len(dictionary) > maxLen {
		dictionary = dictionary[len(dictionary)-maxLen:]
	}

	w.w = writer
	w.level = level
	w.dictionary = dictionary
	return nil
}

// Close implements compression.Writer.
func (w *Writer) Close() error {
	if w == nil {
		return errNilReceiver
	}
	err := w.flush(true)
	w.close()
	return err
}

func (w *Writer) flush(final bool) error {
	if w.w == nil {
		return nil
	}

	if final {
		if err := w.write(nil, true); err != nil {
			return err
		}
	}

	if w.j == 0 {
		return nil
	}
	_, err := w.w.Write(w.buf[:w.j])
	w.j = 0
	return err
}

func (w *Writer) close() {
	if w.w == nil {
		return
	}
	w.j = 0
	w.w = nil
	w.writeErr = nil
	if w.z != nil {
		if (w.recycler != nil) && !w.recycler.closed && (w.recycler.z == nil) {
			w.recycler.z, w.z = w.z, nil
		} else {
			C.ZSTD_freeCCtx(w.z)
			w.z = nil
		}
	}
}

// Write implements compression.Writer.
func (w *Writer) Write(p []byte) (int, error) {
	if w == nil {
		return 0, errNilReceiver
	}
	if w.w == nil {
		return 0, errMissingResetCall
	}
	if w.writeErr != nil {
		return 0, w.writeErr
	}

	originalLenP := len(p)
	for {
		remaining := []byte(nil)
		if len(p) > maxLen {
			p, remaining = p[:maxLen], p[maxLen:]
		}

		if err := w.write(p, false); err != nil {
			return 0, err
		}

		p, remaining = remaining, nil
		if len(p) == 0 {
			return originalLenP, nil
		}
	}
}

func (w *Writer) write(p []byte, final bool) error {
	if len(p) > maxLen {
		panic("unreachable")
	}

	if w.z == nil {
		if (w.recycler != nil) && !w.recycler.closed && (w.recycler.z != nil) {
			w.z, w.recycler.z = w.recycler.z, nil
		} else {
			w.z = C.ZSTD_createCStream()
			if w.z == nil {
				return errOutOfMemory
			}
		}

		e := errCode(0)
		if len(w.dictionary) == 0 {
			e = errCode(C.cgozstd_compress_start(w.z,
				(*C.uint8_t)(nil),
				(C.uint32_t)(0),
				C.int(w.zstdCompressionLevel()),
			))
		} else {
			e = errCode(C.cgozstd_compress_start(w.z,
				(*C.uint8_t)(unsafe.Pointer(&w.dictionary[0])),
				(C.uint32_t)(len(w.dictionary)),
				C.int(w.zstdCompressionLevel()),
			))
		}
		if e < 0 {
			w.writeErr = errZstdVersionTooSmall
			return w.writeErr
		} else if e != 0 {
			w.writeErr = e
			return w.writeErr
		}
	}

	for (len(p) > 0) || final {
		if w.j == uint32(len(w.buf)) {
			if err := w.flush(false); err != nil {
				w.writeErr = err
				return w.writeErr
			}
		}

		e := errCode(0)
		if final {
			e = errCode(C.cgozstd_compress_end(w.z, &w.a,
				(*C.uint8_t)(unsafe.Pointer(&w.buf[w.j])),
				(C.uint32_t)(uint32(len(w.buf))-w.j),
			))
			final = w.a.eof == 0
		} else {
			e = errCode(C.cgozstd_compress(w.z, &w.a,
				(*C.uint8_t)(unsafe.Pointer(&w.buf[w.j])),
				(C.uint32_t)(uint32(len(w.buf))-w.j),
				(*C.uint8_t)(unsafe.Pointer(&p[0])),
				(C.uint32_t)(len(p)),
			))
		}

		w.j += uint32(w.a.ndst)
		p = p[uint32(w.a.nsrc):]

		if e != 0 {
			w.writeErr = e
			return w.writeErr
		}
	}
	return nil
}
