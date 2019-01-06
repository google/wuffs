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
	"path/filepath"
	"sort"
	"strings"

	cf "github.com/google/wuffs/cmd/commonflags"
)

func doGenrelease(args []string) error {
	flags := flag.FlagSet{}
	cformatterFlag := flags.String("cformatter", cf.CformatterDefault, cf.CformatterUsage)
	commitDateFlag := flags.String("commitdate", "", "git commit date the release was built from")
	gitRevListCountFlag := flags.Int("gitrevlistcount", 0, `git "rev-list --count" that the release was built from`)
	revisionFlag := flags.String("revision", "", "git revision the release was built from")
	versionFlag := flags.String("version", cf.VersionDefault, cf.VersionUsage)

	if err := flags.Parse(args); err != nil {
		return err
	}
	if !cf.IsAlphaNumericIsh(*cformatterFlag) {
		return fmt.Errorf("bad -cformatter flag value %q", *cformatterFlag)
	}
	if (*gitRevListCountFlag < 0) || (0x7FFFFFFF < *gitRevListCountFlag) {
		return fmt.Errorf("bad -gitrevlistcount flag value %d", *gitRevListCountFlag)
	}
	if !cf.IsAlphaNumericIsh(*commitDateFlag) {
		return fmt.Errorf("bad -commitdate flag value %q", *commitDateFlag)
	}
	if !cf.IsAlphaNumericIsh(*revisionFlag) {
		return fmt.Errorf("bad -revision flag value %q", *revisionFlag)
	}
	v, ok := cf.ParseVersion(*versionFlag)
	if !ok {
		return fmt.Errorf("bad -version flag value %q", *versionFlag)
	}
	args = flags.Args()

	// Calculate the base directory.
	baseDir := ""
	for _, filename := range args {
		if filepath.Base(filename) != "wuffs-base.c" {
			continue
		}
		baseDir = filepath.Dir(filename)
	}
	if baseDir == "" {
		return fmt.Errorf("could not determine base directory")
	}
	baseDirSlash := baseDir + string(filepath.Separator)

	h := &genReleaseHelper{
		filesList:       nil,
		filesMap:        map[string]parsedCFile{},
		seen:            nil,
		commitDate:      *commitDateFlag,
		gitRevListCount: *gitRevListCountFlag,
		revision:        *revisionFlag,
		version:         v,
	}

	for _, filename := range args {
		if !strings.HasPrefix(filename, baseDirSlash) {
			return fmt.Errorf("filename %q is not under base directory %q", filename, baseDir)
		}
		relFilename := filename[len(baseDirSlash):]

		s, err := ioutil.ReadFile(filename)
		if err != nil {
			return err
		}

		if err := h.parse(relFilename, s); err != nil {
			return err
		}
	}
	sort.Strings(h.filesList)

	unformatted := bytes.NewBuffer(nil)
	unformatted.WriteString("#ifndef WUFFS_INCLUDE_GUARD\n")
	unformatted.WriteString("#define WUFFS_INCLUDE_GUARD\n\n")
	unformatted.WriteString(grSingleFileGuidance)

	h.seen = map[string]bool{}
	for _, f := range h.filesList {
		if err := h.gen(unformatted, f, 0, 0); err != nil {
			return err
		}
	}

	unformatted.Write(grImplStartsHere)
	unformatted.WriteString("\n")

	h.seen = map[string]bool{}
	for _, f := range h.filesList {
		if err := h.gen(unformatted, f, 1, 0); err != nil {
			return err
		}
	}

	unformatted.WriteString("\n")
	unformatted.Write(grImplEndsHere)
	unformatted.WriteString("\n\n#endif  // WUFFS_INCLUDE_GUARD\n\n")

	cmd := exec.Command(*cformatterFlag, "-style=Chromium")
	cmd.Stdin = unformatted
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

var (
	grImplStartsHere = []byte("\n// WUFFS C HEADER ENDS HERE.\n#ifdef WUFFS_IMPLEMENTATION\n")
	grImplEndsHere   = []byte("#endif  // WUFFS_IMPLEMENTATION\n")
	grIncludeQuote   = []byte("#include \"")
	grNN             = []byte("\n\n")
	grVOverride      = []byte("// !! Some code generation programs can override WUFFS_VERSION.\n")
	grVEnd           = []byte(`#define WUFFS_VERSION_STRING "0.0.0+0.00000000"`)
	grWmrAbove       = []byte("// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING ABOVE.\n")
	grWmrBelow       = []byte("// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING BELOW.\n")
)

const grSingleFileGuidance = `
// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.

`

type parsedCFile struct {
	includes []string
	// fragments[0] is the header, fragments[1] is the implementation.
	fragments [2][]byte
}

type genReleaseHelper struct {
	filesList       []string
	filesMap        map[string]parsedCFile
	seen            map[string]bool
	commitDate      string
	gitRevListCount int
	revision        string
	version         cf.Version
}

func (h *genReleaseHelper) parse(relFilename string, s []byte) error {
	if _, ok := h.filesMap[relFilename]; ok {
		return fmt.Errorf("duplicate %q", relFilename)
	}

	f := parsedCFile{}

	if i := bytes.Index(s, grWmrAbove); i < 0 {
		return fmt.Errorf("could not find %q in %s", grWmrAbove, relFilename)
	} else {
		f.includes = parseIncludes(s[:i])
		s = s[i+len(grWmrAbove):]
	}

	if i := bytes.LastIndex(s, grWmrBelow); i < 0 {
		return fmt.Errorf("could not find %q in %s", grWmrBelow, relFilename)
	} else {
		s = s[:i]
	}

	if i := bytes.Index(s, grImplStartsHere); i < 0 {
		return fmt.Errorf("could not find %q in %s", grImplStartsHere, relFilename)
	} else {
		f.fragments[0], s = s[:i], s[i+len(grImplStartsHere):]
	}

	if i := bytes.LastIndex(s, grImplEndsHere); i < 0 {
		return fmt.Errorf("could not find %q in %s", grImplEndsHere, relFilename)
	} else {
		f.fragments[1] = s[:i]
	}

	if relFilename == "wuffs-base.c" && (h.version != cf.Version{}) {
		if subs, err := h.substituteWuffsVersion(f.fragments[0]); err != nil {
			return err
		} else {
			f.fragments[0] = subs
		}
	}

	h.filesList = append(h.filesList, relFilename)
	h.filesMap[relFilename] = f
	return nil
}

func parseIncludes(s []byte) (ret []string) {
	for remaining := []byte(nil); len(s) > 0; s, remaining = remaining, nil {
		if i := bytes.IndexByte(s, '\n'); i >= 0 {
			s, remaining = s[:i+1], s[i+1:]
		}
		if len(s) == 0 || s[0] != '#' || !bytes.HasPrefix(s, grIncludeQuote) {
			continue
		}
		s = s[len(grIncludeQuote):]
		if len(s) < 2 || s[len(s)-2] != '"' || s[len(s)-1] != '\n' {
			continue
		}
		s = s[:len(s)-2]

		ret = append(ret, string(s))
	}
	sort.Strings(ret)
	return ret
}

func (h *genReleaseHelper) gen(w *bytes.Buffer, relFilename string, which int, depth uint32) error {
	if depth > 1024 {
		return fmt.Errorf("genrelease recursion depth too large")
	}
	depth++

	if strings.HasPrefix(relFilename, "./") {
		relFilename = relFilename[2:]
	}

	if h.seen[relFilename] {
		return nil
	}
	f, ok := h.filesMap[relFilename]
	if !ok {
		return fmt.Errorf("cannot resolve %q", relFilename)
	}

	// Process the files in #include-ee before #include-er order.
	for _, inc := range f.includes {
		if err := h.gen(w, inc, which, depth); err != nil {
			return err
		}
	}

	w.Write(f.fragments[which])
	h.seen[relFilename] = true
	return nil
}

func (h *genReleaseHelper) substituteWuffsVersion(s []byte) ([]byte, error) {
	ret := []byte(nil)
	if i := bytes.Index(s, grVOverride); i < 0 {
		return nil, fmt.Errorf("could not find %q in %s", grVOverride, "wuffs-base.c")
	} else {
		ret = append(ret, s[:i]...)
		s = s[i+len(grVOverride):]
	}

	cut := []byte(nil)
	if i := bytes.Index(s, grNN); i < 0 {
		return nil, fmt.Errorf(`could not find "\n\n" near WUFFS_VERSION`)
	} else {
		cut, s = s[:i], s[i+len(grNN):]
	}

	if !bytes.HasSuffix(cut, grVEnd) {
		return nil, fmt.Errorf("%q did not end with %q", cut, grVEnd)
	}

	w := bytes.NewBuffer(nil)
	fmt.Fprintf(w, `// WUFFS_VERSION was overridden by "wuffs gen -version"`)
	if h.revision != "" && h.commitDate != "" {
		fmt.Fprintf(w, " based on revision\n// %s committed on %s", h.revision, h.commitDate)
	}

	commitDate := "0"
	if h.commitDate != "" {
		commitDate = strings.Replace(h.commitDate, "-", "", -1)
	}

	buildMetadata := ""
	if h.gitRevListCount != 0 {
		buildMetadata = fmt.Sprintf("+%d.%s", h.gitRevListCount, commitDate)
	}

	fmt.Fprintf(w, `.
		#define WUFFS_VERSION ((uint64_t)0x%016X)
		#define WUFFS_VERSION_MAJOR ((uint64_t)0x%08X)
		#define WUFFS_VERSION_MINOR ((uint64_t)0x%04X)
		#define WUFFS_VERSION_PATCH ((uint64_t)0x%04X)
		#define WUFFS_VERSION_PRE_RELEASE_LABEL %q
		#define WUFFS_VERSION_BUILD_METADATA_COMMIT_COUNT %d
		#define WUFFS_VERSION_BUILD_METADATA_COMMIT_DATE %s
		#define WUFFS_VERSION_STRING %q

	`, h.version.Uint64(), h.version.Major, h.version.Minor, h.version.Patch,
		h.version.Extension, h.gitRevListCount, commitDate,
		h.version.String()+buildMetadata)

	ret = append(ret, w.Bytes()...)
	ret = append(ret, s...)
	return ret, nil
}
