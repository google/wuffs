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

# This script, build-all.sh, is a simple sanity check that the tests pass and
# the example programs (including fuzz programs) compile.
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
wuffs gen

echo "Checking snapshot compiles cleanly (as C)"
gcc -c -Wall -Werror -DWUFFS_IMPLEMENTATION -std=c99 \
    release/c/wuffs-unsupported-snapshot.h -o /dev/null

echo "Checking snapshot compiles cleanly (as C++)"
g++ -c -Wall -Werror -DWUFFS_IMPLEMENTATION -std=c++11 \
    release/c/wuffs-unsupported-snapshot.h -o /dev/null

wuffs genlib -skipgen
wuffs test   -skipgen -mimic
wuffs bench  -skipgen -mimic -reps=1 -iterscale=1

./build-example.sh
./build-fuzz.sh
