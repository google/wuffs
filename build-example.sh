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
CFLAGS=${CFLAGS:--O3 -fdata-sections -ffunction-sections -Wl,--gc-sections}
CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:--O3 -fdata-sections -ffunction-sections -Wl,--gc-sections}
LDFLAGS=${LDFLAGS:-}

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

  if [ $f = imageviewer ]; then
    # example/imageviewer is unusual in that needs additional libraries.
    echo "Building (C++) gen/bin/example-$f"
    $CXX $CXXFLAGS example/$f/*.cc \
        $LDFLAGS -lxcb -lxcb-image -lxcb-render -lxcb-render-util \
        -o gen/bin/example-$f
  elif [ $f = "sdl-imageviewer" ]; then
    # example/sdl-imageviewer is unusual in that needs additional libraries.
    echo "Building (C++) gen/bin/example-$f"
    $CXX $CXXFLAGS example/$f/*.cc \
        $LDFLAGS -lSDL2 -lSDL2_image \
        -o gen/bin/example-$f
  elif [ $f = "toy-genlib" ]; then
    # example/toy-genlib is unusual in that it uses separately compiled
    # libraries (built by "wuffs genlib", e.g. by running build-all.sh) instead
    # of directly #include'ing Wuffs' .c files.
    if [ -e gen/lib/c/$CC-static/libwuffs.a ]; then
      echo "Building (C)   gen/bin/example-$f"
      $CC $CFLAGS -static -I.. example/$f/*.c gen/lib/c/$CC-static/libwuffs.a \
          $LDFLAGS -o gen/bin/example-$f
    else
      echo "Skipping gen/bin/example-$f; run \"wuffs genlib\" first"
    fi
  elif [ -e example/$f/*.c ]; then
    echo "Building (C)   gen/bin/example-$f"
    $CC  $CFLAGS              example/$f/*.c  $LDFLAGS -o gen/bin/example-$f
  elif [ $f = "jsonfindptrs" ]; then
    echo "Building (C++) gen/bin/example-$f"
    $CXX $CXXFLAGS -std=c++17 example/$f/*.cc $LDFLAGS -o gen/bin/example-$f
  elif [ -e example/$f/*.cc ]; then
    echo "Building (C++) gen/bin/example-$f"
    $CXX $CXXFLAGS            example/$f/*.cc $LDFLAGS -o gen/bin/example-$f
  fi
done
