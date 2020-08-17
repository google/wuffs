// After editing this file, run "go generate" in the ../data directory.

// Copyright 2020 The Wuffs Authors.
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

// ---------------- Auxiliary - Base

// Auxiliary code is discussed at
// https://github.com/google/wuffs/blob/master/doc/note/auxiliary-code.md

#include <stdio.h>

#include <string>

namespace wuffs_aux {

using IOBuffer = wuffs_base__io_buffer;

namespace sync_io {

// --------

class Input {
 public:
  virtual IOBuffer* BringsItsOwnIOBuffer();
  virtual std::string CopyIn(IOBuffer* dst) = 0;
};

// --------

// FileInput is an Input that reads from a file source.
//
// It does not take responsibility for closing the file when done.
class FileInput : public Input {
 public:
  FileInput(FILE* f);

  virtual std::string CopyIn(IOBuffer* dst);

 private:
  FILE* m_f;

  // Delete the copy and assign constructors.
  FileInput(const FileInput&) = delete;
  FileInput& operator=(const FileInput&) = delete;
};

// --------

// MemoryInput is an Input that reads from an in-memory source.
//
// It does not take responsibility for freeing the memory when done.
class MemoryInput : public Input {
 public:
  MemoryInput(const char* ptr, size_t len);
  MemoryInput(const uint8_t* ptr, size_t len);

  virtual IOBuffer* BringsItsOwnIOBuffer();
  virtual std::string CopyIn(IOBuffer* dst);

 private:
  IOBuffer m_io;

  // Delete the copy and assign constructors.
  MemoryInput(const MemoryInput&) = delete;
  MemoryInput& operator=(const MemoryInput&) = delete;
};

// --------

}  // namespace sync_io

}  // namespace wuffs_aux
