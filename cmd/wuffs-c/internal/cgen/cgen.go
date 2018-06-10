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

//go:generate go run gen.go

package cgen

import (
	"bytes"
	"errors"
	"flag"
	"fmt"
	"io/ioutil"
	"math/big"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"sort"
	"strings"

	"github.com/google/wuffs/lang/base38"
	"github.com/google/wuffs/lang/builtin"
	"github.com/google/wuffs/lang/check"
	"github.com/google/wuffs/lang/generate"

	cf "github.com/google/wuffs/cmd/commonflags"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var (
	zero = big.NewInt(0)
	one  = big.NewInt(1)
)

// Prefixes are prepended to names to form a namespace and to avoid e.g.
// "double" being a valid Wuffs variable name but not a valid C one.
const (
	aPrefix = "a_" // Function argument.
	cPrefix = "c_" // Coroutine state.
	fPrefix = "f_" // Struct field.
	iPrefix = "i_" // Iterate variable.
	oPrefix = "o_" // Temporary io_bind variable.
	tPrefix = "t_" // Temporary local variable.
	uPrefix = "u_" // Derived from a local variable.
	vPrefix = "v_" // Local variable.
)

// Do transpiles a Wuffs program to a C program.
//
// The arguments list the source Wuffs files. If no arguments are given, it
// reads from stdin.
//
// The generated program is written to stdout.
func Do(args []string) error {
	flags := flag.FlagSet{}
	cformatterFlag := flags.String("cformatter", cf.CformatterDefault, cf.CformatterUsage)

	return generate.Do(&flags, args, func(pkgName string, tm *t.Map, c *check.Checker, files []*a.File) ([]byte, error) {
		if !cf.IsAlphaNumericIsh(*cformatterFlag) {
			return nil, fmt.Errorf("bad -cformatter flag value %q", *cformatterFlag)
		}

		unformatted := []byte(nil)
		if pkgName == "base" {
			if len(files) != 0 {
				return nil, fmt.Errorf("base package shouldn't have any .wuffs files")
			}
			buf := make(buffer, 0, 128*1024)
			if err := expandBangBangInsert(&buf, baseImplC, map[string]func(*buffer) error{
				"// !! INSERT base-public.h.\n": insertBasePublicH,
				"// !! INSERT wuffs_base__status__string data.\n": func(b *buffer) error {
					messages := [256]string{}
					for _, z := range builtin.StatusList {
						messages[uint8(z.Value)] = z.Message
					}
					return genStatusStringData(b, "wuffs_base__", &messages)
				},
			}); err != nil {
				return nil, err
			}
			unformatted = []byte(buf)

		} else {
			g := &gen{
				PKGPREFIX: "WUFFS_" + strings.ToUpper(pkgName) + "__",
				pkgPrefix: "wuffs_" + pkgName + "__",
				pkgName:   pkgName,
				tm:        tm,
				checker:   c,
				files:     files,
			}
			var err error
			unformatted, err = g.generate()
			if err != nil {
				return nil, err
			}
		}

		stdout := &bytes.Buffer{}
		cmd := exec.Command(*cformatterFlag, "-style=Chromium")
		cmd.Stdin = bytes.NewReader(unformatted)
		cmd.Stdout = stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err != nil {
			return nil, err
		}
		return stdout.Bytes(), nil
	})
}

type replacementPolicy bool

const (
	replaceNothing          = replacementPolicy(false)
	replaceCallSuspendibles = replacementPolicy(true)
)

type visibility uint32

const (
	bothPubPri = visibility(iota)
	pubOnly
	priOnly
)

const (
	maxIOBinds        = 100
	maxIOBindInFields = 100
	maxTemp           = 10000
)

type status struct {
	name    string
	value   int8
	msg     string
	keyword t.ID
}

type buffer []byte

func (b *buffer) Write(p []byte) (int, error) {
	*b = append(*b, p...)
	return len(p), nil
}

func (b *buffer) printf(format string, args ...interface{}) { fmt.Fprintf(b, format, args...) }
func (b *buffer) writeb(x byte)                             { *b = append(*b, x) }
func (b *buffer) writes(s string)                           { *b = append(*b, s...) }
func (b *buffer) writex(s []byte)                           { *b = append(*b, s...) }

func expandBangBangInsert(b *buffer, s string, m map[string]func(*buffer) error) error {
	for {
		remaining := ""
		if i := strings.IndexByte(s, '\n'); i >= 0 {
			s, remaining = s[:i+1], s[i+1:]
		}

		const prefix = "// !! INSERT "
		if !strings.HasPrefix(s, prefix) {
			b.writes(s)
		} else if f := m[s]; f == nil {
			msg := []byte(fmt.Sprintf("unrecognized line %q, want one of:\n", s))
			keys := []string(nil)
			for k := range m {
				keys = append(keys, k)
			}
			sort.Strings(keys)
			for _, k := range keys {
				msg = append(msg, '\t')
				msg = append(msg, k...)
			}
			return errors.New(string(msg))
		} else if err := f(b); err != nil {
			return err
		}

		if remaining == "" {
			break
		}
		s = remaining
	}
	return nil
}

func insertBasePublicH(buf *buffer) error {
	buf.writes("// Code generated by wuffs-c. DO NOT EDIT.\n\n")
	if err := expandBangBangInsert(buf, basePublicH, map[string]func(*buffer) error{
		"// !! INSERT wuffs_base__status names.\n": func(b *buffer) error {
			for _, z := range builtin.StatusList {
				code := int32(z.Value) << 24
				b.printf("#define %s %d // 0x%08X\n",
					strings.ToUpper(cName(z.String(), "WUFFS_BASE__")), code, uint32(code))
			}
			b.writes("\n")
			return nil
		},
	}); err != nil {
		return err
	}
	buf.writeb('\n')
	return nil
}

func genStatusStringData(b *buffer, pkgPrefix string, ss *[256]string) error {
	data := []byte{0x00}
	offsets := [256]uint16{}
	for i, s := range ss {
		if s == "" {
			continue
		}
		if len(data) > 0xFFFF {
			return errors.New("status string messages are too long")
		}
		offsets[i] = uint16(len(data))
		data = append(data, s...)
		data = append(data, 0x00)
	}

	b.printf("static const char %sstatus__string_data[] = {\n", pkgPrefix)
	for _, x := range data {
		b.printf("0x%02X,", x)
	}
	b.writes("};\n\n")

	b.printf("static const uint16_t %sstatus__string_offsets[] = {\n", pkgPrefix)
	for _, x := range offsets {
		b.printf("0x%04X,", x)
	}
	b.writes("};\n\n")

	return nil
}

type gen struct {
	PKGPREFIX string // e.g. "WUFFS_JPEG__"
	pkgPrefix string // e.g. "wuffs_jpeg__"
	pkgName   string // e.g. "jpeg"

	tm      *t.Map
	checker *check.Checker
	files   []*a.File

	statusList []status
	statusMap  map[t.QID]status
	structList []*a.Struct
	structMap  map[t.QID]*a.Struct
	usesList   []string
	usesMap    map[string]struct{}

	currFunk  funk
	funks     map[t.QQID]funk
	wuffsRoot string
}

func (g *gen) generate() ([]byte, error) {
	b := new(buffer)

	g.statusMap = map[t.QID]status{}
	if err := g.forEachStatus(b, bothPubPri, (*gen).gatherStatuses); err != nil {
		return nil, err
	}

	// Make a topologically sorted list of structs.
	unsortedStructs := []*a.Struct(nil)
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() == a.KStruct {
				unsortedStructs = append(unsortedStructs, tld.Struct())
			}
		}
	}
	var ok bool
	g.structList, ok = a.TopologicalSortStructs(unsortedStructs)
	if !ok {
		return nil, fmt.Errorf("cyclical struct definitions")
	}
	g.structMap = map[t.QID]*a.Struct{}
	for _, n := range g.structList {
		g.structMap[n.QID()] = n
	}

	g.funks = map[t.QQID]funk{}
	if err := g.forEachFunc(nil, bothPubPri, (*gen).gatherFuncImpl); err != nil {
		return nil, err
	}

	if err := g.genHeader(b); err != nil {
		return nil, err
	}
	b.writes("// C HEADER ENDS HERE.\n\n")
	if err := g.genImpl(b); err != nil {
		return nil, err
	}
	return *b, nil
}

