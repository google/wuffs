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

// ---------------- Auxiliary - JSON

namespace wuffs_aux {

struct DecodeJsonResult {
  DecodeJsonResult(std::string&& error_message0, uint64_t cursor_position0);

  std::string error_message;
  uint64_t cursor_position;
};

class DecodeJsonCallbacks {
 public:
  // AppendXxx are called for leaf nodes: literals, numbers and strings. For
  // strings, the Callbacks implementation is responsible for tracking map keys
  // versus other values.

  virtual std::string AppendNull() = 0;
  virtual std::string AppendBool(bool val) = 0;
  virtual std::string AppendF64(double val) = 0;
  virtual std::string AppendI64(int64_t val) = 0;
  virtual std::string AppendTextString(std::string&& val) = 0;

  // Push and Pop are called for container nodes: JSON arrays (lists) and JSON
  // objects (dictionaries).
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

  // Done is always the last Callback method called by DecodeJson, whether or
  // not parsing the input as JSON encountered an error. Even when successful,
  // trailing data may remain in input and buffer. See "Unintuitive JSON
  // Parsing" (https://nullprogram.com/blog/2019/12/28/) which discusses JSON
  // parsing and when it stops.
  //
  // Do not keep a reference to buffer or buffer.data.ptr after Done returns,
  // as DecodeJson may then de-allocate the backing array.
  //
  // The default Done implementation is a no-op.
  virtual void Done(DecodeJsonResult& result,
                    sync_io::Input& input,
                    IOBuffer& buffer);
};

extern const char DecodeJson_BadJsonPointer[];
extern const char DecodeJson_NoMatch[];

// DecodeJson calls callbacks based on the JSON-formatted data in input.
//
// On success, the returned error_message is empty and cursor_position counts
// the number of bytes consumed. On failure, error_message is non-empty and
// cursor_position is the location of the error. That error may be a content
// error (invalid JSON) or an input error (e.g. network failure).
//
// json_pointer is a query in the JSON Pointer (RFC 6901) syntax. The callbacks
// run for the input's sub-node that matches the query. DecodeJson_NoMatch is
// returned if no matching sub-node was found. The empty query matches the
// input's root node, consistent with JSON Pointer semantics.
//
// The JSON Pointer implementation is greedy: duplicate keys are not rejected
// but only the first match for each '/'-separated fragment is followed.
DecodeJsonResult DecodeJson(
    DecodeJsonCallbacks& callbacks,
    sync_io::Input& input,
    wuffs_base__slice_u32 quirks = wuffs_base__empty_slice_u32(),
    std::string json_pointer = std::string());

}  // namespace wuffs_aux
