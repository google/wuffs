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

// Package raczstd provides access to RAC (Random Access Compression) files
// with the Zstd compression codec.
//
// The RAC specification is at
// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md
package raczstd

import (
	"bytes"
	"errors"
	"io"

	"github.com/google/wuffs/lib/cgozstd"
	"github.com/google/wuffs/lib/compression"
	"github.com/google/wuffs/lib/rac"
)

var (
	errCannotCut    = errors.New("raczstd: cannot cut")
	errInvalidCodec = errors.New("raczstd: invalid codec")
)

// CodecReader specializes a rac.Reader to decode Zstd-compressed chunks.
type CodecReader struct {
	cachedReader compression.Reader
	recycler     cgozstd.ReaderRecycler
}

// Close implements rac.CodecReader.
func (r *CodecReader) Close() error {
	return r.recycler.Close()
}

// Accepts implements rac.CodecReader.
func (r *CodecReader) Accepts(c rac.Codec) bool {
	return c == rac.CodecZstandard
}

// Clone implements rac.CodecReader.
func (r *CodecReader) Clone() rac.CodecReader {
	return &CodecReader{}
}

// MakeDecompressor implements rac.CodecReader.
func (r *CodecReader) MakeDecompressor(compressed io.Reader, rctx rac.ReaderContext) (io.Reader, error) {
	if r.cachedReader == nil {
		zr := &cgozstd.Reader{}
		r.cachedReader = zr
		r.recycler.Bind(zr)
	}
	if err := r.cachedReader.Reset(compressed, rctx.Secondary); err != nil {
		return nil, err
	}
	return r.cachedReader, nil
}

// MakeReaderContext implements rac.CodecReader.
func (r *CodecReader) MakeReaderContext(rs io.ReadSeeker, chunk rac.Chunk) (rac.ReaderContext, error) {
	return rac.ReaderContext{}, nil
}

// CodecWriter specializes a rac.Writer to encode Zstd-compressed chunks.
type CodecWriter struct {
	compressed   bytes.Buffer
	cachedWriter compression.Writer
	recycler     cgozstd.WriterRecycler
}

// Close implements rac.CodecWriter.
func (w *CodecWriter) Close() error {
	return w.recycler.Close()
}

// Clone implements rac.CodecWriter.
func (w *CodecWriter) Clone() rac.CodecWriter {
	return &CodecWriter{}
}

// Compress implements rac.CodecWriter.
func (w *CodecWriter) Compress(p []byte, q []byte, resourcesData [][]byte) (
	codec rac.Codec, compressed []byte, secondaryResource int, tertiaryResource int, retErr error) {

	// Compress p+q without any shared dictionary.
	baseline, err := w.compress(p, q, nil)
	if err != nil {
		return 0, nil, 0, 0, err
	}
	return rac.CodecZstandard, baseline, rac.NoResourceUsed, rac.NoResourceUsed, nil
}

func (w *CodecWriter) compress(p []byte, q []byte, dict []byte) ([]byte, error) {
	w.compressed.Reset()
	if w.cachedWriter == nil {
		zw := &cgozstd.Writer{}
		w.cachedWriter = zw
		w.recycler.Bind(zw)
	}
	if err := w.cachedWriter.Reset(&w.compressed, dict, compression.LevelSmall); err != nil {
		return nil, err
	}

	if len(p) > 0 {
		if _, err := w.cachedWriter.Write(p); err != nil {
			w.cachedWriter.Close()
			return nil, err
		}
	}
	if len(q) > 0 {
		if _, err := w.cachedWriter.Write(q); err != nil {
			w.cachedWriter.Close()
			return nil, err
		}
	}

	if err := w.cachedWriter.Close(); err != nil {
		return nil, err
	}
	return w.compressed.Bytes(), nil
}

// CanCut implements rac.CodecWriter.
func (w *CodecWriter) CanCut() bool {
	return false
}

// Cut implements rac.CodecWriter.
func (w *CodecWriter) Cut(codec rac.Codec, encoded []byte, maxEncodedLen int) (encodedLen int, decodedLen int, retErr error) {
	return 0, 0, errCannotCut
}

// WrapResource implements rac.CodecWriter.
func (w *CodecWriter) WrapResource(raw []byte) ([]byte, error) {
	return nil, nil
}
