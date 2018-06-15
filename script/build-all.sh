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

# This is a simple sanity check that the tests pass and the example and fuzz
# programs compile.

go install github.com/google/wuffs/cmd/...
go test    github.com/google/wuffs/...
wuffs genlib
wuffs test  -skipgen -mimic
wuffs bench -skipgen -mimic -reps=1 -iterscale=1

for f in example/*; do
  echo Building $f
  if [ "$f" = "example/library" ]; then
    gcc -static -I.. $f/*.c gen/lib/c/gcc-static/libwuffs.a -o $f/a.out
  elif [ -e $f/*.c ]; then
    gcc $f/*.c -o $f/a.out
  fi
done

for f in fuzz/c/std/*_fuzzer.c; do
  echo Building $f
  gcc -DWUFFS_CONFIG__FUZZLIB_MAIN $f -o ${f%.c}.out
done
