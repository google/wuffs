// Copyright 2018 The Wuffs Authors.
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
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"

	cf "github.com/google/wuffs/cmd/commonflags"
)

func doGenrelease(wuffsRoot string, args []string) error {
	flags := flag.NewFlagSet("genrelease", flag.ExitOnError)
	langsFlag := flags.String("langs", langsDefault, langsUsage)
	skipgenFlag := flags.Bool("skipgen", skipgenDefault, skipgenUsage)
	versionFlag := flags.String("version", cf.VersionDefault, cf.VersionUsage)

	if err := flags.Parse(args); err != nil {
		return err
	}
	langs, err := parseLangs(*langsFlag)
	if err != nil {
		return err
	}
	v, ok := cf.ParseVersion(*versionFlag)
	if !ok {
		return fmt.Errorf("bad -version flag value %q", *versionFlag)
	}

	if !*skipgenFlag {
		gh := genHelper{
			wuffsRoot:  wuffsRoot,
			langs:      langs,
			cformatter: cf.CformatterDefault,
		}
		if err := gh.gen("std", true); err != nil {
			return err
		}
	}

	revision := findRevision(wuffsRoot)
	for _, lang := range langs {
		filename, contents, err := doGenrelease1(wuffsRoot, revision, v, lang)
		if err != nil {
			return err
		}
		if err := writeFile(filename, contents); err != nil {
			return err
		}

		// Special-case the "c" generator to also write a .h file.
		if lang != "c" {
			continue
		}

		filename = filename[:len(filename)-1] + "h"
		if i := bytes.Index(contents, cHeaderEndsHere); i >= 0 {
			contents = contents[:i]
		} else {
			return fmt.Errorf("wuffs-c output did not contain %q", cHeaderEndsHere)
		}
		if err := writeFile(filename, contents); err != nil {
			return err
		}
	}
	return nil
}

func doGenrelease1(wuffsRoot string, revision string, v cf.Version, lang string) (filename string, contents []byte, err error) {
	qualFilenames, err := findFiles(filepath.Join(wuffsRoot, "gen", lang), "."+lang)
	if err != nil {
		return "", nil, err
	}

	command := "wuffs-" + lang
	args := []string(nil)
	args = append(args, "genrelease", "-revision", revision, "-version", v.String())
	args = append(args, qualFilenames...)
	stdout := &bytes.Buffer{}

	cmd := exec.Command(command, args...)
	cmd.Stdout = stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err == nil {
		// No-op.
	} else if _, ok := err.(*exec.ExitError); ok {
		return "", nil, fmt.Errorf("%s: failed", command)
	} else {
		return "", nil, err
	}

	wv := fmt.Sprintf("wuffs-v%d.%d", v.Major, v.Minor)
	return filepath.Join(wuffsRoot, "release", lang, wv, wv+"."+lang), stdout.Bytes(), nil
}

func findRevision(wuffsRoot string) string {
	// Assume that we're using git.

	head, err := ioutil.ReadFile(filepath.Join(wuffsRoot, ".git", "HEAD"))
	if err != nil {
		return ""
	}
	refPrefix := []byte("ref: ")
	if !bytes.HasPrefix(head, refPrefix) {
		return ""
	}
	head = head[len(refPrefix):]
	if len(head) == 0 || head[len(head)-1] != '\n' {
		return ""
	}
	head = head[:len(head)-1]

	ref, err := ioutil.ReadFile(filepath.Join(wuffsRoot, ".git", string(head)))
	if err != nil {
		return ""
	}
	if len(ref) == 0 || ref[len(ref)-1] != '\n' {
		return ""
	}
	ref = ref[:len(ref)-1]

	return string(ref)
}
