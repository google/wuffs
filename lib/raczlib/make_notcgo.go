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

// +build !cgo

package raczlib

import (
	"compress/zlib"
	"io"

	"github.com/google/wuffs/lib/rac"
)

// MakeDecompressor implements rac.CodecReader.
func (r *CodecReader) MakeDecompressor(compressed io.Reader, rctx rac.ReaderContext) (io.Reader, error) {
	if r.cachedZlibReader != nil {
		if err := r.cachedZlibReader.Reset(compressed, rctx.Secondary); err != nil {
			return nil, err
		}
		return r.cachedZlibReader, nil
	}

	zlibReader, err := zlib.NewReaderDict(compressed, rctx.Secondary)
	if err != nil {
		return nil, err
	}
	r.cachedZlibReader = zlibReader.(resetReadCloser)
	return zlibReader, nil
}
