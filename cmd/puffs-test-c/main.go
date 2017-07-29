// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs-test-c runs C benchmark and test programs.
package main

import (
	"bufio"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

const (
	mimicDefault = false
	mimicUsage   = `whether to compare Puffs' output with other libraries' output`

	repsDefault = 5
	repsMin     = 0
	repsMax     = 1000000
	repsUsage   = `the number of repetitions per benchmark`
)

var (
	benchFlag = flag.Bool("bench", false, "whether to run benchmarks instead of regular tests")
	mimicFlag = flag.Bool("mimic", mimicDefault, mimicUsage)
	repsFlag  = flag.Int("reps", repsDefault, repsUsage)
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()
	if *repsFlag < repsMin || repsMax < *repsFlag {
		return fmt.Errorf("bad -reps flag value %d, outside the range [%d..%d]", *repsFlag, repsMin, repsMax)
	}

	failed := false
	for _, arg := range flag.Args() {
		f, err := do(arg)
		if err != nil {
			return err
		}
		failed = failed || f
	}
	if failed {
		s := "tests"
		if *benchFlag {
			s = "benchmarks"
		}
		return fmt.Errorf("%s: some %s failed", os.Args[0], s)
	}
	return nil
}

func do(filename string) (failed bool, err error) {
	workDir, err := ioutil.TempDir("", "puffs-test-c")
	if err != nil {
		return false, err
	}
	defer os.RemoveAll(workDir)

	in := filename + ".c"
	out := filepath.Join(workDir, "a.out")

	ccArgs := []string(nil)
	if *benchFlag {
		ccArgs = append(ccArgs, "-O3")
	} else {
		// TODO: set these flags even if we pass -O3.
		ccArgs = append(ccArgs, "-Wall", "-Werror")
	}
	ccArgs = append(ccArgs, "-std=c99", "-o", out, in)
	if *mimicFlag {
		extra, err := findPuffsMimicCflags(in)
		if err != nil {
			return false, err
		}
		ccArgs = append(ccArgs, extra...)
	}

	for _, cc := range []string{"clang", "gcc"} {
		ccCmd := exec.Command(cc, ccArgs...)
		ccCmd.Stdout = os.Stdout
		ccCmd.Stderr = os.Stderr
		if err := ccCmd.Run(); err != nil {
			return false, err
		}

		outArgs := []string(nil)
		if *benchFlag {
			outArgs = append(outArgs, "-bench", fmt.Sprintf("-reps=%d", *repsFlag))
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

func findPuffsMimicCflags(filename string) ([]string, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	s := bufio.NewScanner(f)
	for s.Scan() {
		t := s.Text()
		const prefix = "// !! puffs mimic cflags:"
		if strings.HasPrefix(t, prefix) {
			t = strings.TrimSpace(t[len(prefix):])
			return strings.Split(t, " "), nil
		}
	}
	return nil, s.Err()
}
