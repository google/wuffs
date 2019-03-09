// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// wuffs-c handles the C language specific parts of the wuffs tool.
package main

import (
	"fmt"
	"os"

	"github.com/google/wuffs/internal/cgen"
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
	case "genrelease":
		return doGenrelease(args)
	case "test":
		return doTest(args)
	}
	return fmt.Errorf("bad sub-command %q", os.Args[1])
}
