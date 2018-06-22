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
	"bytes"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"strings"

	"github.com/google/wuffs/lang/generate"
	"github.com/google/wuffs/lang/parse"

	cf "github.com/google/wuffs/cmd/commonflags"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

func doGen(wuffsRoot string, args []string) error    { return doGenGenlib(wuffsRoot, args, false) }
func doGenlib(wuffsRoot string, args []string) error { return doGenGenlib(wuffsRoot, args, true) }

func doGenGenlib(wuffsRoot string, args []string, genlib bool) error {
	flags := flag.NewFlagSet("gen", flag.ExitOnError)
	cformatterFlag := flags.String("cformatter", cf.CformatterDefault, cf.CformatterUsage)
	langsFlag := flags.String("langs", langsDefault, langsUsage)
	skipgendepsFlag := flags.Bool("skipgendeps", skipgendepsDefault, skipgendepsUsage)

	if err := flags.Parse(args); err != nil {
		return err
	}
	if !cf.IsAlphaNumericIsh(*cformatterFlag) {
		return fmt.Errorf("bad -cformatter flag value %q", *cformatterFlag)
	}
	langs, err := parseLangs(*langsFlag)
	if err != nil {
		return err
	}
	args = flags.Args()
	if len(args) == 0 {
		args = []string{"std/..."}
	}

	h := genHelper{
		wuffsRoot:   wuffsRoot,
		langs:       langs,
		cformatter:  *cformatterFlag,
		skipgendeps: *skipgendepsFlag,
	}

	for _, arg := range args {
		recursive := strings.HasSuffix(arg, "/...")
		if recursive {
			arg = arg[:len(arg)-4]
		}
		if arg == "" {
			continue
		}
		if err := h.gen(arg, recursive); err != nil {
			return err
		}
	}

	if genlib {
		return h.genlibAffected()
	}
	return nil
}

type genHelper struct {
	wuffsRoot   string
	langs       []string
	cformatter  string
	skipgendeps bool

	affected []string
	seen     map[string]struct{}
	tm       t.Map
}

func (h *genHelper) gen(dirname string, recursive bool) error {
	for len(dirname) > 0 && dirname[len(dirname)-1] == '/' {
		dirname = dirname[:len(dirname)-1]
	}

	if h.seen == nil {
		h.seen = map[string]struct{}{}
	} else if _, ok := h.seen[dirname]; ok {
		return nil
	}
	h.seen[dirname] = struct{}{}

	if dirname == "base" {
		if err := h.genDir(dirname, nil); err != nil {
			return err
		}
		h.affected = append(h.affected, dirname)
		return nil
	}

	if !cf.IsValidUsePath(dirname) {
		return fmt.Errorf("invalid package path %q", dirname)
	}

	qualFilenames, dirnames, err := listDir(
		filepath.Join(h.wuffsRoot, filepath.FromSlash(dirname)), ".wuffs", recursive)
	if err != nil {
		return err
	}
	if len(qualFilenames) > 0 {
		if err := h.genDir(dirname, qualFilenames); err != nil {
			return err
		}
		h.affected = append(h.affected, dirname)
	}
	if len(dirnames) > 0 {
		for _, d := range dirnames {
			if err := h.gen(dirname+"/"+d, recursive); err != nil {
				return err
			}
		}
	}
	return nil
}

func (h *genHelper) genDir(dirname string, qualFilenames []string) error {
	// TODO: skip the generation if the output file already exists and its
	// mtime is newer than all inputs and the wuffs-gen-foo command.

	packageName := path.Base(dirname)
	if !validName(packageName) {
		return fmt.Errorf(`invalid package %q, not in [a-z0-9]+`, packageName)
	}

	if !h.skipgendeps {
		if err := h.genDirDependencies(qualFilenames); err != nil {
			return err
		}
	}

	for _, lang := range h.langs {
		command := "wuffs-" + lang
		cmdArgs := []string{"gen", "-package_name", packageName}
		if lang == "c" {
			cmdArgs = append(cmdArgs, fmt.Sprintf("-cformatter=%s", h.cformatter))
		}
		cmdArgs = append(cmdArgs, qualFilenames...)
		stdout := &bytes.Buffer{}

		cmd := exec.Command(command, cmdArgs...)
		cmd.Stdin = nil
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
		if err := h.genFile(dirname, lang, out); err != nil {
			return err
		}

		// Special-case the "c" generator to also write a .h file.
		if lang != "c" || packageName == "base" {
			continue
		}

		if i := bytes.Index(out, cHeaderEndsHere); i >= 0 {
			out = out[:i]
		} else {
			return fmt.Errorf("%s: output did not contain %q", command, cHeaderEndsHere)
		}
		if err := h.genFile(dirname, "h", out); err != nil {
			return err
		}
	}
	if len(h.langs) > 0 && packageName != "base" {
		if err := h.genWuffs(dirname, qualFilenames); err != nil {
			return err
		}
	}
	return nil
}

