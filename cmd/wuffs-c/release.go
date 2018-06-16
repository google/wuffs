// Copyright 2018 The Wuffs Authors.
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
	"io/ioutil"
	"os"
	"os/exec"
	"time"

	cf "github.com/google/wuffs/cmd/commonflags"
)

func doGenrelease(args []string) error {
	flags := flag.FlagSet{}
	cformatterFlag := flags.String("cformatter", cf.CformatterDefault, cf.CformatterUsage)
	revisionFlag := flags.String("revision", "", "git revision the release was built from")
	versionFlag := flags.String("version", cf.VersionDefault, cf.VersionUsage)

	if err := flags.Parse(args); err != nil {
		return err
	}
	if !cf.IsAlphaNumericIsh(*cformatterFlag) {
		return fmt.Errorf("bad -cformatter flag value %q", *cformatterFlag)
	}
	if !cf.IsAlphaNumericIsh(*revisionFlag) {
		return fmt.Errorf("bad -revision flag value %q", *revisionFlag)
	}
	v, ok := cf.ParseVersion(*versionFlag)
	if !ok {
		return fmt.Errorf("bad -version flag value %q", *versionFlag)
	}
	args = flags.Args()

	h := &genReleaseHelper{
		seen:     map[string]bool{},
		revision: *revisionFlag,
		version:  v,
	}

	unformatted := bytes.NewBuffer(nil)
	unformatted.WriteString("#ifndef WUFFS_INCLUDE_GUARD\n")
	unformatted.WriteString("#define WUFFS_INCLUDE_GUARD\n\n")

	// First, cat all of the headers together, filtering out duplicate
	// WUFFS_INCLUDE_GUARD__FOO sections and overriding WUFFS_VERSION.
	for _, filename := range args {
		s, err := ioutil.ReadFile(filename)
		if err != nil {
			return err
		}

		if i := bytes.Index(s, grCHeader); i >= 0 {
			s = s[:i]
		}

		if err := h.gen(unformatted, s); err != nil {
			return fmt.Errorf("%v in %s", err, filename)
		}
	}

	unformatted.WriteString("\n\n#endif  // WUFFS_INCLUDE_GUARD\n\n")

	// Then, cat all of the implementations together, filtering out duplicate
	// WUFFS_INCLUDE_GUARD__BASE_PRIVATE sections.
	for _, filename := range args {
		s, err := ioutil.ReadFile(filename)
		if err != nil {
			return err
		}

		if i := bytes.Index(s, grCHeader); i >= 0 {
			s = s[i+len(grCHeader):]
		} else {
			continue
		}

		if err := h.gen(unformatted, s); err != nil {
			return fmt.Errorf("%v in %s", err, filename)
		}
	}

	cmd := exec.Command(*cformatterFlag, "-style=Chromium")
	cmd.Stdin = unformatted
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

var (
	grCHeader   = []byte("// !! C HEADER ENDS HERE.\n")
	grNN        = []byte("\n\n")
	grVOverride = []byte("// !! Some code generation programs can override WUFFS_VERSION.\n")
	grVString   = []byte(`#define WUFFS_VERSION_STRING "0.0.0"`)

	grDefine = []byte("#define WUFFS_INCLUDE_GUARD__")
	grEndif  = []byte("#endif  // WUFFS_INCLUDE_GUARD__")
	grIfndef = []byte("#ifndef WUFFS_INCLUDE_GUARD__")
)

type genReleaseHelper struct {
	seen     map[string]bool
	revision string
	version  cf.Version
}

func (h *genReleaseHelper) gen(w *bytes.Buffer, s []byte) error {
	skipping := 0
	stack := []string{}

	for remaining := []byte(nil); len(s) > 0; s, remaining = remaining, nil {
		if i := bytes.IndexByte(s, '\n'); i >= 0 {
			s, remaining = s[:i+1], s[i+1:]
		}
		if len(s) == 0 {
			continue
		}

		if s[0] == '#' {
			switch {
			case bytes.HasPrefix(s, grIfndef):
				pkg := string(s[len(grIfndef):])
				for _, p := range stack {
					if p == pkg {
						return fmt.Errorf("unexpected %q", s)
					}
				}

				if h.seen[pkg] {
					skipping++
				}

				stack = append(stack, pkg)
				continue

			case bytes.HasPrefix(s, grDefine):
				pkg := string(s[len(grDefine):])
				if (len(stack) == 0) || (stack[len(stack)-1] != pkg) {
					return fmt.Errorf("unexpected %q", s)
				}
				continue

			case bytes.HasPrefix(s, grEndif):
				pkg := string(s[len(grEndif):])
				if (len(stack) == 0) || (stack[len(stack)-1] != pkg) {
					return fmt.Errorf("unexpected %q", s)
				}

				if h.seen[pkg] {
					skipping--
				} else {
					h.seen[pkg] = true
				}

				stack = stack[:len(stack)-1]
				continue
			}
		}

		if skipping > 0 {
			continue
		}

		if s[0] == '/' {
			switch {
			case bytes.Equal(s, grVOverride):
				var err error
				remaining, err = h.genWuffsVersion(w, remaining)
				if err != nil {
					return err
				}
				continue
			}
		}

		w.Write(s)
	}

	if len(stack) != 0 {
		return fmt.Errorf("unmatched %q", string(grIfndef)+stack[len(stack)-1])
	}
	return nil
}

func (h *genReleaseHelper) genWuffsVersion(w *bytes.Buffer, s []byte) (remaining []byte, err error) {
	cut := []byte(nil)
	if i := bytes.Index(s, grNN); i >= 0 {
		cut, s = s[:i], s[i+len(grNN):]
	} else {
		return nil, fmt.Errorf(`could not find "\n\n" near WUFFS_VERSION`)
	}

	if !bytes.HasSuffix(cut, grVString) {
		return nil, fmt.Errorf("%q did not end with %q", cut, grVString)
	}

	fmt.Fprintf(w, "// WUFFS_VERSION was overridden by \"wuffs genrelease\" on %v UTC",
		time.Now().UTC().Format("2006-01-02"))
	if h.revision != "" {
		fmt.Fprintf(w, ",\n// based on revision %s", h.revision)
	}

	fmt.Fprintf(w, `.
		#define WUFFS_VERSION ((uint64_t)0x%016X)
		#define WUFFS_VERSION_MAJOR ((uint64_t)0x%08X)
		#define WUFFS_VERSION_MINOR ((uint64_t)0x%04X)
		#define WUFFS_VERSION_PATCH ((uint64_t)0x%04X)
		#define WUFFS_VERSION_EXTENSION %q
		#define WUFFS_VERSION_STRING %q

	`, h.version.Uint64(), h.version.Major, h.version.Minor, h.version.Patch,
		h.version.Extension, h.version)

	return s, nil
}
