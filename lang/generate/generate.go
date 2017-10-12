// Copyright 2017 The Puffs Authors.
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
	"flag"
	"io/ioutil"
	"os"

	"github.com/google/puffs/lang/ast"
	"github.com/google/puffs/lang/check"
	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/token"
)

type Generator func(packageName string, tm *token.Map, c *check.Checker, files []*ast.File) ([]byte, error)

func Do(args []string, g Generator) error {
	flags := flag.FlagSet{}
	packageName := flags.String("package_name", "", "the package name of the Puffs input code")
	if err := flags.Parse(args); err != nil {
		return err
	}
	args = flags.Args()

	tm := &token.Map{}
	files, err := parseFiles(tm, args)
	if err != nil {
		return err
	}

	c, err := check.Check(tm, files...)
	if err != nil {
		return err
	}

	out, err := g(*packageName, tm, c, files)
	if err != nil {
		return err
	}

	if _, err := os.Stdout.Write(out); err != nil {
		return err
	}
	return nil
}

func parseFiles(tm *token.Map, args []string) (files []*ast.File, err error) {
	if len(args) == 0 {
		const filename = "stdin"
		src, err := ioutil.ReadAll(os.Stdin)
		if err != nil {
			return nil, err
		}
		tokens, _, err := token.Tokenize(tm, filename, src)
		if err != nil {
			return nil, err
		}
		f, err := parse.Parse(tm, filename, tokens)
		if err != nil {
			return nil, err
		}
		return []*ast.File{f}, nil
	}

	for _, filename := range args {
		src, err := ioutil.ReadFile(filename)
		if err != nil {
			return nil, err
		}
		tokens, _, err := token.Tokenize(tm, filename, src)
		if err != nil {
			return nil, err
		}
		f, err := parse.Parse(tm, filename, tokens)
		if err != nil {
			return nil, err
		}
		files = append(files, f)
	}
	return files, nil
}
