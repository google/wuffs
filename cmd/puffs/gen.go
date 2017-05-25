// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package main

import (
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"os/exec"
	"path/filepath"
	"sort"
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
		arg = filepath.FromSlash(arg)
		if err := gen(puffsRoot, filepath.Join(puffsRoot, arg), langs, recursive); err != nil {
			return err
		}
	}
	return nil
}

func gen(puffsRoot, dirname string, langs []string, recursive bool) error {
	filenames, subDirs := []string(nil), []string(nil)
	infos, err := readdir(dirname)
	if err != nil {
		return err
	}
	for _, o := range infos {
		name := o.Name()
		if o.IsDir() {
			if recursive {
				subDirs = append(subDirs, name)
			}
		} else if strings.HasSuffix(name, ".puffs") {
			filenames = append(filenames, name)
		}
	}

	if len(filenames) > 0 {
		sort.Strings(filenames)
		if err := genDir(puffsRoot, dirname, filenames, langs); err != nil {
			return err
		}
	}

	if len(subDirs) > 0 {
		sort.Strings(subDirs)
		for _, d := range subDirs {
			if err := gen(puffsRoot, filepath.Join(dirname, d), langs, recursive); err != nil {
				return err
			}
		}
	}
	return nil
}

func genDir(puffsRoot string, dirname string, filenames []string, langs []string) error {
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
		cmd := exec.Command(command, "-assume_already_checked")
		cmd.Stdin = bytes.NewReader(combinedSrc)
		out, err := cmd.CombinedOutput()
		if _, ok := err.(*exec.ExitError); ok && len(out) > 0 {
			// Go error messages usually don't end in "\n". The caller, who sees
			// the error we return here, usually adds the "\n" themselves.
			if out[len(out)-1] == '\n' {
				out = out[:len(out)-1]
			}
			// The exec.ExitError's Error message doesn't include stderr, so we
			// return that explicitly.
			return fmt.Errorf("%s: %s", command, out)
		}
		if err != nil {
			return fmt.Errorf("%s: %v", command, err)
		}
		outFilename := filepath.Join(puffsRoot, "gen", lang, filepath.Base(dirname)+"."+lang)
		if existing, err := ioutil.ReadFile(outFilename); err == nil && bytes.Equal(existing, out) {
			fmt.Println("unchanged:", outFilename)
			continue
		}
		if err := ioutil.WriteFile(outFilename, out, 0644); err != nil {
			return err
		}
		fmt.Println("wrote:    ", outFilename)
	}
	return nil
}
