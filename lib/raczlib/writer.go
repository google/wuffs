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
	"bytes"
	"compress/zlib"
	"errors"

	"github.com/google/wuffs/lib/rac"
	"github.com/google/wuffs/lib/zlibcut"
)

var (
	errInvalidCodec = errors.New("raczlib: invalid codec")
)

// CodecWriter specializes a rac.Writer to encode Zlib-compressed chunks.
type CodecWriter struct {
	compressed bytes.Buffer
	zlibWriter *zlib.Writer
}

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

func (w *CodecWriter) Cut(codec rac.Codec, encoded []byte, maxEncodedLen int) (encodedLen int, decodedLen int, retErr error) {
	if codec != rac.CodecZlib {
		return 0, 0, errInvalidCodec
	}
	return zlibcut.Cut(nil, encoded, maxEncodedLen)
}
