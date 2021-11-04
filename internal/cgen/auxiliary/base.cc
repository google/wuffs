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

DynIOBuffer::DynIOBuffer(uint64_t max_incl)
    : m_buf(wuffs_base__empty_io_buffer()), m_max_incl(max_incl) {}

DynIOBuffer::~DynIOBuffer() {
  if (m_buf.data.ptr) {
    free(m_buf.data.ptr);
  }
}

DynIOBuffer::GrowResult  //
DynIOBuffer::grow(uint64_t min_incl) {
  uint64_t n = round_up(min_incl, m_max_incl);
  if (n == 0) {
    return ((min_incl == 0) && (m_max_incl == 0))
               ? DynIOBuffer::GrowResult::OK
               : DynIOBuffer::GrowResult::FailedMaxInclExceeded;
  } else if (n > m_buf.data.len) {
    uint8_t* ptr = static_cast<uint8_t*>(realloc(m_buf.data.ptr, n));
    if (!ptr) {
      return DynIOBuffer::GrowResult::FailedOutOfMemory;
    }
    m_buf.data.ptr = ptr;
    m_buf.data.len = n;
  }
  return DynIOBuffer::GrowResult::OK;
}

// round_up rounds min_incl up, returning the smallest value x satisfying
// (min_incl <= x) and (x <= max_incl) and some other constraints. It returns 0
// if there is no such x.
//
// When max_incl <= 4096, the other constraints are:
//  - (x == max_incl)
//
// When max_incl >  4096, the other constraints are:
//  - (x == max_incl) or (x is a power of 2)
//  - (x >= 4096)
uint64_t  //
DynIOBuffer::round_up(uint64_t min_incl, uint64_t max_incl) {
  if (min_incl > max_incl) {
    return 0;
  }
  uint64_t n = 4096;
  if (n >= max_incl) {
    return max_incl;
  }
  while (n < min_incl) {
    if (n >= (max_incl / 2)) {
      return max_incl;
    }
    n *= 2;
  }
  return n;
}

// --------

Input::~Input() {}

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

