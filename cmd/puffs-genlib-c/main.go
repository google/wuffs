// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// puffs-genlib-c builds C software libraries.
package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

var (
	dstdirFlag = flag.String("dstdir", "", "directory containing the object files ")
	srcdirFlag = flag.String("srcdir", "", "directory containing the C source files")
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()
	if *dstdirFlag == "" {
		return fmt.Errorf("empty -dstdir flag")
	}
	if *srcdirFlag == "" {
		return fmt.Errorf("empty -srcdir flag")
	}
	args := flag.Args()

	for _, cc := range []string{"clang", "gcc"} {
		for _, dynamism := range []string{"static", "dynamic"} {
			outDir := filepath.Join(*dstdirFlag, cc+"-"+dynamism)
			if err := os.MkdirAll(outDir, 0755); err != nil {
				return err
			}
			if err := genObj(outDir, cc, dynamism, args); err != nil {
				return err
			}
			if err := genLib(outDir, cc, dynamism, args); err != nil {
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

func genObj(outDir string, cc string, dynamism string, filenames []string) error {
	for _, filename := range filenames {
		in := filepath.Join(*srcdirFlag, filename+".c")
		out := outFilename(outDir, dynamism, filename)

		args := []string(nil)
		args = append(args, "-O3", "-std=c99")
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
		// TODO: add a "-Wl,-soname,libpuffs.so.1.2.3" argument?
		args = append(args, "-shared", "-fPIC", "-o")
	case "static":
		cc = "ar"
		args = append(args, "rc")
	}
	out := filepath.Join(outDir, "libpuffs"+libExtensions[dynamism])
	args = append(args, out)

	for _, filename := range filenames {
		args = append(args, outFilename(outDir, dynamism, filename))
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

func outFilename(outDir string, dynamism string, filename string) string {
	filename = strings.Replace(filename, "/", "-", -1)
	filename = strings.Replace(filename, "\\", "-", -1)
	filename = filepath.Join(outDir, filename+objExtensions[dynamism])
	return filename
}
