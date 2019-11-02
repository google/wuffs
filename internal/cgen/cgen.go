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
	"math/big"
	"os"
	"os/exec"
	"sort"
	"strings"

	"github.com/google/wuffs/lang/builtin"
	"github.com/google/wuffs/lang/check"
	"github.com/google/wuffs/lang/generate"

	cf "github.com/google/wuffs/cmd/commonflags"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var (
	zero      = big.NewInt(0)
	one       = big.NewInt(1)
	eight     = big.NewInt(8)
	sixtyFour = big.NewInt(64)

	mibi = big.NewInt(1 << 20)

	typeExprUtility = a.NewTypeExpr(0, t.IDBase, t.IDUtility, nil, nil, nil)
)

// Prefixes are prepended to names to form a namespace and to avoid e.g.
// "double" being a valid Wuffs variable name but not a valid C one.
const (
	aPrefix = "a_" // Function argument.
	fPrefix = "f_" // Struct field.
	iPrefix = "i_" // Iterate variable.
	oPrefix = "o_" // Temporary io_bind variable.
	pPrefix = "p_" // Coroutine suspension point (program counter).
	sPrefix = "s_" // Coroutine stack (saved local variables).
	tPrefix = "t_" // Temporary local variable.
	uPrefix = "u_" // Derived from a local variable.
	vPrefix = "v_" // Local variable.
)

