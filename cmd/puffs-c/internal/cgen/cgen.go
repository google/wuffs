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

//go:generate go run gen.go

package cgen

import (
	"bytes"
	"fmt"
	"math/big"
	"os"
	"os/exec"
	"strings"

	"github.com/google/puffs/lang/base38"
	"github.com/google/puffs/lang/builtin"
	"github.com/google/puffs/lang/check"
	"github.com/google/puffs/lang/generate"

	a "github.com/google/puffs/lang/ast"
	t "github.com/google/puffs/lang/token"
)

var (
	zero = big.NewInt(0)
	one  = big.NewInt(1)
)

// Prefixes are prepended to names to form a namespace and to avoid e.g.
// "double" being a valid Puffs variable name but not a valid C one.
const (
	aPrefix = "a_" // Function argument.
	bPrefix = "b_" // Derived from a function argument.
	cPrefix = "c_" // Coroutine state.
	fPrefix = "f_" // Struct field.
	lPrefix = "l_" // Limit length.
	tPrefix = "t_" // Temporary local variable.
	vPrefix = "v_" // Local variable.
)

// Do transpiles a Puffs program to a C program.
//
// The arguments list the source Puffs files. If no arguments are given, it
// reads from stdin.
//
// The generated program is written to stdout.
func Do(args []string) error {
	return generate.Do(args, func(pkgName string, tm *t.Map, c *check.Checker, files []*a.File) ([]byte, error) {
		g := &gen{
			PKGPREFIX: "PUFFS_" + strings.ToUpper(pkgName) + "_",
			pkgPrefix: "puffs_" + pkgName + "_",
			pkgName:   pkgName,
			tm:        tm,
			checker:   c,
			files:     files,
		}
		unformatted, err := g.generate()
		if err != nil {
			return nil, err
		}
		stdout := &bytes.Buffer{}
		cmd := exec.Command("clang-format", "-style=Chromium")
		cmd.Stdin = bytes.NewReader(unformatted)
		cmd.Stdout = stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err != nil {
			return nil, err
		}
		return stdout.Bytes(), nil
	})
}

const (
	maxNamespacedStatusCode  = 255
	statusCodeNamespaceMask  = 1<<base38.MaxBits - 1
	statusCodeNamespaceShift = 10
	statusCodeCodeBits       = 8
	statusCodeDescription    = "" +
		"// Status codes are int32_t values:\n" +
		"//  - the sign bit indicates a non-recoverable status code: an error\n" +
		"//  - bits 10-30 hold the packageid: a namespace\n" +
		"//  - bits 8-9 are reserved\n" +
		"//  - bits 0-7 are a package-namespaced numeric code\n"
)

func init() {
	// The +1 is for the error bit (the sign bit).
	if statusCodeNamespaceShift+base38.MaxBits+1 != 32 {
		panic("inconsistent status code namespace shift")
	}
	if len(builtin.StatusList) > maxNamespacedStatusCode {
		panic("too many built-in statuses")
	}
}

type replacementPolicy bool

const (
	replaceNothing          = replacementPolicy(false)
	replaceCallSuspendibles = replacementPolicy(true)
)

// parenthesesPolicy controls whether to print the outer parentheses in an
// expression like "(x + y)". An "if" or "while" will print their own
// parentheses for "if (expr)" because they need to be able to say "if (x)".
// But a double-parenthesized expression like "if ((x == y))" is a clang
// warning (-Wparentheses-equality) and we like to compile with -Wall -Werror.
type parenthesesPolicy bool

const (
	parenthesesMandatory = parenthesesPolicy(false)
	parenthesesOptional  = parenthesesPolicy(true)
)

type visibility uint32

const (
	bothPubPri = visibility(iota)
	pubOnly
	priOnly
)

const maxTemp = 10000

