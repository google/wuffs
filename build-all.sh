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

# This script, build-all.sh, is a simple sanity check that the tests pass and
# the example and fuzz programs compile. Despite the "all" in the script name,
# it does not build the release editions, as building a release is a separate
# process from e.g. checking that all tests pass before pushing commits.
#
# If you're just looking to get started with Wuffs, running this script isn't
# necessary (as Wuffs doesn't have the "configure; make; make install" dance or
# its equivalent), although studying this script can help you learn how the
# pieces fit together.
#
# For example, this script builds command line tools, such as 'wuffs' and
# 'wuffs-fmt', that are used when *developing* Wuffs, but aren't necessary if
# all you want to do is *use* Wuffs as a third party library. In the latter
# case, the only files you need are those under the release/ directory.
#
# Instead of running this script, you should be able to run the example
# programs (except the 'example/library' special case) out of the box, without
# having to separately configure, build or install a library:
#
# git clone https://github.com/google/wuffs.git
# cd wuffs/example/zcat
# gcc zcat.c
# ./a.out < ../../test/data/romeo.txt.gz

go install github.com/google/wuffs/cmd/...
go test    github.com/google/wuffs/...
wuffs genlib
wuffs test  -skipgen -mimic
wuffs bench -skipgen -mimic -reps=1 -iterscale=1

for f in example/*; do
  echo Building $f
  if [ "$f" = "example/crc32" ]; then
    # example/crc32 is unusual in that it's C++, not C.
    g++ $f/*.cc -o $f/a.out
  elif [ "$f" = "example/library" ]; then
    # example/library is unusual in that it uses separately compiled libraries
    # instead of directly #include'ing Wuffs' .c files.
    gcc -Wall -Werror -static -I.. $f/*.c gen/lib/c/gcc-static/libwuffs.a -o $f/a.out
  elif [ -e $f/*.c ]; then
    gcc -Wall -Werror $f/*.c -o $f/a.out
  fi
done

for f in fuzz/c/std/*_fuzzer.c; do
  echo Building $f
  gcc -DWUFFS_CONFIG__FUZZLIB_MAIN $f -o ${f%.c}.out
done
