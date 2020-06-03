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
	"fmt"
	"os"
	"os/exec"
	"path/filepath"

	cf "github.com/google/wuffs/cmd/commonflags"
)

func genrelease(wuffsRoot string, langs []string, v cf.Version) error {
	revision := runGitCommand(wuffsRoot, "rev-parse", "HEAD")
	commitDate := runGitCommand(wuffsRoot, "show",
		"--quiet", "--date=format-local:%Y-%m-%d", "--format=%cd")
	gitRevListCount := runGitCommand(wuffsRoot, "rev-list", "--count", "HEAD")
	for _, lang := range langs {
		filename, contents, err := genreleaseLang(wuffsRoot, revision, commitDate, gitRevListCount, v, lang)
		if err != nil {
			return err
		}
		if err := writeFile(filename, contents); err != nil {
			return err
		}
	}
	return nil
}

func genreleaseLang(wuffsRoot string, revision string, commitDate, gitRevListCount string, v cf.Version, lang string) (filename string, contents []byte, err error) {
	qualFilenames, err := findFiles(filepath.Join(wuffsRoot, "gen", lang), "."+lang)
	if err != nil {
		return "", nil, err
	}

	command := "wuffs-" + lang
	args := []string(nil)
	args = append(args, "genrelease",
		"-revision", revision,
		"-commitdate", commitDate,
		"-version", v.String(),
	)
	if gitRevListCount != "" {
		args = append(args, "-gitrevlistcount", gitRevListCount)
	}
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

	base := "wuffs-unsupported-snapshot"
	if v.Major != 0 || v.Minor != 0 {
		base = fmt.Sprintf("wuffs-v%d.%d", v.Major, v.Minor)
	}
	return filepath.Join(wuffsRoot, "release", lang, base+"."+lang), stdout.Bytes(), nil
}

func runGitCommand(wuffsRoot string, cmdArgs ...string) string {
	cmd := exec.Command("git", cmdArgs...)
	cmd.Dir = wuffsRoot
	cmd.Env = []string{"TZ=UTC"}
	out, err := cmd.Output()
	if err != nil {
		return ""
	}
	return string(bytes.TrimSpace(out))
}
