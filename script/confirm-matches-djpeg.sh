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

DJPEG_BINARY=${DJPEG_BINARY:-djpeg}

if [ ! -e wuffs-root-directory.txt ]; then
  echo "$0 should be run from the Wuffs root directory."
  exit 1
elif [ ! -e gen/bin/example-convert-to-nia ]; then
  echo "Run \"./build-example.sh example/convert-to-nia\" first."
  exit 1
elif [ ! -e gen/bin/example-crc32 ]; then
  echo "Run \"./build-example.sh example/crc32\" first."
  exit 1
elif [ -z $(which $DJPEG_BINARY) ]; then
  echo "Could not find DJPEG_BINARY \"$DJPEG_BINARY\"."
  exit 1
elif [[ ! $($DJPEG_BINARY -version 2>&1 >/dev/null) =~ ^libjpeg ]]; then
  echo "Could not get libjpeg version from DJPEG_BINARY \"$DJPEG_BINARY\"."
  exit 1
fi

djpeg_version=$($DJPEG_BINARY -version 2>&1 >/dev/null)
if [[ $djpeg_version =~ ^libjpeg.*version.2\.0 ||
      $djpeg_version =~ ^libjpeg.*version.2\.1 ||
      $djpeg_version =~ ^libjpeg.*version.3\.0\.0 ]]; then
  # "Wuffs's std/jpeg matches djpeg" needs these libjpeg-turbo commits:
  #  - 6d91e950 Use 5x5 win & 9 AC coeffs when smoothing DC scans  (in 2.1.0)
  #  - dbae5928 Fix interblock smoothing with narrow prog. JPEGs   (in 3.0.1)
  #  - 9b704f96 Fix block smoothing w/vert.-subsampled prog. JPEGs (in 3.0.1)
  # https://github.com/libjpeg-turbo/libjpeg-turbo/commit/6d91e950c871103a11bac2f10c63bf998796c719
  # https://github.com/libjpeg-turbo/libjpeg-turbo/commit/dbae59281fdc6b3a6304a40134e8576d50d662c0
  # https://github.com/libjpeg-turbo/libjpeg-turbo/commit/9b704f96b2dccc54363ad7a2fe8e378fc1a2893b
  echo "\"$djpeg_version\" too old from DJPEG_BINARY \"$DJPEG_BINARY\"."
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
  local want=$($DJPEG_BINARY $1 2>/dev/null | gen/bin/example-crc32)
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
