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

package generate

import (
	"errors"
	"flag"
	"fmt"
	"go/build"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"sync"

	"github.com/google/wuffs/lang/check"
	"github.com/google/wuffs/lang/parse"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

type Generator func(packageName string, tm *t.Map, c *check.Checker, files []*a.File) ([]byte, error)

func Do(args []string, g Generator) error {
	flags := flag.FlagSet{}
	packageName := flags.String("package_name", "", "the package name of the Wuffs input code")
	if err := flags.Parse(args); err != nil {
		return err
	}
	pkgName := checkPackageName(*packageName)
	if pkgName == "" {
		return fmt.Errorf("prohibited package name %q", *packageName)
	}

	tm := &t.Map{}
	files, err := parseFiles(tm, flags.Args())
	if err != nil {
		return err
	}

	c, err := check.Check(tm, files, resolveUse)
	if err != nil {
		return err
	}

	out, err := g(pkgName, tm, c, files)
	if err != nil {
		return err
	}

	if _, err := os.Stdout.Write(out); err != nil {
		return err
	}
	return nil
}

func checkPackageName(s string) string {
	allUnderscores := true
	for i := 0; i < len(s); i++ {
		c := s[i]
		if ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (c == '_') || ('0' <= c && c <= '9') {
			allUnderscores = allUnderscores && c == '_'
		} else {
			return ""
		}
	}
	if allUnderscores {
		return ""
	}
	s = strings.ToLower(s)
	// Blacklist certain package names.
	switch s {
	case "base", "base_header", "base_impl", "config", "version":
		return ""
	}
	return s
}

func parseFiles(tm *t.Map, filenames []string) (files []*a.File, err error) {
	if len(filenames) == 0 {
		const filename = "stdin"
		src, err := ioutil.ReadAll(os.Stdin)
		if err != nil {
			return nil, err
		}
		tokens, _, err := t.Tokenize(tm, filename, src)
		if err != nil {
			return nil, err
		}
		f, err := parse.Parse(tm, filename, tokens, nil)
		if err != nil {
			return nil, err
		}
		return []*a.File{f}, nil
	}
	return ParseFiles(tm, filenames, nil)
}

func ParseFiles(tm *t.Map, filenames []string, opts *parse.Options) (files []*a.File, err error) {
	for _, filename := range filenames {
		src, err := ioutil.ReadFile(filename)
		if err != nil {
			return nil, err
		}
		tokens, _, err := t.Tokenize(tm, filename, src)
		if err != nil {
			return nil, err
		}
		f, err := parse.Parse(tm, filename, tokens, opts)
		if err != nil {
			return nil, err
		}
		files = append(files, f)
	}
	return files, nil
}

func resolveUse(usePath string) ([]byte, error) {
	wuffsRoot, err := WuffsRoot()
	if err != nil {
		return nil, err
	}
	return ioutil.ReadFile(filepath.Join(wuffsRoot, "gen", "wuffs", filepath.FromSlash(usePath)))
}

var cachedWuffsRoot struct {
	mu    sync.Mutex
	value string
}

func WuffsRoot() (string, error) {
	cachedWuffsRoot.mu.Lock()
	value := cachedWuffsRoot.value
	cachedWuffsRoot.mu.Unlock()

	if value != "" {
		return value, nil
	}

	// TODO: look for a WUFFSROOT environment variable?
	for _, p := range filepath.SplitList(build.Default.GOPATH) {
		p = filepath.Join(p, "src", "github.com", "google", "wuffs")
		if o, err := os.Stat(p); err == nil && o.IsDir() {
			cachedWuffsRoot.mu.Lock()
			cachedWuffsRoot.value = p
			cachedWuffsRoot.mu.Unlock()

			return p, nil
		}
	}
	return "", errors.New("could not find Wuffs root directory")
}