func (g *gen) genHeader(b *buffer) error {
	includeGuard := "WUFFS_" + strings.ToUpper(g.pkgName) + "_H"
	b.printf("#ifndef %s\n#define %s\n\n", includeGuard, includeGuard)

	if err := insertBasePublicH(b); err != nil {
		return err
	}

	b.writes("// ---------------- Use Declarations\n\n")
	if err := g.forEachUse(b, (*gen).writeUse); err != nil {
		return err
	}

	b.writes("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")

	b.writes("// ---------------- Status Codes\n\n")

	b.printf("typedef int32_t %sstatus;\n\n", g.pkgPrefix)
	pkgID := g.checker.PackageID()
	b.printf("#define %spackageid %d // 0x%08X\n\n", g.pkgPrefix, pkgID, pkgID)

	for _, z := range builtin.StatusList {
		name := z.Message
		if z.Keyword == t.IDError {
			name = "error " + name
		} else if z.Keyword == t.IDSuspension {
			name = "suspension " + name
		} else {
			name = "status " + name
		}
		code := int32(z.Value) << 24
		b.printf("#define %s %d // 0x%08X\n",
			strings.ToUpper(cName(name, g.PKGPREFIX)), code, uint32(code))
	}
	b.writes("\n")

	for _, s := range g.statusList {
		code := (int32(s.value) << 24) | int32(pkgID)
		b.printf("#define %s %d // 0x%08X\n", s.name, code, uint32(code))
	}
	b.writes("\n")

	b.printf("bool %sstatus__is_error(%sstatus s);\n\n", g.pkgPrefix, g.pkgPrefix)
	b.printf("const char* %sstatus__string(%sstatus s);\n\n", g.pkgPrefix, g.pkgPrefix)

	b.writes("// ---------------- Public Consts\n\n")
	if err := g.forEachConst(b, pubOnly, (*gen).writeConst); err != nil {
		return err
	}

	b.writes("// ---------------- Structs\n\n")
	for _, n := range g.structList {
		if err := g.writeStruct(b, n); err != nil {
			return err
		}
	}

	b.writes("// ---------------- Public Initializer Prototypes\n\n")
	for _, n := range g.structList {
		if n.Public() {
			if err := g.writeInitializerPrototype(b, n); err != nil {
				return err
			}
		}
	}

	b.writes("// ---------------- Public Function Prototypes\n\n")
	if err := g.forEachFunc(b, pubOnly, (*gen).writeFuncPrototype); err != nil {
		return err
	}

	b.writes("\n#ifdef __cplusplus\n}  // extern \"C\"\n#endif\n\n")
	b.printf("#endif  // %s\n\n", includeGuard)
	return nil
}

