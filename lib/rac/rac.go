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

// Package rac provides access to RAC (Random Access Compression) files.
//
// RAC is just a container format, and this package ("rac") is a relatively
// low-level. Users will typically want to process a particular compression
// format wrapped in RAC, such as (RAC + Zlib). For that, look at e.g. the
// sibling "raczlib" package.
//
// The RAC specification is at
// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md
package rac

import (
	"errors"
)

const (
	// MaxSize is the maximum RAC file size (in both CSpace and DSpace).
	MaxSize = (1 << 48) - 1

	// invalidCOffsetCLength is ((1 << 64) - 1).
	invalidCOffsetCLength = 0xFFFFFFFFFFFFFFFF
)

// Codec is the compression codec for the RAC file.
//
// See the RAC specification for further discussion.
type Codec uint8

const (
	CodecZlib      = Codec(0x01)
	CodecBrotli    = Codec(0x02)
	CodecZstandard = Codec(0x04)
)

// IndexLocation is whether the index is at the start or end of the RAC file.
//
// See the RAC specification for further discussion.
type IndexLocation uint8

const magic = "\x72\xC3\x63"

const (
	IndexLocationAtEnd   = IndexLocation(0)
	IndexLocationAtStart = IndexLocation(1)
)

var indexLocationAtEndMagic = []byte("\x72\xC3\x63\x00")

// OptResource is an option type, optionally holding a Writer-specific
// identifier for a shared resource.
//
// Zero means that the option is not taken: no shared resource is used.
type OptResource uint32

var (
	errCChunkSizeIsTooSmall          = errors.New("rac: CChunkSize is too small")
	errILAEndTempFile                = errors.New("rac: IndexLocationAtEnd requires a nil TempFile")
	errILAStartTempFile              = errors.New("rac: IndexLocationAtStart requires a non-nil TempFile")
	errInconsistentCompressedSize    = errors.New("rac: inconsistent compressed size")
	errInvalidCPageSize              = errors.New("rac: invalid CPageSize")
	errInvalidChunk                  = errors.New("rac: invalid chunk")
	errInvalidChunkTooLarge          = errors.New("rac: invalid chunk (too large)")
	errInvalidChunkTruncated         = errors.New("rac: invalid chunk (truncated)")
	errInvalidCodec                  = errors.New("rac: invalid Codec")
	errInvalidCodecWriter            = errors.New("rac: invalid CodecWriter")
	errInvalidCompressedSize         = errors.New("rac: invalid CompressedSize")
	errInvalidIndexNode              = errors.New("rac: invalid index node")
	errInvalidInputMissingMagicBytes = errors.New("rac: invalid input: missing magic bytes")
	errInvalidInputMissingRootNode   = errors.New("rac: invalid input: missing root node")
	errInvalidReadSeeker             = errors.New("rac: invalid ReadSeeker")
	errInvalidWriter                 = errors.New("rac: invalid Writer")
	errSeekToInvalidWhence           = errors.New("rac: seek to invalid whence")
	errSeekToNegativePosition        = errors.New("rac: seek to negative position")
	errTooManyChunks                 = errors.New("rac: too many chunks")
	errTooManyResources              = errors.New("rac: too many resources")
	errTooMuchInput                  = errors.New("rac: too much input")
	errUnsupportedRACFileVersion     = errors.New("rac: unsupported RAC file version")
	errWriterIsClosed                = errors.New("rac: Writer is closed")

	errInternalArityIsTooLarge      = errors.New("rac: internal error: arity is too large")
	errInternalInconsistentArity    = errors.New("rac: internal error: inconsistent arity")
	errInternalInconsistentPosition = errors.New("rac: internal error: inconsistent position")
	errInternalShortCSize           = errors.New("rac: internal error: short CSize")
)
