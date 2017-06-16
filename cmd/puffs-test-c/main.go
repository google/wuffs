// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs-test-c runs C test programs.
package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
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
		f, err := do(arg)
		if err != nil {
			return err
		}
		failed = failed || f
	}
	if failed {
		return fmt.Errorf("%s: some tests failed", os.Args[0])
	}
	return nil
}

func do(filename string) (failed bool, err error) {
	workDir, err := ioutil.TempDir("", "puffs-test-c")
	if err != nil {
		return false, err
	}
	defer os.RemoveAll(workDir)

	for _, cc := range []string{"clang", "gcc"} {
		in := filename + ".c"
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
		outCmd.Dir = filepath.Dir(filename)
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