func (g *gen) genImpl(b *buffer) error {
	b.writes("#ifndef WUFFS_BASE_PRIVATE_H\n#define WUFFS_BASE_PRIVATE_H\n\n")
	b.writes(basePrivateH)
	b.writes("#endif  // WUFFS_BASE_PRIVATE_H\n\n")

	b.writes("// ---------------- Status Codes Implementations\n\n")
	b.printf("bool %sstatus__is_error(%sstatus s) { return s < 0; }\n\n", g.pkgPrefix, g.pkgPrefix)

	{
		messages := [256]string{}
		for _, s := range g.statusList {
			messages[uint8(s.value)] = g.pkgName + ": " + s.msg
		}
		if err := genStatusStringData(b, g.pkgPrefix, &messages); err != nil {
			return err
		}
	}

	b.printf("const char* %sstatus__string(%sstatus s) {\n", g.pkgPrefix, g.pkgPrefix)
	b.printf("uint16_t o;")
	b.printf("switch (s & 0x%X) {\n", (1<<base38.MaxBits)-1)
	b.printf("case 0: return wuffs_base__status__string(s);\n")
	b.printf("case %spackageid:\n", g.pkgPrefix)
	b.printf("o = %sstatus__string_offsets[(uint8_t)(s >> 24)];\n", g.pkgPrefix)
	b.printf("if (o) { return %sstatus__string_data + o; } break;\n", g.pkgPrefix)
	for _, u := range g.usesList {
		// TODO: is path.Base always correct? Should we check
		// validName(packageName)?
		useePkgName := path.Base(u)
		b.printf("case wuffs_%s__packageid: return wuffs_%s__status__string(s);\n", useePkgName, useePkgName)
	}
	b.printf("}\n")
	b.printf("return \"unknown status\";\n")

	b.writes("}\n\n")

	b.writes("// ---------------- Private Consts\n\n")
	if err := g.forEachConst(b, priOnly, (*gen).writeConst); err != nil {
		return err
	}

	b.writes("// ---------------- Private Initializer Prototypes\n\n")
	for _, n := range g.structList {
		if !n.Public() {
			if err := g.writeInitializerPrototype(b, n); err != nil {
				return err
			}
		}
	}

	b.writes("// ---------------- Private Function Prototypes\n\n")
	if err := g.forEachFunc(b, priOnly, (*gen).writeFuncPrototype); err != nil {
		return err
	}

	b.writes("// ---------------- Initializer Implementations\n\n")
	for _, n := range g.structList {
		if err := g.writeInitializerImpl(b, n); err != nil {
			return err
		}
	}

	b.writes("// ---------------- Function Implementations\n\n")
	if err := g.forEachFunc(b, bothPubPri, (*gen).writeFuncImpl); err != nil {
		return err
	}

	return nil
}

