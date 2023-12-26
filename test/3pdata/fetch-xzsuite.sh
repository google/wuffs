#!/bin/bash -eu
# Copyright 2023 The Wuffs Authors.
#
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

# ----------------

# This script fetches xz's tests/files suite.

if [ ! -e ../../wuffs-root-directory.txt ]; then
  echo "$0 should be run from the Wuffs test/3pdata directory."
  exit 1
fi

# Check out a specific commit (from December 2023).
git clone --quiet https://github.com/nigeltao/xz-tests-files.git
cd xz-tests-files
git reset --quiet --hard 6c3e22469300d284182159b5ae7db9e959dfe62e
cd ..

# Trim the git metadata.
rm -rf xzsuite
rm -rf xz-tests-files/.git
mv xz-tests-files xzsuite
