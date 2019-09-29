#!/bin/bash -eu
# Copyright 2019 The Wuffs Authors.
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

# This script measures Wuffs benchmarks over recent commits. For example:
#
# PACKAGE=deflate FOCUS=wuffs_deflate_decode_100k script/bench-history.sh

num_commits=${NUM_COMMITS:-100}
cc=${CC:-gcc}
package=${PACKAGE:-gif}
focus=${FOCUS:-wuffs_gif_decode_1000k_full_init}
iterscale=${ITERSCALE:-50}
reps=${REPS:-20}

# ----

save=`git log -1 --pretty=format:"%H"`

i=0
while [ $i -lt $num_commits ]; do
  this_hash=`git log -1 --pretty=format:"%h"`
  $cc -O3 -o bench-history.out test/c/std/$package.c
  this_metric=`./bench-history.out -bench -focus=$focus -iterscale=$iterscale -reps=$reps | benchstat /dev/stdin | sed -ne '/^name.*speed$/,$ p' | grep $focus`
  echo $this_hash $this_metric
  git checkout --quiet HEAD^
  i=$((i + 1))
done

rm -f ./bench-history.out
git checkout --quiet $save