// I/O (reader/writer) prefixes. In the generated C code, the variables with
// these prefixes all have type uint8_t*. The iop_etc variables are the key
// ones. For an io_reader or io_writer function argument a_src or a_dst,
// reading or writing the next byte (and advancing the stream) is essentially
// "etc = *iop_a_src++" or "*io_a_dst++ = etc".
//
// The other two prefixes, giving names like io1_etc and io2_etc, are
// auxilliary pointers: lower and upper inclusive bounds. As an iop_etc pointer
// advances, it cannot advance past io2_etc. In the rarer case that an iop_etc
// pointer retreats, undoing a read or write, it cannot retreat past io1_etc.
//
// The iop_etc pointer can change over the lifetime of a function. The ioN_etc
// pointers, for numeric N, cannot.
//
// At the start of a function, these pointers are initialized from an
// io_buffer's fields (ptr, ri, wi, len). For an io_reader:
//  - io0_etc = ptr
//  - io1_etc = ptr + ri
//  - iop_etc = ptr + ri
//  - io2_etc = ptr + wi
// and for an io_writer:
//  - io0_etc = ptr
//  - io1_etc = ptr + wi
//  - iop_etc = ptr + wi
//  - io2_etc = ptr + len
const (
	io0Prefix = "io0_" // Base.
	io1Prefix = "io1_" // Lower bound.
	io2Prefix = "io2_" // Upper bound.
	iopPrefix = "iop_" // Pointer.
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
	genlinenumFlag := flags.Bool("genlinenum", cf.GenlinenumDefault, cf.GenlinenumUsage)

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
			if err := expandBangBangInsert(&buf, baseAllImplC, map[string]func(*buffer) error{
				"// !! INSERT base/all-private.h.\n": insertBaseAllPrivateH,
				"// !! INSERT base/all-public.h.\n":  insertBaseAllPublicH,
				"// !! INSERT base/copyright\n":      insertBaseCopyright,
				"// !! INSERT base/image-impl.c.\n":  insertBaseImageImplC,
				"// !! INSERT wuffs_base__status strings.\n": func(b *buffer) error {
					for _, z := range builtin.Statuses {
						msg, _ := t.Unescape(z)
						if msg == "" {
							continue
						}
						pre := "warning"
						if msg[0] == '$' {
							pre = "suspension"
						} else if msg[0] == '#' {
							pre = "error"
						}
						b.printf("const char* wuffs_base__%s__%s = \"%sbase: %s\";\n",
							pre, cName(msg, ""), msg[:1], msg[1:])
					}
					return nil
				},
			}); err != nil {
				return nil, err
			}
			unformatted = []byte(buf)

		} else {
			g := &gen{
				pkgPrefix:  "wuffs_" + pkgName + "__",
				pkgName:    pkgName,
				tm:         tm,
				checker:    c,
				files:      files,
				genlinenum: *genlinenumFlag,
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

type visibility uint32

const (
	bothPubPri = visibility(iota)
	pubOnly
	priOnly
)

const (
	maxIOBinds = 100
	maxTemp    = 10000
)

type status struct {
	cName       string
	msg         string
	fromThisPkg bool
	public      bool
}

func statusMsgIsError(msg string) bool {
	return (len(msg) != 0) && (msg[0] == '#')
}

func statusMsgIsSuspension(msg string) bool {
	return (len(msg) != 0) && (msg[0] == '$')
}

func statusMsgIsWarning(msg string) bool {
	return (len(msg) == 0) || (msg[0] != '$' && msg[0] != '#')
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

func insertBaseAllPrivateH(buf *buffer) error {
	buf.writes(baseCorePrivateH)
	buf.writeb('\n')
	buf.writes(baseRangePrivateH)
	buf.writeb('\n')
	buf.writes(baseIOPrivateH)
	buf.writeb('\n')
	buf.writes(baseMemoryPrivateH)
	buf.writeb('\n')
	buf.writes(baseImagePrivateH)
	buf.writeb('\n')
	return nil
}

func insertBaseAllPublicH(buf *buffer) error {
	if err := expandBangBangInsert(buf, baseCorePublicH, map[string]func(*buffer) error{
		"// !! INSERT FourCCs.\n": func(b *buffer) error {
			for _, z := range builtin.FourCCs {
				b.printf("// %s.\n#define WUFFS_BASE__FOURCC__%s 0x%02X%02X%02X%02X\n\n",
					z[1],
					strings.ToUpper(strings.TrimSpace(z[0])),
					z[0][0],
					z[0][1],
					z[0][2],
					z[0][3],
				)
			}
			return nil
		},
		"// !! INSERT wuffs_base__status names.\n": func(b *buffer) error {
			for _, z := range builtin.Statuses {
				msg, _ := t.Unescape(z)
				if msg == "" {
					return fmt.Errorf("bad built-in status %q", z)
				}
				pre := "warning"
				if statusMsgIsError(msg) {
					pre = "error"
				} else if statusMsgIsSuspension(msg) {
					pre = "suspension"
				}
				b.printf("extern const char* wuffs_base__%s__%s;\n", pre, cName(msg, ""))
			}
			return nil
		},
	}); err != nil {
		return err
	}
	buf.writeb('\n')

	buf.writes(baseRangePublicH)
	buf.writeb('\n')
	buf.writes(baseIOPublicH)
	buf.writeb('\n')
	buf.writes(baseMemoryPublicH)
	buf.writeb('\n')
	buf.writes(baseImagePublicH)
	buf.writeb('\n')
	return nil
}

func insertBaseCopyright(buf *buffer) error {
	buf.writes(baseCopyright)
	buf.writeb('\n')
	return nil
}

func insertBaseImageImplC(buf *buffer) error {
	buf.writes(baseImageImplC)
	buf.writeb('\n')
	return nil
}

type gen struct {
	pkgPrefix string // e.g. "wuffs_jpeg__"
	pkgName   string // e.g. "jpeg"

	tm      *t.Map
	checker *check.Checker
	files   []*a.File

	// genlinenum is whether to print "// foo.wuffs:123" comments in the
	// generated C code. This can be useful for debugging, although it is not
	// enabled by default as it can lead to many spurious changes in the
	// generated C code (due to line numbers changing) when editing Wuffs code.
	genlinenum bool

	privateDataFields map[t.QQID]struct{}
	scalarConstsMap   map[t.QID]*a.Const
	statusList        []status
	statusMap         map[t.QID]status
	structList        []*a.Struct
	structMap         map[t.QID]*a.Struct

	currFunk funk
	funks    map[t.QQID]funk

	numPublicCoroutines uint32
}

func (g *gen) generate() ([]byte, error) {
	b := new(buffer)

	g.statusMap = map[t.QID]status{}
	if err := g.forEachStatus(b, bothPubPri, (*gen).gatherStatuses); err != nil {
		return nil, err
	}
	for _, z := range builtin.Statuses {
		id, err := g.tm.Insert(z)
		if err != nil {
			return nil, err
		}
		msg, _ := t.Unescape(z)
		if msg == "" {
			return nil, fmt.Errorf("bad built-in status %q", z)
		}
		if err := g.addStatus(t.QID{t.IDBase, id}, msg, true); err != nil {
			return nil, err
		}
	}

	g.scalarConstsMap = map[t.QID]*a.Const{}
	if err := g.forEachConst(b, bothPubPri, (*gen).gatherScalarConsts); err != nil {
		return nil, err
	}

	// Make a topologically sorted list of structs.
	unsortedStructs := []*a.Struct(nil)
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() == a.KStruct {
				unsortedStructs = append(unsortedStructs, tld.AsStruct())
			}
		}
	}
	var ok bool
	g.structList, ok = a.TopologicalSortStructs(unsortedStructs)
	if !ok {
		return nil, fmt.Errorf("cyclical struct definitions")
	}
	g.structMap = map[t.QID]*a.Struct{}
	g.privateDataFields = map[t.QQID]struct{}{}
	for _, n := range g.structList {
		qid := n.QID()
		g.structMap[qid] = n
		for _, f := range n.Fields() {
			f := f.AsField()
			if f.PrivateData() {
				g.privateDataFields[t.QQID{qid[0], qid[1], f.Name()}] = struct{}{}
			}
		}
	}

	g.funks = map[t.QQID]funk{}
	if err := g.forEachFunc(nil, bothPubPri, (*gen).gatherFuncImpl); err != nil {
		return nil, err
	}

	includeGuard := "WUFFS_INCLUDE_GUARD__" + strings.ToUpper(g.pkgName)
	b.printf("#ifndef %s\n#define %s\n\n", includeGuard, includeGuard)

	if err := g.genIncludes(b); err != nil {
		return nil, err
	}

	b.writes("// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING ABOVE.\n\n")

	if err := g.genHeader(b); err != nil {
		return nil, err
	}
	b.writex(wiStartImpl)
	if err := g.genImpl(b); err != nil {
		return nil, err
	}
	b.writex(wiEnd)

	b.writes("// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING BELOW.\n\n")

	b.printf("#endif  // %s\n\n", includeGuard)
	return *b, nil
}

func (g *gen) genIncludes(b *buffer) error {
	b.writes("#if defined(WUFFS_IMPLEMENTATION) && !defined(WUFFS_CONFIG__MODULES)\n")
	b.writes("#define WUFFS_CONFIG__MODULES\n")
	b.printf("#define WUFFS_CONFIG__MODULE__%s\n", strings.ToUpper(g.pkgName))
	b.writes("#endif\n\n")

	usesList := []string(nil)
	usesMap := map[string]struct{}{}

	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KUse {
				continue
			}
			useDirname := g.tm.ByID(tld.AsUse().Path())
			useDirname, _ = t.Unescape(useDirname)

			// TODO: sanity check useDirname via commonflags.IsValidUsePath?

			if _, ok := usesMap[useDirname]; ok {
				continue
			}
			usesMap[useDirname] = struct{}{}
			usesList = append(usesList, useDirname)
		}
	}
	sort.Strings(usesList)

	b.writes("#include \"./wuffs-base.c\"\n")
	for _, use := range usesList {
		b.printf("#include \"./wuffs-%s.c\"\n",
			strings.Replace(use, "/", "-", -1))
	}

	b.writeb('\n')
	return nil
}