namespace private_impl {

const char ErrMsg_MaxInclMetadataLengthExceeded[] =  //
    "wuffs_aux::private_impl: max_incl_metadata_length exceeded";
const char ErrMsg_OutOfMemory[] =  //
    "wuffs_aux::private_impl: out of memory";
const char ErrMsg_UnexpectedEndOfFile[] =  //
    "wuffs_aux::private_impl: unexpected end of file";
const char ErrMsg_UnsupportedMetadata[] =  //
    "wuffs_aux::private_impl: unsupported metadata";
const char ErrMsg_UnsupportedNegativeAdvance[] =  //
    "wuffs_aux::private_impl: unsupported negative advance";

std::string  //
AdvanceIOBufferTo(sync_io::Input& input,
                  IOBuffer& io_buf,
                  uint64_t absolute_position) {
  if (absolute_position < io_buf.reader_position()) {
    return ErrMsg_UnsupportedNegativeAdvance;
  }
  while (true) {
    uint64_t relative_position = absolute_position - io_buf.reader_position();
    if (relative_position <= io_buf.reader_length()) {
      io_buf.meta.ri += (size_t)relative_position;
      break;
    } else if (io_buf.meta.closed) {
      return ErrMsg_UnexpectedEndOfFile;
    }
    io_buf.meta.ri = io_buf.meta.wi;
    if (!input.BringsItsOwnIOBuffer()) {
      io_buf.compact();
    }
    std::string error_message = input.CopyIn(&io_buf);
    if (!error_message.empty()) {
      return error_message;
    }
  }
  return "";
}

std::string  //
HandleMetadata(
    sync_io::Input& input,
    wuffs_base__io_buffer& io_buf,
    uint64_t max_incl_metadata_length,
    wuffs_base__status (*tell_me_more_func)(void*,
                                            wuffs_base__io_buffer*,
                                            wuffs_base__more_information*,
                                            wuffs_base__io_buffer*),
    void* tell_me_more_receiver,
    std::string (*handle_metadata_func)(void*,
                                        const wuffs_base__more_information*,
                                        wuffs_base__slice_u8),
    void* handle_metadata_receiver) {
  wuffs_base__more_information minfo = wuffs_base__empty_more_information();
  sync_io::DynIOBuffer raw(max_incl_metadata_length);

  while (true) {
    minfo = wuffs_base__empty_more_information();
    wuffs_base__status status = (*tell_me_more_func)(
        tell_me_more_receiver, &raw.m_buf, &minfo, &io_buf);
    switch (minfo.flavor) {
      case 0:
      case WUFFS_BASE__MORE_INFORMATION__FLAVOR__METADATA_RAW_TRANSFORM:
      case WUFFS_BASE__MORE_INFORMATION__FLAVOR__METADATA_PARSED:
        break;

      case WUFFS_BASE__MORE_INFORMATION__FLAVOR__METADATA_RAW_PASSTHROUGH: {
        wuffs_base__range_ie_u64 r = minfo.metadata_raw_passthrough__range();
        if (r.is_empty()) {
          break;
        }
        uint64_t num_to_copy = r.length();
        if (num_to_copy > (max_incl_metadata_length - raw.m_buf.meta.wi)) {
          return ErrMsg_MaxInclMetadataLengthExceeded;
        } else if (num_to_copy > (raw.m_buf.data.len - raw.m_buf.meta.wi)) {
          switch (raw.grow(num_to_copy + raw.m_buf.meta.wi)) {
            case sync_io::DynIOBuffer::GrowResult::OK:
              break;
            case sync_io::DynIOBuffer::GrowResult::FailedMaxInclExceeded:
              return ErrMsg_MaxInclMetadataLengthExceeded;
            case sync_io::DynIOBuffer::GrowResult::FailedOutOfMemory:
              return ErrMsg_OutOfMemory;
          }
        }

        if (io_buf.reader_position() > r.min_incl) {
          return ErrMsg_UnsupportedMetadata;
        } else {
          std::string error_message =
              AdvanceIOBufferTo(input, io_buf, r.min_incl);
          if (!error_message.empty()) {
            return error_message;
          }
        }

        for (uint64_t num_to_copy = r.length(); num_to_copy > 0;) {
          uint64_t n =
              wuffs_base__u64__min(num_to_copy, io_buf.reader_length());
          memcpy(raw.m_buf.writer_pointer(), io_buf.reader_pointer(), n);
          raw.m_buf.meta.wi += n;
          io_buf.meta.ri += n;
          num_to_copy -= n;
          if (num_to_copy == 0) {
            break;
          } else if (io_buf.meta.closed) {
            return ErrMsg_UnexpectedEndOfFile;
          } else if (!input.BringsItsOwnIOBuffer()) {
            io_buf.compact();
          }
          std::string error_message = input.CopyIn(&io_buf);
          if (!error_message.empty()) {
            return error_message;
          }
        }
        break;
      }

      default:
        return ErrMsg_UnsupportedMetadata;
    }

    if (status.repr == nullptr) {
      break;
    } else if (status.repr != wuffs_base__suspension__even_more_information) {
      if (status.repr != wuffs_base__suspension__short_write) {
        return status.message();
      }
      switch (raw.grow(wuffs_base__u64__sat_add(raw.m_buf.data.len, 1))) {
        case sync_io::DynIOBuffer::GrowResult::OK:
          break;
        case sync_io::DynIOBuffer::GrowResult::FailedMaxInclExceeded:
          return ErrMsg_MaxInclMetadataLengthExceeded;
        case sync_io::DynIOBuffer::GrowResult::FailedOutOfMemory:
          return ErrMsg_OutOfMemory;
      }
    }
  }

  return (*handle_metadata_func)(handle_metadata_receiver, &minfo,
                                 raw.m_buf.reader_slice());
}

}  // namespace private_impl

}  // namespace wuffs_aux

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__AUX__BASE)
