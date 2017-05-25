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
	"strings"
)

var commands = []struct {
	name string
	do   func(puffsRoot string, args []string) error
}{
	{"gen", doGen},
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

func readdir(dirname string) ([]os.FileInfo, error) {
	f, err := os.Open(dirname)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	return f.Readdir(-1)
}

func usage() {
	fmt.Fprintf(os.Stderr, `Puffs is a tool for managing Puffs source code.

Usage:

	puffs command [arguments]

The commands are:

	gen     generate code for packages and dependencies
`)
}

const langsUsage = `comma-separated list of target languages, e.g. "c,go,rs"`

func parseLangs(commaSeparated string) ([]string, error) {
	ret := []string(nil)
	for _, s := range strings.Split(commaSeparated, ",") {
		if len(s) == 0 {
			return nil, fmt.Errorf(`invalid empty lang ""`)
		}
		for _, c := range s {
			if ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') {
				continue
			}
			return nil, fmt.Errorf(`invalid lang %q, not in [a-z0-9]+`, s)
		}
		ret = append(ret, s)
	}
	return ret, nil
}
