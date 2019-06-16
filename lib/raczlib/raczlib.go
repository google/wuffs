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
	"github.com/google/wuffs/lib/rac"
)

const (
	// MaxSize is the maximum RAC file size (in both CSpace and DSpace).
	MaxSize = rac.MaxSize
)

// IndexLocation is whether the index is at the start or end of the RAC file.
//
// See the RAC specification for further discussion.
type IndexLocation = rac.IndexLocation

const (
	IndexLocationAtEnd   = rac.IndexLocationAtEnd
	IndexLocationAtStart = rac.IndexLocationAtStart
)
