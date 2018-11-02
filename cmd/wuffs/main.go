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
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"github.com/google/wuffs/lang/wuffsroot"
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

	wuffsRoot, err := wuffsroot.Value()
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

func findFiles(qualDirname string, suffix string) (filenames []string, err error) {
	filenames, err = findFiles1(nil, qualDirname, suffix)
	if err != nil {
		return nil, err
	}
	sort.Strings(filenames)
	return filenames, nil
}

func findFiles1(dstQF []string, qualDirname string, suffix string) (qualFilenames []string, err error) {
	dstQF, relDirnames, err := appendDir(dstQF, qualDirname, suffix, true)
	if err != nil {
		return nil, err
	}
	for _, d := range relDirnames {
		dstQF, err = findFiles1(dstQF, filepath.Join(qualDirname, d), suffix)
		if err != nil {
			return nil, err
		}
	}
	return dstQF, nil
}

func listDir(qualDirname string, suffix string, returnSubdirs bool) (qualFilenames []string, relDirnames []string, err error) {
	qualFilenames, relDirnames, err = appendDir(nil, qualDirname, suffix, returnSubdirs)
	if err != nil {
		return nil, nil, err
	}
	sort.Strings(qualFilenames)
	sort.Strings(relDirnames)
	return qualFilenames, relDirnames, nil
}

func appendDir(dstQF []string, qualDirname string, suffix string, returnSubdirs bool) (qualFilenames []string, relDirnames []string, err error) {
	f, err := os.Open(qualDirname)
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
				relDirnames = append(relDirnames, name)
			}
		} else if strings.HasSuffix(name, suffix) {
			dstQF = append(dstQF, filepath.Join(qualDirname, name))
		}
	}

	return dstQF, relDirnames, nil
}

func writeFile(filename string, contents []byte) error {
	if existing, err := ioutil.ReadFile(filename); err == nil && bytes.Equal(existing, contents) {
		fmt.Println("gen unchanged: ", filename)
		return nil
	}
	if err := os.MkdirAll(filepath.Dir(filename), 0755); err != nil {
		return err
	}
	if err := ioutil.WriteFile(filename, contents, 0644); err != nil {
		return err
	}
	fmt.Println("gen wrote:     ", filename)
	return nil
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
