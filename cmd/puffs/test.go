// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package main

import (
	"errors"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

func doTest(puffsRoot string, args []string) error {
	flags := flag.NewFlagSet("test", flag.ExitOnError)
	langsFlag := flags.String("langs", langsDefault, langsUsage)
	if err := flags.Parse(args); err != nil {
		return err
	}
	langs, err := parseLangs(*langsFlag)
	if err != nil {
		return err
	}
	args = flags.Args()
	if len(args) == 0 {
		args = []string{"std/..."}
	}

	failed := false
	for _, arg := range args {
		recursive := strings.HasSuffix(arg, "/...")
		if recursive {
			arg = arg[:len(arg)-4]
		}
		arg = filepath.Join(puffsRoot, filepath.FromSlash(arg))

		// Ensure that we are testing the latest version of the generated code.
		if err := gen(puffsRoot, arg, langs, recursive); err != nil {
			return err
		}

		// Proceed with testing the generated code.
		f, err := test(puffsRoot, arg, langs, recursive)
		if err != nil {
			return err
		}
		failed = failed || f
	}
	if failed {
		return errors.New("puffs test: some tests failed")
	}
	return nil
}

func test(puffsRoot, dirname string, langs []string, recursive bool) (failed bool, err error) {
	filenames, dirnames, err := listDir(dirname, recursive)
	if err != nil {
		return false, err
	}
	if len(filenames) > 0 {
		f, err := testDir(puffsRoot, dirname, langs)
		if err != nil {
			return false, err
		}
		failed = failed || f
	}
	if len(dirnames) > 0 {
		for _, d := range dirnames {
			f, err := test(puffsRoot, filepath.Join(dirname, d), langs, recursive)
			if err != nil {
				return false, err
			}
			failed = failed || f
		}
	}
	return failed, nil
}

func testDir(puffsRoot string, dirname string, langs []string) (failed bool, err error) {
	packageName := filepath.Base(dirname)
	if !validName(packageName) {
		return false, fmt.Errorf(`invalid package %q, not in [a-z0-9]+`, packageName)
	}

	for _, lang := range langs {
		command := "puffs-test-" + lang
		testDirname := filepath.Join(puffsRoot, "test", lang, packageName)
		fmt.Println("test:          ", testDirname)
		cmd := exec.Command(command, testDirname)
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
