// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs is a tool for managing Puffs source code.
package main

import (
	"errors"
	"flag"
	"fmt"
	"go/build"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

var commands = []struct {
	name string
	do   func(puffsRoot string, args []string) error
}{
	{"bench", doBench},
	{"gen", doGen},
	{"test", doTest},
}

func usage() {
	fmt.Fprintf(os.Stderr, `Puffs is a tool for managing Puffs source code.

Usage:

	puffs command [arguments]

The commands are:

	bench   benchmark packages
	gen     generate code for packages and dependencies
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

	puffsRoot, err := findPuffsRoot()
	if err != nil {
		return err
	}
	if args := flag.Args(); len(args) > 0 {
		for _, c := range commands {
			if args[0] == c.name {
				return c.do(puffsRoot, args[1:])
			}
		}
	}
	usage()
	os.Exit(1)
	return nil
}

func findPuffsRoot() (string, error) {
	// TODO: look for a PUFFSROOT environment variable?
	for _, p := range filepath.SplitList(build.Default.GOPATH) {
		p = filepath.Join(p, "src", "github.com", "google", "puffs")
		if o, err := os.Stat(p); err == nil && o.IsDir() {
			return p, nil
		}
	}
	return "", errors.New("could not find Puffs root directory")
}

func listDir(puffsRoot string, dirname string, returnSubdirs bool) (filenames []string, dirnames []string, err error) {
	f, err := os.Open(filepath.Join(puffsRoot, filepath.FromSlash(dirname)))
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
		} else if strings.HasSuffix(name, ".puffs") {
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

	mimicDefault = false
	mimicUsage   = `whether to compare Puffs' output with other libraries' output`
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
