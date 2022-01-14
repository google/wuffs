#!/bin/bash -eu
# Copyright 2022 The Wuffs Authors.
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

# This script fetches the TrueVision TGA test suite.

if [ ! -e ../../wuffs-root-directory.txt ]; then
  echo "$0 should be run from the Wuffs test/3pdata directory."
  exit 1
fi

# Check out a specific commit (from January 2022).
git clone --quiet https://github.com/image-rs/image.git
cd image
git reset --quiet --hard 2e1d2e5928077eae7a92f8b5cac83ca1ae0db0cd
cd ..

# Copy out the *.tga files.
mkdir -p tgasuite
cp image/tests/images/tga/testsuite/*.tga  tgasuite
cp image/tests/regression/tga/*.tga        tgasuite
rm -rf image
