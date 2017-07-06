// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package generate

import (
	"flag"
	"io/ioutil"
	"os"
	"os/exec"

	"github.com/google/puffs/lang/ast"
	"github.com/google/puffs/lang/check"
	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/token"
)

var (
	packageName = flag.String("package_name", "", "the package name of the Puffs input code")
)

type Generator func(packageName string, tm *token.Map, c *check.Checker, files []*ast.File) ([]byte, error)

func Main(g Generator) {
	if err := main1(g); err != nil {
		if _, ok := err.(*exec.ExitError); !ok {
			os.Stderr.WriteString(err.Error() + "\n")
		}
		os.Exit(1)
	}
}

func main1(g Generator) error {
	flag.Parse()
	tm := &token.Map{}
	files, err := parseFiles(tm)
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

func parseFiles(tm *token.Map) (files []*ast.File, err error) {
	if len(flag.Args()) == 0 {
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

	for _, filename := range flag.Args() {
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