var cHeaderEndsHere = []byte("\n// !! C HEADER ENDS HERE.\n\n")

func (h *genHelper) genDirDependencies(qualifiedFilenames []string) error {
	files, err := generate.ParseFiles(&h.tm, qualifiedFilenames, nil)
	if err != nil {
		return err
	}
	for _, f := range files {
		for _, n := range f.TopLevelDecls() {
			if n.Kind() != a.KUse {
				continue
			}
			useDirname := h.tm.ByID(n.Use().Path())
			useDirname, _ = t.Unescape(useDirname)
			if err := h.gen(useDirname, false); err != nil {
				return err
			}
		}
	}
	return h.gen("base", false)
}

func (h *genHelper) genFile(dirname string, lang string, out []byte) error {
	return writeFile(
		filepath.Join(h.wuffsRoot, "gen", lang, filepath.FromSlash(dirname)+"."+lang),
		out,
	)
}

func (h *genHelper) genWuffs(dirname string, qualifiedFilenames []string) error {
	files, err := generate.ParseFiles(&h.tm, qualifiedFilenames, &parse.Options{
		AllowDoubleUnderscoreNames: true,
	})
	if err != nil {
		return err
	}
	pkgIDNode := (*a.PackageID)(nil)
	for _, f := range files {
		for _, n := range f.TopLevelDecls() {
			if n.Kind() == a.KPackageID {
				pkgIDNode = n.PackageID()
			}
		}
	}
	if pkgIDNode == nil {
		return fmt.Errorf("missing packageid declaration")
	}
	pkgIDStr, ok := t.Unescape(pkgIDNode.ID().Str(&h.tm))
	if !ok {
		return fmt.Errorf("invalid packageid declaration")
	}

	out := &bytes.Buffer{}
	fmt.Fprintf(out, "// Code generated by running \"wuffs gen\". DO NOT EDIT.\n\n")
	fmt.Fprintf(out, "packageid %q\n\n", pkgIDStr)

	for _, f := range files {
		for _, n := range f.TopLevelDecls() {
			switch n.Kind() {
			case a.KConst:
				n := n.Const()
				if !n.Public() {
					continue
				}
				return fmt.Errorf("TODO: genWuffs for consts")

			case a.KFunc:
				n := n.Func()
				if !n.Public() {
					continue
				}
				effect := ""
				if n.Suspendible() {
					effect = "?"
				} else if n.Impure() {
					effect = "!"
				}
				if n.Receiver().IsZero() {
					return fmt.Errorf("TODO: genWuffs for a free-standing function")
				}
				// TODO: look at n.Asserts().
				fmt.Fprintf(out, "pub func %s.%s%s(", n.Receiver().Str(&h.tm), n.FuncName().Str(&h.tm), effect)
				for i, param := range [2]*a.Struct{n.In(), n.Out()} {
					if i > 0 {
						fmt.Fprintf(out, ")(")
					}
					for j, field := range param.Fields() {
						field := field.Field()
						if j > 0 {
							fmt.Fprintf(out, ", ")
						}
						// TODO: what happens if the XType is from another
						// package?
						fmt.Fprintf(out, "%s %s", field.Name().Str(&h.tm), field.XType().Str(&h.tm))
					}
				}
				fmt.Fprintf(out, ") { }\n")

			case a.KStatus:
				n := n.Status()
				if !n.Public() {
					continue
				}
				fmt.Fprintf(out, "pub %s (%s) %s\n",
					n.Keyword().Str(&h.tm), n.Value().Str(&h.tm), n.QID().Str(&h.tm))

			case a.KStruct:
				n := n.Struct()
				if !n.Public() {
					continue
				}
				effect := ""
				if n.Suspendible() {
					effect = "?"
				}
				fmt.Fprintf(out, "pub struct %s%s()\n", n.QID().Str(&h.tm), effect)
			}
		}
	}
	return h.genFile(dirname, "wuffs", out.Bytes())
}

func (h *genHelper) genlibAffected() error {
	for _, lang := range h.langs {
		command := "wuffs-" + lang
		args := []string{"genlib"}
		args = append(args, "-dstdir", filepath.Join(h.wuffsRoot, "gen", "lib", lang))
		args = append(args, "-srcdir", filepath.Join(h.wuffsRoot, "gen", lang))
		args = append(args, h.affected...)
		cmd := exec.Command(command, args...)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err != nil {
			return err
		}
	}
	return nil
}
