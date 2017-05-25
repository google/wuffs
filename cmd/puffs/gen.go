// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package main

import (
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"github.com/google/puffs/lang/ast"
	"github.com/google/puffs/lang/check"
	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/render"
	"github.com/google/puffs/lang/token"
)

func doGen(puffsRoot string, args []string) error {
	flags := flag.NewFlagSet("gen", flag.ExitOnError)
	langsStr := flags.String("langs", "c", langsUsage)
	if err := flags.Parse(args); err != nil {
		return err
	}
	langs, err := parseLangs(*langsStr)
	if err != nil {
		return err
	}
	args = flags.Args()
	if len(args) == 0 {
		args = []string{"std/..."}
	}

	for _, arg := range args {
		recursive := strings.HasSuffix(arg, "/...")
		if recursive {
			arg = arg[:len(arg)-4]
		}
		arg = filepath.Join(puffsRoot, filepath.FromSlash(arg))
		if err := gen(puffsRoot, arg, langs, recursive); err != nil {
			return err
		}
	}
	return nil
}

func gen(puffsRoot, dirname string, langs []string, recursive bool) error {
	filenames, dirnames, err := listDir(dirname, recursive)
	if err != nil {
		return err
	}
	if len(filenames) > 0 {
		if err := genDir(puffsRoot, dirname, filenames, langs); err != nil {
			return err
		}
	}
	if len(dirnames) > 0 {
		for _, d := range dirnames {
			if err := gen(puffsRoot, filepath.Join(dirname, d), langs, recursive); err != nil {
				return err
			}
		}
	}
	return nil
}

func genDir(puffsRoot string, dirname string, filenames []string, langs []string) error {
	// TODO: skip the generation if the output file already exists and its
	// mtime is newer than all inputs and the puffs-gen-foo command.

	idMap := &token.IDMap{}
	files := []*ast.File(nil)
	buf := &bytes.Buffer{}
	for _, filename := range filenames {
		filename = filepath.Join(dirname, filename)
		src, err := ioutil.ReadFile(filename)
		if err != nil {
			return err
		}
		tokens, _, err := token.Tokenize(idMap, filename, src)
		if err != nil {
			return err
		}
		f, err := parse.Parse(idMap, filename, tokens)
		if err != nil {
			return err
		}
		files = append(files, f)
		if err := render.Render(buf, idMap, tokens, nil); err != nil {
			return err
		}
	}
	if _, err := check.Check(idMap, files...); err != nil {
		return err
	}
	combinedSrc := buf.Bytes()

	for _, lang := range langs {
		command := "puffs-gen-" + lang
		stdout := &bytes.Buffer{}
		cmd := exec.Command(command, "-assume_already_checked")
		cmd.Stdin = bytes.NewReader(combinedSrc)
		cmd.Stdout = stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err == nil {
			// No-op.
		} else if _, ok := err.(*exec.ExitError); ok {
			return fmt.Errorf("%s: failed", command)
		} else {
			return err
		}
		out := stdout.Bytes()
		outFilename := filepath.Join(puffsRoot, "gen", lang, filepath.Base(dirname)+"."+lang)
		if existing, err := ioutil.ReadFile(outFilename); err == nil && bytes.Equal(existing, out) {
			fmt.Println("gen unchanged: ", outFilename)
			continue
		}
		if err := ioutil.WriteFile(outFilename, out, 0644); err != nil {
			return err
		}
		fmt.Println("gen wrote:     ", outFilename)
	}
	return nil
}
