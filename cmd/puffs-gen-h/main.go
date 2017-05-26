// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs-gen-h transpiles a Puffs program to a C header.
//
// The command line arguments list the source Puffs files. If no arguments are
// given, it reads from stdin.
//
// The generated header is written to stdout.
package main

import (
	"github.com/google/puffs/cmd/internal/cgen"
	"github.com/google/puffs/lang/generate"
)

func main() {
	generate.Main(&cgen.Generator{Extension: 'h'})
}