func (g *gen) genHeader(b *buffer) error {
	b.writes("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")

	b.writes("// ---------------- Status Codes\n\n")

	for _, z := range g.statusList {
		if !z.fromThisPkg || !z.public {
			continue
		}
		b.printf("extern const char* %s;\n", z.cName)
	}
	b.writes("\n")

	b.writes("// ---------------- Public Consts\n\n")
	if err := g.forEachConst(b, pubOnly, (*gen).writeConst); err != nil {
		return err
	}

	b.writes("// ---------------- Struct Declarations\n\n")
	for _, n := range g.structList {
		structName := n.QID().Str(g.tm)
		b.printf("typedef struct %s%s__struct %s%s;\n\n", g.pkgPrefix, structName, g.pkgPrefix, structName)
	}

	b.writes("// ---------------- Public Initializer Prototypes\n\n")
	b.writes("// For any given \"wuffs_foo__bar* self\", \"wuffs_foo__bar__initialize(self,\n")
	b.writes("// etc)\" should be called before any other \"wuffs_foo__bar__xxx(self, etc)\".\n")
	b.writes("//\n")
	b.writes("// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.\n")
	b.writes("// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.\n\n")
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

	b.writes("// ---------------- Struct Definitions\n\n")
	b.writes("// These structs' fields, and the sizeof them, are private implementation\n")
	b.writes("// details that aren't guaranteed to be stable across Wuffs versions.\n")
	b.writes("//\n")
	b.writes("// See https://en.wikipedia.org/wiki/Opaque_pointer#C\n\n")
	b.writes("#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)\n\n")

	for _, n := range g.structList {
		if err := g.writeStruct(b, n); err != nil {
			return err
		}
	}
	b.writes("#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)\n")

	b.writes("\n#ifdef __cplusplus\n}  // extern \"C\"\n#endif\n\n")
	return nil
}

