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
// https://github.com/google/wuffs/blob/main/doc/note/auxiliary-code.md

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__AUX__BASE)

namespace wuffs_aux {

namespace sync_io {

// --------

IOBuffer*  //
Input::BringsItsOwnIOBuffer() {
  return nullptr;
}

// --------

FileInput::FileInput(FILE* f) : m_f(f) {}

std::string  //
FileInput::CopyIn(IOBuffer* dst) {
  if (!m_f) {
    return "wuffs_aux::sync_io::FileInput: nullptr file";
  } else if (!dst) {
    return "wuffs_aux::sync_io::FileInput: nullptr IOBuffer";
  } else if (dst->meta.closed) {
    return "wuffs_aux::sync_io::FileInput: end of file";
  } else {
    dst->compact();
    size_t n = fread(dst->writer_pointer(), 1, dst->writer_length(), m_f);
    dst->meta.wi += n;
    dst->meta.closed = feof(m_f);
    if (ferror(m_f)) {
      return "wuffs_aux::sync_io::FileInput: error reading file";
    }
  }
  return "";
}

// --------

MemoryInput::MemoryInput(const char* ptr, size_t len)
    : m_io(wuffs_base__ptr_u8__reader(
          static_cast<uint8_t*>(static_cast<void*>(const_cast<char*>(ptr))),
          len,
          true)) {}

MemoryInput::MemoryInput(const uint8_t* ptr, size_t len)
    : m_io(wuffs_base__ptr_u8__reader(const_cast<uint8_t*>(ptr), len, true)) {}

IOBuffer*  //
MemoryInput::BringsItsOwnIOBuffer() {
  return &m_io;
}

std::string  //
MemoryInput::CopyIn(IOBuffer* dst) {
  if (!dst) {
    return "wuffs_aux::sync_io::MemoryInput: nullptr IOBuffer";
  } else if (dst->meta.closed) {
    return "wuffs_aux::sync_io::MemoryInput: end of file";
  } else if (wuffs_base__slice_u8__overlaps(dst->data, m_io.data)) {
    // Treat m_io's data as immutable, so don't compact dst or otherwise write
    // to it.
    return "wuffs_aux::sync_io::MemoryInput: overlapping buffers";
  } else {
    dst->compact();
    size_t nd = dst->writer_length();
    size_t ns = m_io.reader_length();
    size_t n = (nd < ns) ? nd : ns;
    memcpy(dst->writer_pointer(), m_io.reader_pointer(), n);
    m_io.meta.ri += n;
    dst->meta.wi += n;
    dst->meta.closed = m_io.reader_length() == 0;
  }
  return "";
}

// --------

}  // namespace sync_io

}  // namespace wuffs_aux

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__AUX__BASE)