func (g *gen) forEachConst(b *buffer, v visibility, f func(*gen, *buffer, *a.Const) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KConst ||
				(v == pubOnly && tld.Raw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.Raw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, b, tld.Const()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) forEachFunc(b *buffer, v visibility, f func(*gen, *buffer, *a.Func) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KFunc ||
				(v == pubOnly && tld.Raw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.Raw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, b, tld.Func()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) forEachStatus(b *buffer, v visibility, f func(*gen, *buffer, *a.Status) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KStatus ||
				(v == pubOnly && tld.Raw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.Raw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, b, tld.Status()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) forEachUse(b *buffer, f func(*gen, *buffer, *a.Use) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KUse {
				continue
			}
			if err := f(g, b, tld.Use()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) cName(name string) string {
	return cName(name, g.pkgPrefix)
}

func cName(name string, pkgPrefix string) string {
	s := []byte(pkgPrefix)
	underscore := true
	for _, r := range name {
		if 'A' <= r && r <= 'Z' {
			s = append(s, byte(r+'a'-'A'))
			underscore = false
		} else if ('a' <= r && r <= 'z') || ('0' <= r && r <= '9') {
			s = append(s, byte(r))
			underscore = false
		} else if !underscore {
			s = append(s, '_')
			underscore = true
		}
	}
	if underscore {
		s = s[:len(s)-1]
	}
	return string(s)
}

func uintBits(qid t.QID) uint32 {
	if qid[0] == t.IDBase {
		switch qid[1] {
		case t.IDU8:
			return 8
		case t.IDU16:
			return 16
		case t.IDU32:
			return 32
		case t.IDU64:
			return 64
		}
	}
	return 0
}

func (g *gen) sizeof(typ *a.TypeExpr) (uint32, error) {
	if typ.Decorator() == 0 {
		if n := uintBits(typ.QID()); n != 0 {
			return n / 8, nil
		}
	}
	return 0, fmt.Errorf("unknown sizeof for %q", typ.Str(g.tm))
}

func (g *gen) gatherStatuses(b *buffer, n *a.Status) error {
	raw := n.QID()[1].Str(g.tm)
	msg, ok := t.Unescape(raw)
	if !ok {
		return fmt.Errorf("bad status message %q", raw)
	}
	prefix := "SUSPENSION_"
	if n.Keyword() == t.IDError {
		prefix = "ERROR_"
	}
	value := int8(n.Value().ConstValue().Int64())
	if n.Keyword() == t.IDError {
		value = -value
	}
	s := status{
		name:    strings.ToUpper(g.cName(prefix + msg)),
		value:   value,
		msg:     msg,
		keyword: n.Keyword(),
	}
	g.statusList = append(g.statusList, s)
	g.statusMap[n.QID()] = s
	return nil
}

func (g *gen) writeConst(b *buffer, n *a.Const) error {
	if !n.Public() {
		b.writes("static ")
	}
	b.writes("const ")
	if err := g.writeCTypeName(b, n.XType(), g.pkgPrefix, n.QID()[1].Str(g.tm)); err != nil {
		return err
	}
	b.writes(" = ")
	if err := g.writeConstList(b, n.Value()); err != nil {
		return err
	}
	b.writes(";\n\n")
	return nil
}

func (g *gen) writeConstList(b *buffer, n *a.Expr) error {
	if n.Operator() == t.IDDollar {
		b.writeb('{')
		for _, o := range n.Args() {
			if err := g.writeConstList(b, o.Expr()); err != nil {
				return err
			}
			b.writeb(',')
		}
		b.writeb('}')
	} else if cv := n.ConstValue(); cv != nil {
		b.writes(cv.String())
	} else {
		return fmt.Errorf("invalid const value %q", n.Str(g.tm))
	}
	return nil
}

func (g *gen) writeStruct(b *buffer, n *a.Struct) error {
	// For API/ABI compatibility, the very first field in the struct's
	// private_impl must be the status code. This lets the initializer callee
	// set "self->private_impl.status = WUFFS_ETC__ERROR_BAD_WUFFS_VERSION;"
	// regardless of the sizeof(*self) struct reserved by the caller and even
	// if the caller and callee were built with different versions.
	structName := n.QID().Str(g.tm)
	b.writes("typedef struct {\n")
	b.writes("// Do not access the private_impl's fields directly. There is no API/ABI\n")
	b.writes("// compatibility or safety guarantee if you do so. Instead, use the\n")
	b.printf("// %s%s__etc functions.\n", g.pkgPrefix, structName)
	b.writes("//\n")
	b.writes("// In C++, these fields would be \"private\", but C does not support that.\n")
	b.writes("//\n")
	b.writes("// It is a struct, not a struct*, so that it can be stack allocated.\n")
	b.writes("struct {\n")
	if n.Suspendible() {
		b.printf("%sstatus status;\n", g.pkgPrefix)
		b.writes("uint32_t magic;\n")
		b.writes("\n")
	}

	for _, o := range n.Fields() {
		o := o.Field()
		if err := g.writeCTypeName(b, o.XType(), fPrefix, o.Name().Str(g.tm)); err != nil {
			return err
		}
		b.writes(";\n")
	}

	if n.Suspendible() {
		b.writeb('\n')
		for _, file := range g.files {
			for _, tld := range file.TopLevelDecls() {
				if tld.Kind() != a.KFunc {
					continue
				}
				o := tld.Func()
				if o.Receiver() != n.QID() || !o.Suspendible() {
					continue
				}
				k := g.funks[o.QQID()]
				if k.coroSuspPoint == 0 && !k.usesScratch {
					continue
				}
				// TODO: allow max depth > 1 for recursive coroutines.
				const maxDepth = 1
				b.writes("struct {\n")
				if k.coroSuspPoint != 0 {
					b.writes("uint32_t coro_susp_point;\n")
					if err := g.writeVars(b, o.Body(), true, true); err != nil {
						return err
					}
				}
				if k.usesScratch {
					b.writes("uint64_t scratch;\n")
				}
				b.printf("} %s%s[%d];\n", cPrefix, o.FuncName().Str(g.tm), maxDepth)
			}
		}
	}

	b.printf("} private_impl;\n } %s%s;\n\n", g.pkgPrefix, structName)
	return nil
}

var (
	wuffsBasePublicHStart = []byte("#ifndef WUFFS_BASE_PUBLIC_H\n")
	wuffsBasePublicHEnd   = []byte("#endif  // WUFFS_BASE_PUBLIC_H\n")
)

func (g *gen) writeUse(b *buffer, n *a.Use) error {
	useDirname := g.tm.ByID(n.Path())
	useDirname, _ = t.Unescape(useDirname)

	// TODO: sanity check useDirname via commonflags.IsValidUsePath?

	if g.usesMap == nil {
		g.usesMap = map[string]struct{}{}
	} else if _, ok := g.usesMap[useDirname]; ok {
		return nil
	}
	g.usesList = append(g.usesList, useDirname)
	g.usesMap[useDirname] = struct{}{}

	if g.wuffsRoot == "" {
		var err error
		g.wuffsRoot, err = generate.WuffsRoot()
		if err != nil {
			return err
		}
	}

	hdrFilename := filepath.Join(g.wuffsRoot, "gen", "h", filepath.FromSlash(useDirname)+".h")
	hdr, err := ioutil.ReadFile(hdrFilename)
	if err != nil {
		return err
	}

	b.printf("// ---------------- BEGIN USE %q\n\n", useDirname)

	// Inline the previously generated .h file, stripping out the redundant
	// copy of the WUFFS_BASE_PUBLIC_H code.
	if i := bytes.Index(hdr, wuffsBasePublicHStart); i < 0 {
		return fmt.Errorf("use %q: previously generated header %q could not be inlined", useDirname, hdrFilename)
	} else {
		b.Write(hdr[:i])
		hdr = hdr[i+len(wuffsBasePublicHStart):]
	}
	if i := bytes.Index(hdr, wuffsBasePublicHEnd); i < 0 {
		return fmt.Errorf("use %q: previously generated header %q could not be inlined", useDirname, hdrFilename)
	} else {
		b.Write(hdr[i+len(wuffsBasePublicHEnd):])
	}

	b.writeb('\n')
	b.printf("// ---------------- END   USE %q\n\n", useDirname)
	return nil
}

func (g *gen) writeInitializerSignature(b *buffer, n *a.Struct, public bool) error {
	structName := n.QID().Str(g.tm)
	if public {
		b.printf("// %s%s__check_wuffs_version is an initializer function.\n", g.pkgPrefix, structName)
		b.printf("//\n")
		b.printf("// It should be called before any other %s%s__* function.\n", g.pkgPrefix, structName)
		b.printf("//\n")
		b.printf("// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.\n")
	}
	b.printf("void %s%s__check_wuffs_version(%s%s *self, size_t sizeof_star_self, uint64_t wuffs_version)",
		g.pkgPrefix, structName, g.pkgPrefix, structName)
	return nil
}

func (g *gen) writeInitializerPrototype(b *buffer, n *a.Struct) error {
	if !n.Suspendible() {
		return nil
	}
	if err := g.writeInitializerSignature(b, n, n.Public()); err != nil {
		return err
	}
	b.writes(";\n\n")
	return nil
}

func (g *gen) writeInitializerImpl(b *buffer, n *a.Struct) error {
	if !n.Suspendible() {
		return nil
	}
	if err := g.writeInitializerSignature(b, n, false); err != nil {
		return err
	}
	b.printf("{\n")
	b.printf("if (!self) { return; }\n")

	b.printf("if (sizeof(*self) != sizeof_star_self) {\n")
	b.printf("self->private_impl.status = %sERROR_BAD_SIZEOF_RECEIVER;\n", g.PKGPREFIX)
	b.printf("return;\n")
	b.printf("}\n")
	b.printf("if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) || " +
		"(((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {\n")
	b.printf("self->private_impl.status = %sERROR_BAD_WUFFS_VERSION;\n", g.PKGPREFIX)
	b.printf("return;\n")
	b.printf("}\n")
	b.printf("if (self->private_impl.magic != 0) {\n")
	b.printf("self->private_impl.status = %sERROR_CHECK_WUFFS_VERSION_CALLED_TWICE;\n", g.PKGPREFIX)
	b.printf("return;\n")
	b.printf("}\n")

	b.writes("self->private_impl.magic = WUFFS_BASE__MAGIC;\n")

	// Call any ctors on sub-structs.
	for _, f := range n.Fields() {
		f := f.Field()
		x := f.XType()
		if x != x.Innermost() {
			// TODO: arrays of sub-structs.
			continue
		}

		prefix := g.pkgPrefix
		qid := x.QID()
		if qid[0] == t.IDBase {
			// Base types don't need further initialization.
			continue
		} else if qid[0] != 0 {
			// See gen.packagePrefix for a related TODO with otherPkg.
			otherPkg := g.tm.ByID(qid[0])
			prefix = "wuffs_" + otherPkg + "__"
		} else if g.structMap[qid] == nil {
			continue
		}

		b.printf("%s%s__check_wuffs_version(&self->private_impl.%s%s, sizeof(self->private_impl.%s%s), WUFFS_VERSION);\n",
			prefix, qid[1].Str(g.tm), fPrefix, f.Name().Str(g.tm), fPrefix, f.Name().Str(g.tm))
	}

	b.writes("}\n\n")
	return nil
}
