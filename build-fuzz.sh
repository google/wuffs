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
  sources=fuzz/c/std/*_fuzzer.c
fi

for f in $sources; do
  f=${f%_fuzzer.c}
  f=${f%/}
  f=${f##*/}
  if [ -z $f ]; then
    continue
  fi
  echo "Building gen/bin/fuzz-$f"

  $CC -DWUFFS_CONFIG__FUZZLIB_MAIN fuzz/c/std/${f}_fuzzer.c -o gen/bin/fuzz-$f
done