type status struct {
	name    string
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

type gen struct {
	PKGPREFIX string // e.g. "PUFFS_JPEG_"
	pkgPrefix string // e.g. "puffs_jpeg_"
	pkgName   string // e.g. "jpeg"

	tm         *t.Map
	checker    *check.Checker
	files      []*a.File
	statusList []status
	statusMap  map[t.ID]status
	structList []*a.Struct
	structMap  map[t.ID]*a.Struct
	currFunk   funk
	funks      map[t.QID]funk
}

func (g *gen) generate() ([]byte, error) {
	b := new(buffer)

	g.statusMap = map[t.ID]status{}
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
	g.structMap = map[t.ID]*a.Struct{}
	for _, n := range g.structList {
		g.structMap[n.Name()] = n
	}

	g.funks = map[t.QID]funk{}
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
	includeGuard := "PUFFS_" + strings.ToUpper(g.pkgName) + "_H"
	b.printf("#ifndef %s\n#define %s\n\n", includeGuard, includeGuard)

	b.printf("// Code generated by puffs-c. DO NOT EDIT.\n\n")
	b.writes(baseHeader)
	b.writes("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")

	b.writes("// ---------------- Status Codes\n\n")
	b.writes(statusCodeDescription)
	b.printf("//\n// Do not manipulate these bits directly. Use the API functions such as\n"+
		"// %sstatus_is_error instead.\n", g.pkgPrefix)
	b.printf("typedef int32_t %sstatus;\n\n", g.pkgPrefix)
	pkgID := g.checker.PackageID()
	b.printf("#define %spackageid %d // %#08x\n\n", g.pkgPrefix, pkgID, pkgID)

	for i, z := range builtin.StatusList {
		code := uint32(0)
		if z.Keyword == t.IDError {
			code |= 1 << 31
		}
		code |= uint32(i)
		b.printf("#define %s %d // %#08x\n", strings.ToUpper(g.cName(z.String())), int32(code), code)
	}
	b.writes("\n")

	if len(g.statusList) > maxNamespacedStatusCode {
		return fmt.Errorf("too many status codes")
	}
	for i, s := range g.statusList {
		code := pkgID << statusCodeNamespaceShift
		if s.keyword.Key() == t.KeyError {
			code |= 1 << 31
		}
		code |= uint32(i)
		b.printf("#define %s %d // %#08x\n", s.name, int32(code), code)
	}
	b.writes("\n")

	b.printf("bool %sstatus_is_error(%sstatus s);\n\n", g.pkgPrefix, g.pkgPrefix)
	b.printf("const char* %sstatus_string(%sstatus s);\n\n", g.pkgPrefix, g.pkgPrefix)

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
	b.writes(baseImpl)
	b.writes("\n")

	b.writes("// ---------------- Status Codes Implementations\n\n")
	b.printf("bool %sstatus_is_error(%sstatus s) { return s < 0; }\n\n", g.pkgPrefix, g.pkgPrefix)

	b.printf("const char* %sstatus_strings0[%d] = {\n", g.pkgPrefix, len(builtin.StatusList))
	for _, z := range builtin.StatusList {
		b.printf("%q,", g.pkgName+": "+z.Message)
	}
	b.writes("};\n\n")

	b.printf("const char* %sstatus_strings1[%d] = {\n", g.pkgPrefix, len(g.statusList))
	for _, s := range g.statusList {
		b.printf("%q,", g.pkgName+": "+s.msg)
	}
	b.writes("};\n\n")

	b.printf("const char* %sstatus_string(%sstatus s) {\n", g.pkgPrefix, g.pkgPrefix)
	b.printf("const char** a = NULL;\n")
	b.printf("uint32_t n = 0;\n")
	b.printf("switch ((s >> %d) & %#x) {\n", statusCodeNamespaceShift, statusCodeNamespaceMask)
	b.printf("case 0: a = %sstatus_strings0; n = %d; break;\n", g.pkgPrefix, len(builtin.StatusList))
	b.printf("case %spackageid: a = %sstatus_strings1; n = %d; break;\n",
		g.pkgPrefix, g.pkgPrefix, len(g.statusList))
	// TODO: add cases for other packages used by this one.
	b.printf("}\n")
	b.printf("uint32_t i = s & %#0x;\n", 1<<statusCodeCodeBits-1)
	b.printf("return i < n ? a[i] : \"%s: unknown status\";\n", g.pkgName)
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
	b.writes("// PUFFS_MAGIC is a magic number to check that initializers are called. It's\n")
	b.writes("// not foolproof, given C doesn't automatically zero memory before use, but it\n")
	b.writes("// should catch 99.99% of cases.\n")
	b.writes("//\n")
	b.writes("// Its (non-zero) value is arbitrary, based on md5sum(\"puffs\").\n")
	b.writes("#define PUFFS_MAGIC (0xCB3699CCU)\n\n")
	b.writes("// PUFFS_ALREADY_ZEROED is passed from a container struct's initializer to a\n")
	b.writes("// containee struct's initializer when the container has already zeroed the\n")
	b.writes("// containee's memory.\n")
	b.writes("//\n")
	b.writes("// Its (non-zero) value is arbitrary, based on md5sum(\"zeroed\").\n")
	b.writes("#define PUFFS_ALREADY_ZEROED (0x68602EF1U)\n\n")
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

func (g *gen) cName(name string) string {
	s := []byte(nil)
	s = append(s, "puffs_"...)
	s = append(s, g.pkgName...)
	s = append(s, '_')
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

func (g *gen) sizeof(typ *a.TypeExpr) (uint32, error) {
	if typ.Decorator() == 0 {
		switch typ.Name().Key() {
		case t.KeyU8:
			return 1, nil
		case t.KeyU16:
			return 2, nil
		case t.KeyU32:
			return 4, nil
		case t.KeyU64:
			return 8, nil
		}
	}
	return 0, fmt.Errorf("unknown sizeof for %q", typ.String(g.tm))
}

func (g *gen) gatherStatuses(b *buffer, n *a.Status) error {
	raw := n.Message().String(g.tm)
	msg, ok := t.Unescape(raw)
	if !ok {
		return fmt.Errorf("bad status message %q", raw)
	}
	prefix := "SUSPENSION_"
	if n.Keyword().Key() == t.KeyError {
		prefix = "ERROR_"
	}
	s := status{
		name:    strings.ToUpper(g.cName(prefix + msg)),
		msg:     msg,
		keyword: n.Keyword(),
	}
	g.statusList = append(g.statusList, s)
	g.statusMap[n.Message()] = s
	return nil
}

func (g *gen) writeConst(b *buffer, n *a.Const) error {
	if !n.Public() {
		b.writes("static ")
	}
	b.writes("const ")
	if err := g.writeCTypeName(b, n.XType(), g.pkgPrefix, n.Name().String(g.tm)); err != nil {
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
	switch n.ID0().Key() {
	case 0:
		b.writes(n.ConstValue().String())
	case t.KeyDollar:
		b.writeb('{')
		for _, o := range n.Args() {
			if err := g.writeConstList(b, o.Expr()); err != nil {
				return err
			}
			b.writeb(',')
		}
		b.writeb('}')
	default:
		return fmt.Errorf("invalid const value %q", n.String(g.tm))
	}
	return nil
}

func (g *gen) writeStruct(b *buffer, n *a.Struct) error {
	// For API/ABI compatibility, the very first field in the struct's
	// private_impl must be the status code. This lets the initializer callee
	// set "self->private_impl.status = etc_error_bad_puffs_version;"
	// regardless of the sizeof(*self) struct reserved by the caller and even
	// if the caller and callee were built with different versions.
	structName := n.Name().String(g.tm)
	b.writes("typedef struct {\n")
	b.writes("// Do not access the private_impl's fields directly. There is no API/ABI\n")
	b.writes("// compatibility or safety guarantee if you do so. Instead, use the\n")
	b.printf("// %s%s_etc functions.\n", g.pkgPrefix, structName)
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
		if err := g.writeCTypeName(b, o.XType(), fPrefix, o.Name().String(g.tm)); err != nil {
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
				if o.Receiver() != n.Name() || !o.Suspendible() {
					continue
				}
				k := g.funks[o.QID()]
				if k.coroSuspPoint == 0 && !k.usesScratch {
					continue
				}
				// TODO: allow max depth > 1 for recursive coroutines.
				const maxDepth = 1
				b.writes("struct {\n")
				if k.coroSuspPoint != 0 {
					b.writes("uint32_t coro_susp_point;\n")
					if err := g.writeVars(b, o.Body(), true); err != nil {
						return err
					}
				}
				if k.usesScratch {
					b.writes("uint64_t scratch;\n")
				}
				b.printf("} %s%s[%d];\n", cPrefix, o.Name().String(g.tm), maxDepth)
			}
		}
	}

	b.printf("} private_impl;\n } %s%s;\n\n", g.pkgPrefix, structName)
	return nil
}

func (g *gen) writeInitializerSignature(b *buffer, n *a.Struct, public bool) error {
	structName := n.Name().String(g.tm)
	if public {
		b.printf("// %s%s_initialize is an initializer function.\n", g.pkgPrefix, structName)
		b.printf("//\n")
		b.printf("// It should be called before any other %s%s_* function.\n",
			g.pkgPrefix, structName)
		b.printf("//\n")
		b.printf("// Pass PUFFS_VERSION and 0 for puffs_version and for_internal_use_only.\n")
	}
	b.printf("void %s%s_initialize(%s%s *self", g.pkgPrefix, structName, g.pkgPrefix, structName)
	b.printf(", uint32_t puffs_version, uint32_t for_internal_use_only")
	b.printf(")")
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

	b.printf("if (puffs_version != PUFFS_VERSION) {\n")
	b.printf("self->private_impl.status = %sERROR_BAD_PUFFS_VERSION;\n", g.PKGPREFIX)
	b.printf("return;\n")
	b.printf("}\n")

	b.writes("if (for_internal_use_only != PUFFS_ALREADY_ZEROED) {" +
		"memset(self, 0, sizeof(*self)); }\n")
	b.writes("self->private_impl.magic = PUFFS_MAGIC;\n")

	for _, f := range n.Fields() {
		f := f.Field()
		if dv := f.DefaultValue(); dv != nil {
			// TODO: set default values for array types.
			b.printf("self->private_impl.%s%s = %d;\n", fPrefix, f.Name().String(g.tm), dv.ConstValue())
		}
	}

	// Call any ctors on sub-structs.
	for _, f := range n.Fields() {
		f := f.Field()
		x := f.XType()
		if x != x.Innermost() {
			// TODO: arrays of sub-structs.
			continue
		}
		if g.structMap[x.Name()] == nil {
			continue
		}
		b.printf("%s%s_initialize(&self->private_impl.%s%s,"+
			"PUFFS_VERSION, PUFFS_ALREADY_ZEROED);\n",
			g.pkgPrefix, x.Name().String(g.tm), fPrefix, f.Name().String(g.tm))
	}

	b.writes("}\n\n")
	return nil
}
