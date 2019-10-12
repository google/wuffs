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

	"github.com/google/wuffs/lang/wuffsroot"

	cf "github.com/google/wuffs/cmd/commonflags"
)

func doBench(args []string) error { return doBenchTest(args, true) }
func doTest(args []string) error  { return doBenchTest(args, false) }

func doBenchTest(args []string, bench bool) error {
	flags := flag.FlagSet{}
	ccompilersFlag := flags.String("ccompilers", cf.CcompilersDefault, cf.CcompilersUsage)
	focusFlag := flags.String("focus", cf.FocusDefault, cf.FocusUsage)
	iterscaleFlag := flags.Int("iterscale", cf.IterscaleDefault, cf.IterscaleUsage)
	mimicFlag := flags.Bool("mimic", cf.MimicDefault, cf.MimicUsage)
	repsFlag := flags.Int("reps", cf.RepsDefault, cf.RepsUsage)

	if err := flags.Parse(args); err != nil {
		return err
	}

	if !cf.IsAlphaNumericIsh(*ccompilersFlag) {
		return fmt.Errorf("bad -ccompilers flag value %q", *ccompilersFlag)
	}
	if !cf.IsAlphaNumericIsh(*focusFlag) {
		return fmt.Errorf("bad -focus flag value %q", *focusFlag)
	}
	if *iterscaleFlag < cf.IterscaleMin || cf.IterscaleMax < *iterscaleFlag {
		return fmt.Errorf("bad -iterscale flag value %d, outside the range [%d ..= %d]",
			*iterscaleFlag, cf.IterscaleMin, cf.IterscaleMax)
	}
	if *repsFlag < cf.RepsMin || cf.RepsMax < *repsFlag {
		return fmt.Errorf("bad -reps flag value %d, outside the range [%d ..= %d]",
			*repsFlag, cf.RepsMin, cf.RepsMax)
	}

	args = flags.Args()

	failed := false
	for _, arg := range args {
		f, err := doBenchTest1(arg, bench,
			*ccompilersFlag, *focusFlag, *iterscaleFlag, *mimicFlag, *repsFlag)
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

func doBenchTest1(filename string, bench bool, ccompilers string, focus string,
	iterscale int, mimic bool, reps int) (failed bool, err error) {

	workDir, err := ioutil.TempDir("", "wuffs-c")
	if err != nil {
		return false, err
	}
	defer os.RemoveAll(workDir)

	in := filename + ".c"
	out := filepath.Join(workDir, "a.out")

	ccArgs := []string(nil)
	if bench {
		ccArgs = append(ccArgs, "-O3")
	}
	ccArgs = append(ccArgs, "-Wall", "-Werror", "-std=c99", "-o", out, in)
	if mimic {
		extra, err := findWuffsMimicCflags(in)
		if err != nil {
			return false, err
		}
		ccArgs = append(ccArgs, extra...)
	}

	for _, cc := range strings.Split(ccompilers, ",") {
		cc = strings.TrimSpace(cc)
		if cc == "" {
			continue
		}

		ccCmd := exec.Command(cc, ccArgs...)
		ccCmd.Stdout = os.Stdout
		ccCmd.Stderr = os.Stderr
		if err := ccCmd.Run(); err != nil {
			return false, err
		}

		outArgs := []string(nil)
		if bench {
			outArgs = append(outArgs, "-bench",
				fmt.Sprintf("-iterscale=%d", iterscale),
				fmt.Sprintf("-reps=%d", reps),
			)
		}
		if focus != "" {
			outArgs = append(outArgs, fmt.Sprintf("-focus=%s", focus))
		}
		outCmd := exec.Command(out, outArgs...)
		outCmd.Stdout = os.Stdout
		outCmd.Stderr = os.Stderr
		if outCmd.Dir, err = wuffsroot.Value(); err != nil {
			return false, err
		}
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

func findWuffsMimicCflags(filename string) ([]string, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	s := bufio.NewScanner(f)
	for s.Scan() {
		t := s.Text()
		const prefix = "// !! wuffs mimic cflags:"
		if strings.HasPrefix(t, prefix) {
			t = strings.TrimSpace(t[len(prefix):])
			return strings.Split(t, " "), nil
		}
	}
	return nil, s.Err()
}
