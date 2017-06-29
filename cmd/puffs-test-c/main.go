// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs-test-c runs C benchmark and test programs.
package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
)

var (
	bench = flag.Bool("b", false, "whether to run benchmarks instead of regular tests")
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

		ccArgs := []string(nil)
		if *bench {
			ccArgs = append(ccArgs, "-O3")
		} else {
			// TODO: set these flags even if we pass -O3.
			ccArgs = append(ccArgs, "-Wall", "-Werror")
		}
		ccArgs = append(ccArgs, "-std=c99", "-o", out, in)
		ccCmd := exec.Command(cc, ccArgs...)
		ccCmd.Stdout = os.Stdout
		ccCmd.Stderr = os.Stderr
		if err := ccCmd.Run(); err != nil {
			return false, err
		}

		outArgs := []string(nil)
		if *bench {
			outArgs = append(outArgs, "-b")
		}
		outCmd := exec.Command(out, outArgs...)
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
