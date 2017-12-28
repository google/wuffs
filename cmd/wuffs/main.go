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

// wuffs is a tool for managing Wuffs source code.
package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/google/wuffs/lang/generate"
)

var commands = []struct {
	name string
	do   func(wuffsRoot string, args []string) error
}{
	{"bench", doBench},
	{"gen", doGen},
	{"genlib", doGenlib},
	{"test", doTest},
}

func usage() {
	fmt.Fprintf(os.Stderr, `Wuffs is a tool for managing Wuffs source code.

Usage:

	wuffs command [arguments]

The commands are:

	bench   benchmark packages
	gen     generate code for packages and dependencies
	genlib  generate software libraries
	test    test packages
`)
}

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Usage = usage
	flag.Parse()

	wuffsRoot, err := generate.WuffsRoot()
	if err != nil {
		return err
	}
	if args := flag.Args(); len(args) > 0 {
		for _, c := range commands {
			if args[0] == c.name {
				return c.do(wuffsRoot, args[1:])
			}
		}
	}
	usage()
	os.Exit(1)
	return nil
}

func listDir(wuffsRoot string, dirname string, returnSubdirs bool) (filenames []string, dirnames []string, err error) {
	f, err := os.Open(filepath.Join(wuffsRoot, filepath.FromSlash(dirname)))
	if err != nil {
		return nil, nil, err
	}
	defer f.Close()

	infos, err := f.Readdir(-1)
	if err != nil {
		return nil, nil, err
	}
	for _, o := range infos {
		name := o.Name()
		if o.IsDir() {
			if returnSubdirs {
				dirnames = append(dirnames, name)
			}
		} else if strings.HasSuffix(name, ".wuffs") {
			filenames = append(filenames, name)
		}
	}

	sort.Strings(filenames)
	sort.Strings(dirnames)
	return filenames, dirnames, nil
}

const (
	langsDefault = "c"
	langsUsage   = `comma-separated list of target languages (file extensions), e.g. "c,go,rs"`

	skipgenDefault = false
	skipgenUsage   = `whether to skip automatically generating code when testing`

	skipgendepsDefault = false
	skipgendepsUsage   = `whether to skip automatically generating packages' dependencies`
)

func parseLangs(commaSeparated string) ([]string, error) {
	ret := []string(nil)
	for _, s := range strings.Split(commaSeparated, ",") {
		if !validName(s) {
			return nil, fmt.Errorf(`invalid lang %q, not in [a-z0-9]+`, s)
		}
		ret = append(ret, s)
	}
	return ret, nil
}

func validName(s string) bool {
	if len(s) == 0 {
		return false
	}
	for _, c := range s {
		if (c < '0' || '9' < c) && (c < 'a' || 'z' < c) {
			return false
		}
	}
	return true
}
