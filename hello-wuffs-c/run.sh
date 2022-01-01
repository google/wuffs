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

CC=${CC:-gcc}

# You may need to run
#   go install github.com/google/wuffs/cmd/wuffs-c
# beforehand, to install the wuffs-c compiler.
wuffs-c gen -package_name demo < parse.wuffs > parse.c

echo --- C Implementation Prints ---
$CC main.c naive-parse.c -o n.out
./n.out

echo ------ Wuffs Impl Prints ------
$CC main.c wuffs-parse.c -o w.out
./w.out
