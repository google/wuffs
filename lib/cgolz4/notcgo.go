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

// +build !cgo

package cgolz4

// This file contains placeholder types and funcs so that the package still
// builds (with the same API) when CGO_ENABLED=0. The package doesn't work
// without cgo, but it will fail at run time, not compile time.
//
// In particular, the build stays green regardless of whether CGO_ENABLED is on
// or off. Installing and testing every package in the whole repository will
// not fail. The tests in this package don't pass, but they are skipped.

import (
	"errors"
	"io"

	"github.com/google/wuffs/lib/compression"
)

const cgoEnabled = false

var (
	errCgoIsNotEnabled = errors.New("cgolz4: cgo is not enabled")
)

type ReaderRecycler struct{}

func (c *ReaderRecycler) Bind(*Reader) {}
func (c *ReaderRecycler) Close() error { return errCgoIsNotEnabled }

type Reader struct{}

func (r *Reader) Close() error                  { return errCgoIsNotEnabled }
func (r *Reader) Read([]byte) (int, error)      { return 0, errCgoIsNotEnabled }
func (r *Reader) Reset(io.Reader, []byte) error { return errCgoIsNotEnabled }

type WriterRecycler struct{}

func (c *WriterRecycler) Bind(*Writer) {}
func (c *WriterRecycler) Close() error { return errCgoIsNotEnabled }

type Writer struct{}

func (w *Writer) Close() error                                     { return errCgoIsNotEnabled }
func (w *Writer) Reset(io.Writer, []byte, compression.Level) error { return errCgoIsNotEnabled }
func (w *Writer) Write([]byte) (int, error)                        { return 0, errCgoIsNotEnabled }
