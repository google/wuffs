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
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

func isStringList(s string) bool {
	for i := 0; i < len(s); i++ {
		c := s[i]
		if c == ',' || c == '-' || c == '.' || c == '/' || ('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') ||
			c == '_' || ('a' <= c && c <= 'z') {
			continue
		}
		return false
	}
	return true
}

func doBench(puffsRoot string, args []string) error { return doBenchTest(puffsRoot, args, true) }
func doTest(puffsRoot string, args []string) error  { return doBenchTest(puffsRoot, args, false) }

func doBenchTest(puffsRoot string, args []string, bench bool) error {
	flags := flag.NewFlagSet("test", flag.ExitOnError)
	ccompilersFlag := flags.String("ccompilers", ccompilersDefault, ccompilersUsage)
	focusFlag := flags.String("focus", focusDefault, focusUsage)
	langsFlag := flags.String("langs", langsDefault, langsUsage)
	mimicFlag := flags.Bool("mimic", mimicDefault, mimicUsage)
	repsFlag := flags.Int("reps", repsDefault, repsUsage)
	skipgenFlag := flags.Bool("skipgen", skipgenDefault, skipgenUsage)

	if err := flags.Parse(args); err != nil {
		return err
	}

	langs, err := parseLangs(*langsFlag)
	if err != nil {
		return err
	}
	if !isStringList(*ccompilersFlag) {
		return fmt.Errorf("bad -ccompilers flag value %q", *ccompilersFlag)
	}
	if !isStringList(*focusFlag) {
		return fmt.Errorf("bad -focus flag value %q", *focusFlag)
	}
	if *repsFlag < repsMin || repsMax < *repsFlag {
		return fmt.Errorf("bad -reps flag value %d, outside the range [%d..%d]", *repsFlag, repsMin, repsMax)
	}

	args = flags.Args()
	if len(args) == 0 {
		args = []string{"std/..."}
	}

	cmdArgs := []string(nil)
	if bench {
		cmdArgs = append(cmdArgs, "bench", fmt.Sprintf("-reps=%d", *repsFlag))
	} else {
		cmdArgs = append(cmdArgs, "test")
	}
	if *focusFlag != "" {
		cmdArgs = append(cmdArgs, fmt.Sprintf("-focus=%s", *focusFlag))
	}
	if *mimicFlag {
		cmdArgs = append(cmdArgs, "-mimic")
	}

	b := btHelper{
		puffsRoot:  puffsRoot,
		langs:      langs,
		cmdArgs:    cmdArgs,
		ccompilers: *ccompilersFlag,
	}

	failed := false
	for _, arg := range args {
		recursive := strings.HasSuffix(arg, "/...")
		if recursive {
			arg = arg[:len(arg)-4]
		}

		// Ensure that we are testing the latest version of the generated code.
		if !*skipgenFlag {
			if _, err := gen(nil, puffsRoot, arg, langs, recursive); err != nil {
				return err
			}
		}

		// Proceed with benching / testing the generated code.
		f, err := b.benchTest(arg, recursive)
		if err != nil {
			return err
		}
		failed = failed || f
	}
	if failed {
		s0, s1 := "test", "tests"
		if bench {
			s0, s1 = "bench", "benchmarks"
		}
		return fmt.Errorf("puffs %s: some %s failed", s0, s1)
	}
	return nil
}

type btHelper struct {
	puffsRoot  string
	langs      []string
	cmdArgs    []string
	ccompilers string
}

func (b *btHelper) benchTest(dirname string, recursive bool) (failed bool, err error) {
	filenames, dirnames, err := listDir(b.puffsRoot, dirname, recursive)
	if err != nil {
		return false, err
	}
	if len(filenames) > 0 {
		f, err := b.benchTestDir(dirname)
		if err != nil {
			return false, err
		}
		failed = failed || f
	}
	if len(dirnames) > 0 {
		for _, d := range dirnames {
			f, err := b.benchTest(filepath.Join(dirname, d), recursive)
			if err != nil {
				return false, err
			}
			failed = failed || f
		}
	}
	return failed, nil
}

func (b *btHelper) benchTestDir(dirname string) (failed bool, err error) {
	if packageName := filepath.Base(dirname); !validName(packageName) {
		return false, fmt.Errorf(`invalid package %q, not in [a-z0-9]+`, packageName)
	}

	for _, lang := range b.langs {
		command := "puffs-" + lang
		args := []string(nil)
		args = append(args, b.cmdArgs...)
		if lang == "c" {
			args = append(args, fmt.Sprintf("-ccompilers=%s", b.ccompilers))
		}
		args = append(args, filepath.Join(b.puffsRoot, "test", lang, filepath.FromSlash(dirname)))
		cmd := exec.Command(command, args...)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err == nil {
			// No-op.
		} else if _, ok := err.(*exec.ExitError); ok {
			failed = true
		} else {
			return false, err
		}
	}
	return failed, nil
}
