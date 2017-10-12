// Copyright 2017 The Puffs Authors.
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

func doBench(args []string) error { return doBenchTest(args, true) }
func doTest(args []string) error  { return doBenchTest(args, false) }

func doBenchTest(args []string, bench bool) error {
	const (
		mimicDefault = false
		mimicUsage   = `whether to compare Puffs' output with other libraries' output`

		repsDefault = 5
		repsMin     = 0
		repsMax     = 1000000
		repsUsage   = `the number of repetitions per benchmark`
	)

	flags := flag.FlagSet{}
	mimicFlag := flags.Bool("mimic", mimicDefault, mimicUsage)
	repsFlag := flags.Int("reps", repsDefault, repsUsage)
	if err := flags.Parse(args); err != nil {
		return err
	}
	args = flags.Args()

	if *repsFlag < repsMin || repsMax < *repsFlag {
		return fmt.Errorf("bad -reps flag value %d, outside the range [%d..%d]", *repsFlag, repsMin, repsMax)
	}

	failed := false
	for _, arg := range args {
		f, err := doBenchTest1(arg, bench, *mimicFlag, *repsFlag)
		if err != nil {
			return err
		}
		failed = failed || f
	}
	if failed {
		s := "tests"
		if bench {
			s = "benchmarks"
		}
		return fmt.Errorf("%s: some %s failed", os.Args[0], s)
	}
	return nil
}

func doBenchTest1(filename string, bench bool, mimic bool, reps int) (failed bool, err error) {
	workDir, err := ioutil.TempDir("", "puffs-c")
	if err != nil {
		return false, err
	}
	defer os.RemoveAll(workDir)

	in := filename + ".c"
	out := filepath.Join(workDir, "a.out")

	ccArgs := []string(nil)
	if bench {
		ccArgs = append(ccArgs, "-O3")
	} else {
		// TODO: set these flags even if we pass -O3.
		ccArgs = append(ccArgs, "-Wall", "-Werror")
	}
	ccArgs = append(ccArgs, "-std=c99", "-o", out, in)
	if mimic {
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
		if bench {
			outArgs = append(outArgs, "-bench", fmt.Sprintf("-reps=%d", reps))
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
