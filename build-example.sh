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

if [ ! -e release/c/wuffs-unsupported-snapshot.c ]; then
  echo "$0 should be run from the Wuffs root directory."
  exit 1
fi

CC=${CC:-gcc}
CXX=${CXX:-g++}

mkdir -p gen/bin

sources=$@
if [ $# -eq 0 ]; then
  sources=example/*
fi

for f in $sources; do
  f=${f%/}
  f=${f##*/}
  if [ -z $f ]; then
    continue
  fi

  if [ $f = crc32 ]; then
    echo "Building gen/bin/example-$f"
    # example/crc32 is unusual in that it's C++, not C.
    $CXX -O3 example/$f/*.cc -o gen/bin/example-$f
  elif [ $f = library ]; then
    # example/library is unusual in that it uses separately compiled libraries
    # (built by "wuffs genlib", e.g. by running build-all.sh) instead of
    # directly #include'ing Wuffs' .c files.
    if [ -e gen/lib/c/$CC-static/libwuffs.a ]; then
      echo "Building gen/bin/example-$f"
      $CC -O3 -static -I.. example/$f/*.c gen/lib/c/$CC-static/libwuffs.a -o gen/bin/example-$f
    else
      echo "Skipping gen/bin/example-$f; run \"wuffs genlib\" first"
    fi
  elif [ -e example/$f/*.c ]; then
    echo "Building gen/bin/example-$f"
    $CC -O3 example/$f/*.c -o gen/bin/example-$f
  fi
done
