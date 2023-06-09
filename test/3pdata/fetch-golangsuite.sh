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

# This script fetches golang/go's src/image/testdata suite.

if [ ! -e ../../wuffs-root-directory.txt ]; then
  echo "$0 should be run from the Wuffs test/3pdata directory."
  exit 1
fi

# Check out a specific commit (from June 2023).
git clone --quiet --depth=1 https://github.com/nigeltao/golang-go-src-image-testdata.git
cd golang-go-src-image-testdata
git reset --quiet --hard f2f7970f9054b910148be4e0dbda94b392c58700
cd ..

# Copy out the testdata directory.
rm -rf golangsuite
mv golang-go-src-image-testdata/testdata golangsuite
rm -rf golang-go-src-image-testdata
