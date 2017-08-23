// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs-c handles the C language specific parts of the puffs tool.
package main

import (
	"fmt"
	"os"

	"github.com/google/puffs/cmd/puffs-c/internal/cgen"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	if len(os.Args) < 2 {
		return fmt.Errorf("no sub-command given")
	}
	args := os.Args[2:]
	switch os.Args[1] {
	case "bench":
		return doBench(args)
	case "gen":
		return cgen.Do(args)
	case "genlib":
		return doGenlib(args)
	case "test":
		return doTest(args)
	}
	return fmt.Errorf("bad sub-command %q", os.Args[1])
}
