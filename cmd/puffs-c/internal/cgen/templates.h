// After editing this file, run "go generate" in this directory.

// Copyright 2017 The Puffs Authors.
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

// This file contains C-like code. It is close enough to C so that clang-format
// can format it well. But it is not C code per se. It contains templates that
// the code generator expands via a custom mechanism.
//
// This file has a .h extension, not a .c extension, so that Go tools such as
// "go generate" don't try to build it as part of the Go package, via cgo.

template short_read(string qPKGPREFIXq, string qnameq) {
short_read_qnameq:
  if (a_qnameq.buf && a_qnameq.buf->closed && !a_qnameq.limit.ptr_to_len &&
      !a_qnameq.use_limitt) {
    status = qPKGPREFIXqERROR_UNEXPECTED_EOF;
    goto exit;
  }
  status = qPKGPREFIXqSUSPENSION_SHORT_READ;
  goto suspend;
}
