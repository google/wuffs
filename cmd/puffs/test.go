// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

func doBench(puffsRoot string, args []string) error { return doBenchTest(puffsRoot, args, true) }
func doTest(puffsRoot string, args []string) error  { return doBenchTest(puffsRoot, args, false) }

func doBenchTest(puffsRoot string, args []string, bench bool) error {
	flags := flag.NewFlagSet("test", flag.ExitOnError)
	langsFlag := flags.String("langs", langsDefault, langsUsage)
	mimicFlag := flags.Bool("mimic", mimicDefault, mimicUsage)
	repsFlag := flags.Int("reps", repsDefault, repsUsage)
	if err := flags.Parse(args); err != nil {
		return err
	}
	langs, err := parseLangs(*langsFlag)
	if err != nil {
		return err
	}
	if *repsFlag < repsMin || repsMax < *repsFlag {
		return fmt.Errorf("bad -reps flag value %d, outside the range [%d..%d]", *repsFlag, repsMin, repsMax)
	}
	args = flags.Args()
	if len(args) == 0 {
		args = []string{"std/..."}
	}

	cmdArgs := []string(nil)
	if *mimicFlag {
		cmdArgs = append(cmdArgs, "-mimic")
	}
	if bench {
		cmdArgs = append(cmdArgs, "-bench", fmt.Sprintf("-reps=%d", *repsFlag))
	}

	failed := false
	for _, arg := range args {
		recursive := strings.HasSuffix(arg, "/...")
		if recursive {
			arg = arg[:len(arg)-4]
		}

		// Ensure that we are testing the latest version of the generated code.
		if _, err := gen(nil, puffsRoot, arg, langs, recursive); err != nil {
			return err
		}

		// Proceed with benching / testing the generated code.
		f, err := benchTest(puffsRoot, arg, langs, cmdArgs, recursive)
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

func benchTest(puffsRoot, dirname string, langs []string, cmdArgs []string, recursive bool) (failed bool, err error) {
	filenames, dirnames, err := listDir(puffsRoot, dirname, recursive)
	if err != nil {
		return false, err
	}
	if len(filenames) > 0 {
		f, err := benchTestDir(puffsRoot, dirname, langs, cmdArgs)
		if err != nil {
			return false, err
		}
		failed = failed || f
	}
	if len(dirnames) > 0 {
		for _, d := range dirnames {
			f, err := benchTest(puffsRoot, filepath.Join(dirname, d), langs, cmdArgs, recursive)
			if err != nil {
				return false, err
			}
			failed = failed || f
		}
	}
	return failed, nil
}

func benchTestDir(puffsRoot string, dirname string, langs []string, cmdArgs []string) (failed bool, err error) {
	if packageName := filepath.Base(dirname); !validName(packageName) {
		return false, fmt.Errorf(`invalid package %q, not in [a-z0-9]+`, packageName)
	}

	for _, lang := range langs {
		command := "puffs-test-" + lang
		args := []string(nil)
		args = append(args, cmdArgs...)
		args = append(args, filepath.Join(puffsRoot, "test", lang, filepath.FromSlash(dirname)))
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
