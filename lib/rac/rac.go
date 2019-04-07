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
// sibling "raczlib" package (TODO).
//
// The RAC specification is at
// https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md
package rac

const (
	// MaxCSize is the maximum RAC file size (in both CSpace and DSpace).
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
