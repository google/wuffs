// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package main

import (
	"bytes"
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"runtime"
	"strings"

	"github.com/google/puffs/lang/parse"
	"github.com/google/puffs/lang/render"
	"github.com/google/puffs/lang/token"
)

var (
	lFlag = flag.Bool("l", false, "list files whose formatting differs from puffsfmt's")
	wFlag = flag.Bool("w", false, "write result to (source) file instead of stdout")
)

func usage() {
	fmt.Fprintf(os.Stderr, "usage: puffsfmt [flags] [path ...]\n")
	flag.PrintDefaults()
}

func main() {
	if err := main1(); err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		os.Exit(1)
	}
}

func main1() error {
	flag.Usage = usage
	flag.Parse()

	if flag.NArg() == 0 {
		if *lFlag {
			return errors.New("cannot use -l with standard input")
		}
		if *wFlag {
			return errors.New("cannot use -w with standard input")
		}
		return do(os.Stdin, "<standard input>")
	}

	if !*lFlag && !*wFlag {
		return errors.New("must use -l or -w if paths are given")
	}

	for i := 0; i < flag.NArg(); i++ {
		arg := flag.Arg(i)
		switch dir, err := os.Stat(arg); {
		case err != nil:
			return err
		case dir.IsDir():
			return filepath.Walk(arg, walk)
		default:
			if err := do(nil, arg); err != nil {
				return err
			}
		}
	}

	return nil
}

func isPuffsFile(info os.FileInfo) bool {
	name := info.Name()
	return !info.IsDir() && !strings.HasPrefix(name, ".") && strings.HasSuffix(name, ".puffs")
}

func walk(filename string, info os.FileInfo, err error) error {
	if err == nil && isPuffsFile(info) {
		err = do(nil, filename)
	}
	// Don't complain if a file was deleted in the meantime (i.e. the directory
	// changed concurrently while running puffsfmt).
	if err != nil && !os.IsNotExist(err) {
		return err
	}
	return nil
}

func do(r io.Reader, filename string) error {
	src, err := []byte(nil), error(nil)
	if r != nil {
		src, err = ioutil.ReadAll(r)
	} else {
		src, err = ioutil.ReadFile(filename)
	}
	if err != nil {
		return err
	}

	idMap := token.IDMap{}
	tokens, comments, err := token.Tokenize(src, &idMap, filename)
	if err != nil {
		return err
	}
	// We don't need the AST node to pretty-print, but it's worth puffsfmt
	// rejecting syntax errors. This is just a parse, not a full type check.
	if _, err := parse.ParseFile(tokens, &idMap, filename); err != nil {
		return err
	}
	buf := &bytes.Buffer{}
	if err := render.RenderFile(buf, tokens, comments, &idMap); err != nil {
		return err
	}
	dst := buf.Bytes()

	if r != nil {
		if _, err := os.Stdout.Write(dst); err != nil {
			return err
		}
	} else if !bytes.Equal(dst, src) {
		if *lFlag {
			fmt.Println(filename)
		}
		if *wFlag {
			if err := writeFile(filename, dst); err != nil {
				return err
			}
		}
	}

	return nil
}

const chmodSupported = runtime.GOOS != "windows"

func writeFile(filename string, b []byte) error {
	info, err := os.Stat(filename)
	if err != nil {
		return err
	}
	f, err := ioutil.TempFile(filepath.Dir(filename), filepath.Base(filename))
	if err != nil {
		return err
	}
	if chmodSupported {
		f.Chmod(info.Mode().Perm())
	}
	_, werr := f.Write(b)
	cerr := f.Close()
	if werr != nil {
		os.Remove(f.Name())
		return werr
	}
	if cerr != nil {
		os.Remove(f.Name())
		return cerr
	}
	return os.Rename(f.Name(), filename)
}
