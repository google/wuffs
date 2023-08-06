#!/bin/bash -eu
# Copyright 2023 The Wuffs Authors.
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

# This script checks that Wuffs's JPEG decoder and djpeg (from libjpeg or
# libjpeg-turbo) produce the same output (using djpeg's default configuration,
# with no further command line arguments).

if [ ! -e wuffs-root-directory.txt ]; then
  echo "$0 should be run from the Wuffs root directory."
  exit 1
elif [ ! -e gen/bin/example-convert-to-nia ]; then
  echo "Run \"./build-example.sh example/convert-to-nia\" first."
  exit 1
elif [ ! -e gen/bin/example-crc32 ]; then
  echo "Run \"./build-example.sh example/crc32\" first."
  exit 1
elif [ -z $(which djpeg) ]; then
  echo "Could not find \"djpeg\"."
  exit 1
fi

sources=$@
if [ $# -eq 0 ]; then
  sources=test/data/*.jpeg
fi

# ----

result=0

handle() {
  local have=$(gen/bin/example-convert-to-nia -output-netpbm <$1 2>/dev/null | gen/bin/example-crc32)
  local want=$(djpeg $1 2>/dev/null | gen/bin/example-crc32)
  if [ "$want" != "00000000" ]; then
    if [ "$want" == "$have" ]; then
      echo "Match   $1"
    else
      echo "Differ  $1"
      result=1
    fi
  fi
}

# ----

for f in $sources; do
  if [ -f $f ]; then
    handle $f
  elif [ -d $f ]; then
    for g in `find $f -type f | LANG=C sort`; do
      handle $g
    done
  else
    echo "Could not open $f"
    exit 1
  fi
done

exit $result
