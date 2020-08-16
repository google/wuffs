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

// ---------------- Auxiliary - CBOR

namespace wuffs_aux {

struct DecodeCborResult {
  DecodeCborResult(std::string&& error_message0, uint64_t cursor_position0);

  std::string error_message;
  uint64_t cursor_position;
};

class DecodeCborCallbacks {
 public:
  // AppendXxx are called for leaf nodes: literals, numbers, strings, etc.

  virtual std::string AppendNull() = 0;
  virtual std::string AppendUndefined() = 0;
  virtual std::string AppendBool(bool val) = 0;
  virtual std::string AppendF64(double val) = 0;
  virtual std::string AppendI64(int64_t val) = 0;
  virtual std::string AppendU64(uint64_t val) = 0;
  virtual std::string AppendByteString(std::string&& val) = 0;
  virtual std::string AppendTextString(std::string&& val) = 0;
  virtual std::string AppendMinus1MinusX(uint64_t val) = 0;
  virtual std::string AppendCborSimpleValue(uint8_t val) = 0;
  virtual std::string AppendCborTag(uint64_t val) = 0;

  // Push and Pop are called for container nodes: CBOR arrays (lists) and CBOR
  // maps (dictionaries).
  //
  // The flags bits combine exactly one of:
  //  - WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_NONE
  //  - WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST
  //  - WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_DICT
  // and exactly one of:
  //  - WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_NONE
  //  - WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST
  //  - WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_DICT

  virtual std::string Push(uint32_t flags) = 0;
  virtual std::string Pop(uint32_t flags) = 0;

  // Done is always the last Callback method called by DecodeCbor, whether or
  // not parsing the input as CBOR encountered an error. Even when successful,
  // trailing data may remain in input and buffer.
  //
  // Do not keep a reference to buffer or buffer.data.ptr after Done returns,
  // as DecodeCbor may then de-allocate the backing array.
  //
  // The default Done implementation is a no-op.
  virtual void Done(DecodeCborResult& result,
                    sync_io::Input& input,
                    IOBuffer& buffer);
};

// DecodeCbor calls callbacks based on the CBOR-formatted data in input.
//
// On success, the returned error_message is empty and cursor_position counts
// the number of bytes consumed. On failure, error_message is non-empty and
// cursor_position is the location of the error. That error may be a content
// error (invalid CBOR) or an input error (e.g. network failure).
DecodeCborResult DecodeCbor(
    DecodeCborCallbacks& callbacks,
    sync_io::Input& input,
    wuffs_base__slice_u32 quirks = wuffs_base__empty_slice_u32());

}  // namespace wuffs_aux
