// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package generate

import (
	"bytes"
	"flag"
	"io/ioutil"
	"os"

	"github.com/google/puffs/lang/ast"
	"github.com/google/puffs/lang/check"
	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/token"
)

var (
	assumeAlreadyChecked = flag.Bool("assume_already_checked", false,
		"whether to skip type- and bounds-checking; unsafe unless some other program checks the inputs")
)

type Generator interface {
	WritePreamble(w *bytes.Buffer) error
	WritePostamble(w *bytes.Buffer) error
	Format(rawSource []byte) ([]byte, error)
}

func Main(g Generator) error {
	flag.Parse()
	idMap := &token.IDMap{}
	files, err := parseFiles(idMap)
	if err != nil {
		return err
	}
	if !*assumeAlreadyChecked {
		for _, f := range files {
			if _, err := check.Check(idMap, f); err != nil {
				return err
			}
		}
	}

	b := &bytes.Buffer{}
	if err := g.WritePreamble(b); err != nil {
		return err
	}
	for _, f := range files {
		// TODO: call g.WriteFoo methods.
		_ = f
	}
	if err := g.WritePostamble(b); err != nil {
		return err
	}
	formatted, err := g.Format(b.Bytes())
	if err != nil {
		return err
	}
	if _, err := os.Stdout.Write(formatted); err != nil {
		return err
	}
	return nil
}

func parseFiles(idMap *token.IDMap) (files []*ast.File, err error) {
	if len(flag.Args()) == 0 {
		const filename = "stdin"
		src, err := ioutil.ReadAll(os.Stdin)
		if err != nil {
			return nil, err
		}
		tokens, _, err := token.Tokenize(idMap, filename, src)
		if err != nil {
			return nil, err
		}
		f, err := parse.Parse(idMap, filename, tokens)
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
		tokens, _, err := token.Tokenize(idMap, filename, src)
		if err != nil {
			return nil, err
		}
		f, err := parse.Parse(idMap, filename, tokens)
		if err != nil {
			return nil, err
		}
		files = append(files, f)
	}
	return files, nil
}