func (g *gen) genImpl(b *buffer) error {
	module := "!defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__" +
		strings.ToUpper(g.pkgName) + ")"
	b.printf("#if %s\n\n", module)

	b.writes("// ---------------- Status Codes Implementations\n\n")

	for _, z := range g.statusList {
		if !z.fromThisPkg || z.msg == "" {
			continue
		}
		b.printf("const char* %s = \"%s%s: %s\";\n", z.cName, z.msg[:1], g.pkgName, z.msg[1:])
	}
	b.writes("\n")

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

	b.printf("#endif  // %s\n\n", module)
	return nil
}

func (g *gen) forEachConst(b *buffer, v visibility, f func(*gen, *buffer, *a.Const) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KConst ||
				(v == pubOnly && tld.AsRaw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.AsRaw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, b, tld.AsConst()); err != nil {
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
				(v == pubOnly && tld.AsRaw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.AsRaw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, b, tld.AsFunc()); err != nil {
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
				(v == pubOnly && tld.AsRaw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.AsRaw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, b, tld.AsStatus()); err != nil {
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
	if !ok || msg == "" {
		return fmt.Errorf("bad status message %q", raw)
	}
	return g.addStatus(n.QID(), msg, n.Public())
}

func (g *gen) addStatus(qid t.QID, msg string, public bool) error {
	category := "warning__"
	if msg[0] == '$' {
		category = "suspension__"
	} else if msg[0] == '#' {
		category = "error__"
	}
	z := status{
		cName:       g.packagePrefix(qid) + category + cName(msg, ""),
		msg:         msg,
		fromThisPkg: qid[0] == 0,
		public:      public,
	}
	g.statusList = append(g.statusList, z)
	g.statusMap[qid] = z
	return nil
}

func (g *gen) gatherScalarConsts(b *buffer, n *a.Const) error {
	if cv := n.Value().ConstValue(); cv != nil {
		g.scalarConstsMap[n.QID()] = n
	}
	return nil
}

func (g *gen) writeConst(b *buffer, n *a.Const) error {
	if cv := n.Value().ConstValue(); cv != nil {
		b.printf("#define %s %v\n\n", strings.ToUpper(g.pkgPrefix+n.QID()[1].Str(g.tm)), cv)
	} else {
		b.writes("static const ")
		if err := g.writeCTypeName(b, n.XType(), "//\n"+g.pkgPrefix, n.QID()[1].Str(g.tm)); err != nil {
			return err
		}
		b.writes("//\n WUFFS_BASE__POTENTIALLY_UNUSED = ")
		if err := g.writeConstList(b, n.Value()); err != nil {
			return err
		}
		b.writes(";\n\n")
	}
	return nil
}

func (g *gen) writeConstList(b *buffer, n *a.Expr) error {
	if n.Operator() == t.IDComma {
		b.writeb('{')
		for _, o := range n.Args() {
			if err := g.writeConstList(b, o.AsExpr()); err != nil {
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

func (g *gen) writeStructPrivateImpl(b *buffer, n *a.Struct) error {
	// TODO: allow max depth > 1 for recursive coroutines.
	const maxDepth = 1

	b.writes("// Do not access the private_impl's or private_data's fields directly. There\n")
	b.writes("// is no API/ABI compatibility or safety guarantee if you do so. Instead, use\n")
	b.writes("// the wuffs_foo__bar__baz functions.\n")
	b.writes("//\n")
	b.writes("// It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct\n")
	b.writes("// can be stack allocated when WUFFS_IMPLEMENTATION is defined.\n\n")

	b.writes("struct {\n")
	if n.Classy() {
		b.writes("uint32_t magic;\n")
		b.writes("uint32_t active_coroutine;\n")
		b.writes("\n")
	}

	for _, o := range n.Fields() {
		o := o.AsField()
		if o.PrivateData() || o.XType().Eq(typeExprUtility) {
			continue
		}
		if err := g.writeCTypeName(b, o.XType(), fPrefix, o.Name().Str(g.tm)); err != nil {
			return err
		}
		b.writes(";\n")
	}

	if n.Classy() {
		b.writeb('\n')
		for _, file := range g.files {
			for _, tld := range file.TopLevelDecls() {
				if tld.Kind() != a.KFunc {
					continue
				}
				o := tld.AsFunc()
				if o.Receiver() != n.QID() || !o.Effect().Coroutine() {
					continue
				}
				k := g.funks[o.QQID()]
				if k.coroSuspPoint == 0 {
					continue
				}

				b.printf("uint32_t %s%s[%d];\n", pPrefix, o.FuncName().Str(g.tm), maxDepth)
			}
		}

	}
	b.writes("} private_impl;\n\n")

	{
		oldOuterLenB0 := len(*b)
		b.writes("struct {\n")
		oldOuterLenB1 := len(*b)

		for _, o := range n.Fields() {
			o := o.AsField()
			if !o.PrivateData() || o.XType().Eq(typeExprUtility) {
				continue
			}
			if err := g.writeCTypeName(b, o.XType(), fPrefix, o.Name().Str(g.tm)); err != nil {
				return err
			}
			b.writes(";\n")
		}
		if oldOuterLenB1 != len(*b) {
			b.writeb('\n')
		}

		for _, file := range g.files {
			for _, tld := range file.TopLevelDecls() {
				if tld.Kind() != a.KFunc {
					continue
				}
				o := tld.AsFunc()
				if o.Receiver() != n.QID() || !o.Effect().Coroutine() {
					continue
				}
				k := g.funks[o.QQID()]
				if k.coroSuspPoint == 0 && !k.usesScratch {
					continue
				}

				oldInnerLenB0 := len(*b)
				b.writes("struct {\n")
				oldInnerLenB1 := len(*b)
				if k.coroSuspPoint != 0 {
					if err := g.writeVars(b, &k, true); err != nil {
						return err
					}
				}
				if k.usesScratch {
					b.writes("uint64_t scratch;\n")
				}
				if oldInnerLenB1 != len(*b) {
					b.printf("} %s%s[%d];\n", sPrefix, o.FuncName().Str(g.tm), maxDepth)
				} else {
					*b = (*b)[:oldInnerLenB0]
				}
			}
		}

		if oldOuterLenB1 != len(*b) {
			b.writes("} private_data;\n\n")
		} else {
			*b = (*b)[:oldOuterLenB0]
		}
	}

	return nil
}

func (g *gen) writeStruct(b *buffer, n *a.Struct) error {
	structName := n.QID().Str(g.tm)
	fullStructName := g.pkgPrefix + structName + "__struct"
	b.printf("struct %s {\n", fullStructName)

	b.writex(wiStart)
	if err := g.writeStructPrivateImpl(b, n); err != nil {
		return err
	}

	b.writex(wiElse)
	b.writes("// When WUFFS_IMPLEMENTATION is not defined, this placeholder private_impl is\n")
	b.writes("// large enough to discourage trying to allocate one on the stack. The sizeof\n")
	b.writes("// the real private_impl (and the sizeof the real outermost wuffs_foo__bar\n")
	b.writes("// struct) is not part of the public, stable, memory-safe API. Call\n")
	b.writes("// wuffs_foo__bar__baz methods (which all take a \"this\"-like pointer as their\n")
	b.writes("// first argument) instead of fiddling with bar.private_impl.qux fields.\n")
	b.writes("//\n")
	b.writes("// Even when WUFFS_IMPLEMENTATION is not defined, the outermost struct still\n")
	b.writes("// defines C++ convenience methods. These methods forward on \"this\", so that\n")
	b.writes("// you can write \"bar->baz(etc)\" instead of \"wuffs_foo__bar__baz(bar, etc)\".\n")
	b.writes("private:\n")
	b.writes("union {\n")
	b.writes("uint32_t align_as_per_magic_field;\n")
	b.writes("uint8_t placeholder[1073741824];  // 1 GiB.\n")
	b.writes("} private_impl WUFFS_BASE__POTENTIALLY_UNUSED_FIELD;\n\n")
	b.writes("public:\n")
	b.writex(wiEnd)

	if n.AsNode().AsRaw().Flags()&a.FlagsPublic != 0 {
		if err := g.writeCppMethods(b, n); err != nil {
			return err
		}
	}

	b.printf("};  // struct %s\n\n", fullStructName)
	return nil
}

func (g *gen) writeCppMethods(b *buffer, n *a.Struct) error {
	structName := n.QID().Str(g.tm)
	fullStructName := g.pkgPrefix + structName + "__struct"
	b.writes("#ifdef __cplusplus\n\n")

	// The empty // comment makes clang-format place the function name
	// at the start of a line.
	b.writes("inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT //\n" +
		"initialize(size_t sizeof_star_self, uint64_t wuffs_version, uint32_t initialize_flags) {\n")
	b.printf("return %s%s__initialize(this, sizeof_star_self, wuffs_version, initialize_flags);\n}\n\n",
		g.pkgPrefix, structName)

	structID := n.QID()[1]
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if (tld.Kind() != a.KFunc) || (tld.AsRaw().Flags()&a.FlagsPublic == 0) {
				continue
			}
			f := tld.AsFunc()
			qqid := f.QQID()
			if qqid[1] != structID {
				continue
			}

			if err := g.writeFuncSignature(b, f, cppInsideStruct); err != nil {
				return err
			}
			b.writes("{ return ")
			b.writes(g.funcCName(f))
			b.writes("(this")
			for _, o := range f.In().Fields() {
				b.writeb(',')
				b.writes(aPrefix)
				b.writes(o.AsField().Name().Str(g.tm))
			}
			b.writes(");}\n\n")
		}
	}

	b.writes("#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)\n")
	b.writes("// Disallow copy and assign.\n")
	b.printf("%s(const %s&) = delete;\n", fullStructName, fullStructName)
	b.printf("%s& operator=(const %s&) = delete;\n", fullStructName, fullStructName)
	b.writes("#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)\n\n")

	b.writes("#endif  // __cplusplus\n\n")
	return nil
}

var (
	wiStartImpl = []byte("\n// WUFFS C HEADER ENDS HERE.\n#ifdef WUFFS_IMPLEMENTATION\n\n")
	wiStart     = []byte("\n#ifdef WUFFS_IMPLEMENTATION\n\n")
	wiElse      = []byte("\n#else   // WUFFS_IMPLEMENTATION\n\n")
	wiEnd       = []byte("\n#endif  // WUFFS_IMPLEMENTATION\n\n")
)

func (g *gen) writeInitializerSignature(b *buffer, n *a.Struct, public bool) error {
	structName := n.QID().Str(g.tm)
	b.printf("wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT //\n"+
		"%s%s__initialize(%s%s *self, size_t sizeof_star_self, uint64_t wuffs_version, uint32_t initialize_flags)",
		g.pkgPrefix, structName, g.pkgPrefix, structName)
	return nil
}

func (g *gen) writeSizeofSignature(b *buffer, n *a.Struct) error {
	b.printf("size_t //\nsizeof__%s%s()", g.pkgPrefix, n.QID().Str(g.tm))
	return nil
}

func (g *gen) writeInitializerPrototype(b *buffer, n *a.Struct) error {
	if !n.Classy() {
		return nil
	}
	if err := g.writeInitializerSignature(b, n, n.Public()); err != nil {
		return err
	}
	b.writes(";\n\n")

	if n.Public() {
		if err := g.writeSizeofSignature(b, n); err != nil {
			return err
		}
		b.writes(";\n\n")
	}
	return nil
}

func (g *gen) writeInitializerImpl(b *buffer, n *a.Struct) error {
	if !n.Classy() {
		return nil
	}
	if err := g.writeInitializerSignature(b, n, false); err != nil {
		return err
	}
	b.writes("{\n")
	b.writes("if (!self) { return wuffs_base__error__bad_receiver; }\n")

	b.writes("if (sizeof(*self) != sizeof_star_self) {\n")
	b.writes("  return wuffs_base__error__bad_sizeof_receiver;\n")
	b.writes("}\n")
	b.writes("if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) || " +
		"(((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {\n")
	b.writes("  return wuffs_base__error__bad_wuffs_version;\n")
	b.writes("}\n\n")

	b.writes("if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {\n")
	b.writes("  // The whole point of this if-check is to detect an uninitialized *self.\n")
	b.writes("  // We disable the warning on GCC. Clang-5.0 does not have this warning.\n")
	b.writes("  #if !defined(__clang__) && defined(__GNUC__)\n")
	b.writes("  #pragma GCC diagnostic push\n")
	b.writes("  #pragma GCC diagnostic ignored \"-Wmaybe-uninitialized\"\n")
	b.writes("  #endif\n")
	b.writes("  if (self->private_impl.magic != 0) {\n")
	b.writes("    return wuffs_base__error__initialize_falsely_claimed_already_zeroed;\n")
	b.writes("  }\n")
	b.writes("  #if !defined(__clang__) && defined(__GNUC__)\n")
	b.writes("  #pragma GCC diagnostic pop\n")
	b.writes("  #endif\n")
	b.writes("} else {\n")
	b.writes("  if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {\n")
	b.writes("    memset(self, 0, sizeof(*self));\n")
	b.writes("    initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;\n")
	b.writes("  } else {\n")
	b.writes("    memset(&(self->private_impl), 0, sizeof(self->private_impl));\n")
	b.writes("  }\n")
	b.writes("}\n\n")

	// Call any ctors on sub-structs.
	for _, f := range n.Fields() {
		f := f.AsField()
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

		b.printf("{\n")
		b.printf("wuffs_base__status z = %s%s__initialize("+
			"&self->private_data.%s%s, sizeof(self->private_data.%s%s), WUFFS_VERSION, initialize_flags);\n",
			prefix, qid[1].Str(g.tm), fPrefix, f.Name().Str(g.tm), fPrefix, f.Name().Str(g.tm))
		b.printf("if (z) { return z; }\n")
		b.printf("}\n")
	}

	b.writes("self->private_impl.magic = WUFFS_BASE__MAGIC;\n")
	b.writes("return NULL;\n")
	b.writes("}\n\n")

	if n.Public() {
		if err := g.writeSizeofSignature(b, n); err != nil {
			return err
		}
		b.printf("{ return sizeof (%s%s); }\n\n", g.pkgPrefix, n.QID().Str(g.tm))
	}
	return nil
}
