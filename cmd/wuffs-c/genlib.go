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
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	cf "github.com/google/wuffs/cmd/commonflags"
)

func doGenlib(args []string) error {
	flags := flag.FlagSet{}
	ccompilersFlag := flags.String("ccompilers", cf.CcompilersDefault, cf.CcompilersUsage)
	dstdirFlag := flags.String("dstdir", "", "directory containing the object files ")
	srcdirFlag := flags.String("srcdir", "", "directory containing the C source files")
	if err := flags.Parse(args); err != nil {
		return err
	}
	args = flags.Args()

	filenames := make([]string, len(args))
	for i, arg := range args {
		filenames[i] = "wuffs-" + strings.Replace(filepath.ToSlash(arg), "/", "-", -1)
	}

	if *dstdirFlag == "" {
		return fmt.Errorf("empty -dstdir flag")
	}
	if *srcdirFlag == "" {
		return fmt.Errorf("empty -srcdir flag")
	}

	for _, cc := range strings.Split(*ccompilersFlag, ",") {
		cc = strings.TrimSpace(cc)
		if cc == "" {
			continue
		}

		for _, dynamism := range []string{"static", "dynamic"} {
			outDir := filepath.Join(*dstdirFlag, cc+"-"+dynamism)
			if err := os.MkdirAll(outDir, 0755); err != nil {
				return err
			}
			if err := genObj(outDir, *srcdirFlag, cc, dynamism, filenames); err != nil {
				return err
			}
			if err := genLib(outDir, cc, dynamism, filenames); err != nil {
				return err
			}
		}
	}
	return nil
}

// TODO: are these extensions correct for non-Linux?
var (
	objExtensions = map[string]string{
		"dynamic": ".lo",
		"static":  ".o",
	}
	libExtensions = map[string]string{
		"dynamic": ".so",
		"static":  ".a",
	}
)

func genObj(outDir string, inDir string, cc string, dynamism string, filenames []string) error {
	for _, filename := range filenames {
		in := filepath.Join(inDir, filename+".c")
		out := genlibOutFilename(outDir, dynamism, filename)

		args := []string(nil)
		args = append(args, "-O3", "-std=c99", "-DWUFFS_IMPLEMENTATION")
		if dynamism == "dynamic" {
			args = append(args, "-fPIC", "-DPIC")
		}
		args = append(args, "-c", "-o", out, in)

		cmd := exec.Command(cc, args...)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err != nil {
			return err
		}
		fmt.Printf("genlib: %s\n", out)
	}
	return nil
}

func genLib(outDir string, cc string, dynamism string, filenames []string) error {
	args := []string(nil)
	switch dynamism {
	case "dynamic":
		// TODO: add a "-Wl,-soname,libwuffs.so.1.2.3" argument?
		args = append(args, "-shared", "-fPIC", "-o")
	case "static":
		cc = "ar"
		args = append(args, "rc")
	}
	out := filepath.Join(outDir, "libwuffs"+libExtensions[dynamism])
	args = append(args, out)

	for _, filename := range filenames {
		args = append(args, genlibOutFilename(outDir, dynamism, filename))
	}

	cmd := exec.Command(cc, args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return err
	}
	fmt.Printf("genlib: %s\n", out)
	return nil
}

func genlibOutFilename(outDir string, dynamism string, filename string) string {
	return filepath.Join(outDir, filename+objExtensions[dynamism])
}
