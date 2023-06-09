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

# This script fetches Blink's web_tests/images/resources suite.

if [ ! -e ../../wuffs-root-directory.txt ]; then
  echo "$0 should be run from the Wuffs test/3pdata directory."
  exit 1
fi

# Check out a specific commit (from June 2023).
git clone --quiet --depth=1 https://github.com/nigeltao/blink-web-tests-images-resources.git
cd blink-web-tests-images-resources
git reset --quiet --hard 9c5a6768184e5b45eaddad8f83ffd87dc5dc8f15
cd ..

# Copy out the resources directory.
rm -rf blinksuite
mv blink-web-tests-images-resources/resources blinksuite
rm -rf blink-web-tests-images-resources
