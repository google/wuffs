#!/bin/bash -eu
# Copyright 2018 The Wuffs Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ----------------

# See build-all.sh for commentary.

for f in example/*; do
  echo "Building $f"
  if [ "$f" = "example/crc32" ]; then
    # example/crc32 is unusual in that it's C++, not C.
    g++ -O3 $f/*.cc -o $f/a.out
  elif [ "$f" = "example/library" ]; then
    # example/library is unusual in that it uses separately compiled libraries
    # (built by "wuffs genlib" above) instead of directly #include'ing Wuffs'
    # .c files.
    gcc -O3 -static -I.. $f/*.c gen/lib/c/gcc-static/libwuffs.a -o $f/a.out
  elif [ -e $f/*.c ]; then
    gcc -O3 $f/*.c -o $f/a.out
  fi
done
