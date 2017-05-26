// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs-test-c runs directories of C test programs.
package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strings"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()
	failed := false
	for _, arg := range flag.Args() {
		filenames, err := listDir(arg)
		if err != nil {
			return err
		}
		for _, filename := range filenames {
			f, err := do(arg, filename)
			if err != nil {
				return err
			}
			failed = failed || f
		}
	}
	if failed {
		return fmt.Errorf("%s: some tests failed", os.Args[0])
	}
	return nil
}

func listDir(dirname string) (filenames []string, err error) {
	f, err := os.Open(dirname)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	infos, err := f.Readdir(-1)
	if err != nil {
		return nil, err
	}
	for _, o := range infos {
		name := o.Name()
		if strings.HasSuffix(name, ".c") {
			filenames = append(filenames, name)
		}
	}
	sort.Strings(filenames)
	return filenames, nil
}

func do(dirname string, filename string) (failed bool, err error) {
	workDir, err := ioutil.TempDir("", "puffs-test-c")
	if err != nil {
		return false, err
	}
	defer os.RemoveAll(workDir)

	for _, cc := range []string{"clang", "gcc"} {
		in := filepath.Join(dirname, filename)
		out := filepath.Join(workDir, cc+".out")

		ccCmd := exec.Command(cc, "-std=c99", "-Wall", "-Werror", "-o", out, in)
		ccCmd.Stdout = os.Stdout
		ccCmd.Stderr = os.Stderr
		if err := ccCmd.Run(); err != nil {
			return false, err
		}

		outCmd := exec.Command(out)
		outCmd.Stdout = os.Stdout
		outCmd.Stderr = os.Stderr
		if err := outCmd.Run(); err == nil {
			// No-op.
		} else if _, ok := err.(*exec.ExitError); ok {
			failed = true
		} else {
			return false, err
		}
	}
	return failed, nil
}
