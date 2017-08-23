// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

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
		if err := g.generate(); err != nil {
			return nil, err
		}
		stdout := &bytes.Buffer{}
		cmd := exec.Command("clang-format", "-style=Chromium")
		cmd.Stdin = &g.buffer
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

var builtInStatuses = [...]string{
	"status ok",
	"error bad puffs version",
	"error bad receiver",
	"error bad argument",
	"error constructor not called",
	"error closed for writes",
	"error unexpected EOF",  // Used if reading when closed == true.
	"suspension short read", // Used if reading when closed == false.
	"suspension short write",
	"suspension limited read",
	"suspension limited write",
}

func init() {
	// The +1 is for the error bit (the sign bit).
	if statusCodeNamespaceShift+base38.MaxBits+1 != 32 {
		panic("inconsistent status code namespace shift")
	}
	if len(builtInStatuses) > maxNamespacedStatusCode {
		panic("too many builtInStatuses")
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
	isError bool
}

type gen struct {
	buffer bytes.Buffer

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
	perFunc    perFunc
}

func (g *gen) printf(format string, args ...interface{}) { fmt.Fprintf(&g.buffer, format, args...) }
func (g *gen) writeb(b byte)                             { g.buffer.WriteByte(b) }
func (g *gen) writes(s string)                           { g.buffer.WriteString(s) }

func (g *gen) jumpTarget(n *a.While) (uint32, error) {
	if g.perFunc.jumpTargets == nil {
		g.perFunc.jumpTargets = map[*a.While]uint32{}
	}
	if jt, ok := g.perFunc.jumpTargets[n]; ok {
		return jt, nil
	}
	jt := uint32(len(g.perFunc.jumpTargets))
	if jt == 1000000 {
		return 0, fmt.Errorf("too many jump targets")
	}
	g.perFunc.jumpTargets[n] = jt
	return jt, nil
}

func (g *gen) generate() error {
	g.statusMap = map[t.ID]status{}
	if err := g.forEachStatus(bothPubPri, (*gen).gatherStatuses); err != nil {
		return err
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
		return fmt.Errorf("cyclical struct definitions")
	}
	g.structMap = map[t.ID]*a.Struct{}
	for _, n := range g.structList {
		g.structMap[n.Name()] = n
	}

	if err := g.genHeader(); err != nil {
		return err
	}
	g.writes("// C HEADER ENDS HERE.\n\n")
	return g.genImpl()
}

func (g *gen) genHeader() error {
	includeGuard := "PUFFS_" + strings.ToUpper(g.pkgName) + "_H"
	g.printf("#ifndef %s\n#define %s\n\n", includeGuard, includeGuard)

	g.printf("// Code generated by puffs-c. DO NOT EDIT.\n\n")
	g.writes(baseHeader)
	g.writes("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")

	g.writes("// ---------------- Status Codes\n\n")
	g.writes(statusCodeDescription)
	g.printf("//\n// Do not manipulate these bits directly. Use the API functions such as\n"+
		"// %sstatus_is_error instead.\n", g.pkgPrefix)
	g.printf("typedef int32_t %sstatus;\n\n", g.pkgPrefix)
	pkgID := g.checker.PackageID()
	g.printf("#define %spackageid %d // %#08x\n\n", g.pkgPrefix, pkgID, pkgID)

	for i, s := range builtInStatuses {
		code := uint32(0)
		if strings.HasPrefix(s, "error ") {
			code |= 1 << 31
		}
		code |= uint32(i)
		g.printf("#define %s %d // %#08x\n", strings.ToUpper(g.cName(s)), int32(code), code)
	}
	g.writes("\n")

	if len(g.statusList) > maxNamespacedStatusCode {
		return fmt.Errorf("too many status codes")
	}
	for i, s := range g.statusList {
		code := pkgID << statusCodeNamespaceShift
		if s.isError {
			code |= 1 << 31
		}
		code |= uint32(i)
		g.printf("#define %s %d // %#08x\n", s.name, int32(code), code)
	}
	g.writes("\n")

	g.printf("bool %sstatus_is_error(%sstatus s);\n\n", g.pkgPrefix, g.pkgPrefix)
	g.printf("const char* %sstatus_string(%sstatus s);\n\n", g.pkgPrefix, g.pkgPrefix)

	g.writes("// ---------------- Public Consts\n\n")
	if err := g.forEachConst(pubOnly, (*gen).writeConst); err != nil {
		return err
	}

	g.writes("// ---------------- Structs\n\n")
	for _, n := range g.structList {
		if err := g.writeStruct(n); err != nil {
			return err
		}
	}

	g.writes("// ---------------- Public Constructor and Destructor Prototypes\n\n")
	for _, n := range g.structList {
		if n.Public() {
			if err := g.writeCtorPrototype(n); err != nil {
				return err
			}
		}
	}

	g.writes("// ---------------- Public Function Prototypes\n\n")
	if err := g.forEachFunc(pubOnly, (*gen).writeFuncPrototype); err != nil {
		return err
	}

	g.writes("\n#ifdef __cplusplus\n}  // extern \"C\"\n#endif\n\n")
	g.printf("#endif  // %s\n\n", includeGuard)
	return nil
}

func (g *gen) genImpl() error {
	g.writes(baseImpl)
	g.writes("\n")

	g.writes("// ---------------- Status Codes Implementations\n\n")
	g.printf("bool %sstatus_is_error(%sstatus s) { return s < 0; }\n\n", g.pkgPrefix, g.pkgPrefix)

	g.printf("const char* %sstatus_strings0[%d] = {\n", g.pkgPrefix, len(builtInStatuses))
	for _, s := range builtInStatuses {
		if strings.HasPrefix(s, "status ") {
			s = s[len("status "):]
		} else if strings.HasPrefix(s, "suspension ") {
			s = s[len("suspension "):]
		} else if strings.HasPrefix(s, "error ") {
			s = s[len("error "):]
		}
		s = g.pkgName + ": " + s
		g.printf("%q,", s)
	}
	g.writes("};\n\n")

	g.printf("const char* %sstatus_strings1[%d] = {\n", g.pkgPrefix, len(g.statusList))
	for _, s := range g.statusList {
		g.printf("%q,", g.pkgName+": "+s.msg)
	}
	g.writes("};\n\n")

	g.printf("const char* %sstatus_string(%sstatus s) {\n", g.pkgPrefix, g.pkgPrefix)
	g.printf("const char** a = NULL;\n")
	g.printf("uint32_t n = 0;\n")
	g.printf("switch ((s >> %d) & %#x) {\n", statusCodeNamespaceShift, statusCodeNamespaceMask)
	g.printf("case 0: a = %sstatus_strings0; n = %d; break;\n",
		g.pkgPrefix, len(builtInStatuses))
	g.printf("case %spackageid: a = %sstatus_strings1; n = %d; break;\n",
		g.pkgPrefix, g.pkgPrefix, len(g.statusList))
	// TODO: add cases for other packages used by this one.
	g.printf("}\n")
	g.printf("uint32_t i = s & %#0x;\n", 1<<statusCodeCodeBits-1)
	g.printf("return i < n ? a[i] : \"%s: unknown status\";\n", g.pkgName)
	g.writes("}\n\n")

	g.writes("// ---------------- Private Consts\n\n")
	if err := g.forEachConst(priOnly, (*gen).writeConst); err != nil {
		return err
	}

	g.writes("// ---------------- Private Constructor and Destructor Prototypes\n\n")
	for _, n := range g.structList {
		if !n.Public() {
			if err := g.writeCtorPrototype(n); err != nil {
				return err
			}
		}
	}

	g.writes("// ---------------- Private Function Prototypes\n\n")
	if err := g.forEachFunc(priOnly, (*gen).writeFuncPrototype); err != nil {
		return err
	}

	g.writes("// ---------------- Constructor and Destructor Implementations\n\n")
	g.writes("// PUFFS_MAGIC is a magic number to check that constructors are called. It's\n")
	g.writes("// not foolproof, given C doesn't automatically zero memory before use, but it\n")
	g.writes("// should catch 99.99% of cases.\n")
	g.writes("//\n")
	g.writes("// Its (non-zero) value is arbitrary, based on md5sum(\"puffs\").\n")
	g.writes("#define PUFFS_MAGIC (0xCB3699CCU)\n\n")
	g.writes("// PUFFS_ALREADY_ZEROED is passed from a container struct's constructor to a\n")
	g.writes("// containee struct's constructor when the container has already zeroed the\n")
	g.writes("// containee's memory.\n")
	g.writes("//\n")
	g.writes("// Its (non-zero) value is arbitrary, based on md5sum(\"zeroed\").\n")
	g.writes("#define PUFFS_ALREADY_ZEROED (0x68602EF1U)\n\n")
	for _, n := range g.structList {
		if err := g.writeCtorImpl(n); err != nil {
			return err
		}
	}

	g.writes("// ---------------- Function Implementations\n\n")
	if err := g.forEachFunc(bothPubPri, (*gen).writeFuncImpl); err != nil {
		return err
	}

	return nil
}

func (g *gen) forEachConst(v visibility, f func(*gen, *a.Const) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KConst ||
				(v == pubOnly && tld.Raw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.Raw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, tld.Const()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) forEachFunc(v visibility, f func(*gen, *a.Func) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KFunc ||
				(v == pubOnly && tld.Raw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.Raw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, tld.Func()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) forEachStatus(v visibility, f func(*gen, *a.Status) error) error {
	for _, file := range g.files {
		for _, tld := range file.TopLevelDecls() {
			if tld.Kind() != a.KStatus ||
				(v == pubOnly && tld.Raw().Flags()&a.FlagsPublic == 0) ||
				(v == priOnly && tld.Raw().Flags()&a.FlagsPublic != 0) {
				continue
			}
			if err := f(g, tld.Status()); err != nil {
				return err
			}
		}
	}
	return nil
}

func (g *gen) cName(name string) string {
	b := []byte(nil)
	b = append(b, "puffs_"...)
	b = append(b, g.pkgName...)
	b = append(b, '_')
	underscore := true
	for _, r := range name {
		if 'A' <= r && r <= 'Z' {
			b = append(b, byte(r+'a'-'A'))
			underscore = false
		} else if ('a' <= r && r <= 'z') || ('0' <= r && r <= '9') {
			b = append(b, byte(r))
			underscore = false
		} else if !underscore {
			b = append(b, '_')
			underscore = true
		}
	}
	if underscore {
		b = b[:len(b)-1]
	}
	return string(b)
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

func (g *gen) gatherStatuses(n *a.Status) error {
	raw := n.Message().String(g.tm)
	msg, ok := t.Unescape(raw)
	if !ok {
		return fmt.Errorf("bad status message %q", raw)
	}
	prefix := "suspension "
	isError := n.Keyword().Key() == t.KeyError
	if isError {
		prefix = "error "
	}
	s := status{
		name:    strings.ToUpper(g.cName(prefix + msg)),
		msg:     msg,
		isError: isError,
	}
	g.statusList = append(g.statusList, s)
	g.statusMap[n.Message()] = s
	return nil
}

func (g *gen) writeConst(n *a.Const) error {
	if !n.Public() {
		g.writes("static ")
	}
	g.writes("const ")
	if err := g.writeCTypeName(n.XType(), g.pkgPrefix, n.Name().String(g.tm)); err != nil {
		return err
	}
	g.writes(" = ")
	if err := g.writeConstList(n.Value()); err != nil {
		return err
	}
	g.writes(";\n\n")
	return nil
}

func (g *gen) writeConstList(n *a.Expr) error {
	switch n.ID0().Key() {
	case 0:
		g.writes(n.ConstValue().String())
	case t.KeyDollar:
		g.writeb('{')
		for _, o := range n.Args() {
			if err := g.writeConstList(o.Expr()); err != nil {
				return err
			}
			g.writeb(',')
		}
		g.writeb('}')
	default:
		return fmt.Errorf("invalid const value %q", n.String(g.tm))
	}
	return nil
}

func (g *gen) writeStruct(n *a.Struct) error {
	// For API/ABI compatibility, the very first field in the struct's
	// private_impl must be the status code. This lets the constructor callee
	// set "self->private_impl.status = etc_error_bad_puffs_version;"
	// regardless of the sizeof(*self) struct reserved by the caller and even
	// if the caller and callee were built with different versions.
	structName := n.Name().String(g.tm)
	g.writes("typedef struct {\n")
	g.writes("// Do not access the private_impl's fields directly. There is no API/ABI\n")
	g.writes("// compatibility or safety guarantee if you do so. Instead, use the\n")
	g.printf("// %s%s_etc functions.\n", g.pkgPrefix, structName)
	g.writes("//\n")
	g.writes("// In C++, these fields would be \"private\", but C does not support that.\n")
	g.writes("//\n")
	g.writes("// It is a struct, not a struct*, so that it can be stack allocated.\n")
	g.writes("struct {\n")
	if n.Suspendible() {
		g.printf("%sstatus status;\n", g.pkgPrefix)
		g.writes("uint32_t magic;\n")
		g.writes("uint64_t scratch;\n")
		g.writes("\n")
	}

	for _, o := range n.Fields() {
		o := o.Field()
		if err := g.writeCTypeName(o.XType(), fPrefix, o.Name().String(g.tm)); err != nil {
			return err
		}
		g.writes(";\n")
	}

	if n.Suspendible() {
		g.writeb('\n')
		for _, file := range g.files {
			for _, tld := range file.TopLevelDecls() {
				if tld.Kind() != a.KFunc {
					continue
				}
				o := tld.Func()
				if o.Receiver() != n.Name() || !o.Suspendible() {
					continue
				}
				// TODO: allow max depth > 1 for recursive coroutines.
				const maxDepth = 1
				g.writes("struct {\n")
				g.writes("uint32_t coro_susp_point;\n")
				if err := g.writeVars(o.Body()); err != nil {
					return err
				}
				g.printf("} %s%s[%d];\n", cPrefix, o.Name().String(g.tm), maxDepth)
			}
		}
	}

	g.printf("} private_impl;\n } %s%s;\n\n", g.pkgPrefix, structName)
	return nil
}

func (g *gen) writeCtorSignature(n *a.Struct, public bool, ctor bool) error {
	structName := n.Name().String(g.tm)
	ctorName := "destructor"
	if ctor {
		ctorName = "constructor"
		if public {
			g.printf("// %s%s_%s is a constructor function.\n", g.pkgPrefix, structName, ctorName)
			g.printf("//\n")
			g.printf("// It should be called before any other %s%s_* function.\n",
				g.pkgPrefix, structName)
			g.printf("//\n")
			g.printf("// Pass PUFFS_VERSION and 0 for puffs_version and for_internal_use_only.\n")
		}
	}
	g.printf("void %s%s_%s(%s%s *self", g.pkgPrefix, structName, ctorName, g.pkgPrefix, structName)
	if ctor {
		g.printf(", uint32_t puffs_version, uint32_t for_internal_use_only")
	}
	g.printf(")")
	return nil
}

func (g *gen) writeCtorPrototype(n *a.Struct) error {
	if !n.Suspendible() {
		return nil
	}
	for _, ctor := range []bool{true, false} {
		if err := g.writeCtorSignature(n, n.Public(), ctor); err != nil {
			return err
		}
		g.writes(";\n\n")
	}
	return nil
}

func (g *gen) writeCtorImpl(n *a.Struct) error {
	if !n.Suspendible() {
		return nil
	}
	for _, ctor := range []bool{true, false} {
		if err := g.writeCtorSignature(n, false, ctor); err != nil {
			return err
		}
		g.printf("{\n")
		g.printf("if (!self) { return; }\n")

		if ctor {
			g.printf("if (puffs_version != PUFFS_VERSION) {\n")
			g.printf("self->private_impl.status = %sERROR_BAD_PUFFS_VERSION;\n", g.PKGPREFIX)
			g.printf("return;\n")
			g.printf("}\n")

			g.writes("if (for_internal_use_only != PUFFS_ALREADY_ZEROED) {" +
				"memset(self, 0, sizeof(*self)); }\n")
			g.writes("self->private_impl.magic = PUFFS_MAGIC;\n")

			for _, f := range n.Fields() {
				f := f.Field()
				if dv := f.DefaultValue(); dv != nil {
					// TODO: set default values for array types.
					g.printf("self->private_impl.%s%s = %d;\n", fPrefix, f.Name().String(g.tm), dv.ConstValue())
				}
			}
		}

		// Call any ctor/dtors on sub-structs.
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
			if ctor {
				g.printf("%s%s_constructor(&self->private_impl.%s%s,"+
					"PUFFS_VERSION, PUFFS_ALREADY_ZEROED);\n",
					g.pkgPrefix, x.Name().String(g.tm), fPrefix, f.Name().String(g.tm))
			} else {
				g.printf("%s%s_destructor(&self->private_impl.%s%s);\n",
					g.pkgPrefix, x.Name().String(g.tm), fPrefix, f.Name().String(g.tm))
			}
		}

		g.writes("}\n\n")
	}
	return nil
}

func (g *gen) writeExprUnaryOp(n *a.Expr, rp replacementPolicy, depth uint32) error {
	// TODO.
	return nil
}

func (g *gen) writeExprBinaryOp(n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	op := n.ID0()
	if op.Key() == t.KeyXBinaryAs {
		return g.writeExprAs(n.LHS().Expr(), n.RHS().TypeExpr(), rp, depth)
	}
	if pp == parenthesesMandatory {
		g.writeb('(')
	}
	if err := g.writeExpr(n.LHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
		return err
	}
	// TODO: does KeyXBinaryAmpHat need special consideration?
	g.writes(cOpNames[0xFF&op.Key()])
	if err := g.writeExpr(n.RHS().Expr(), rp, parenthesesMandatory, depth); err != nil {
		return err
	}
	if pp == parenthesesMandatory {
		g.writeb(')')
	}
	return nil
}

func (g *gen) writeExprAs(lhs *a.Expr, rhs *a.TypeExpr, rp replacementPolicy, depth uint32) error {
	g.writes("((")
	// TODO: watch for passing an array type to writeCTypeName? In C, an array
	// type can decay into a pointer.
	if err := g.writeCTypeName(rhs, "", ""); err != nil {
		return err
	}
	g.writes(")(")
	if err := g.writeExpr(lhs, rp, parenthesesMandatory, depth); err != nil {
		return err
	}
	g.writes("))")
	return nil
}

func (g *gen) writeExprAssociativeOp(n *a.Expr, rp replacementPolicy, pp parenthesesPolicy, depth uint32) error {
	if pp == parenthesesMandatory {
		g.writeb('(')
	}
	opName := cOpNames[0xFF&n.ID0().Key()]
	for i, o := range n.Args() {
		if i != 0 {
			g.writes(opName)
		}
		if err := g.writeExpr(o.Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
		}
	}
	if pp == parenthesesMandatory {
		g.writeb(')')
	}
	return nil
}

func (g *gen) writeCTypeName(n *a.TypeExpr, varNamePrefix string, varName string) error {
	// It may help to refer to http://unixwiz.net/techtips/reading-cdecl.html

	// maxNumPointers is an arbitrary implementation restriction.
	const maxNumPointers = 16

	x := n
	for ; x != nil && x.Decorator().Key() == t.KeyOpenBracket; x = x.Inner() {
	}

	numPointers, innermost := 0, x
	for ; innermost != nil && innermost.Inner() != nil; innermost = innermost.Inner() {
		// TODO: "nptr T", not just "ptr T".
		if p := innermost.Decorator().Key(); p == t.KeyPtr {
			if numPointers == maxNumPointers {
				return fmt.Errorf("cannot convert Puffs type %q to C: too many ptr's", n.String(g.tm))
			}
			numPointers++
			continue
		}
		// TODO: fix this.
		return fmt.Errorf("cannot convert Puffs type %q to C", n.String(g.tm))
	}

	fallback := true
	if k := innermost.Name().Key(); k < t.Key(len(cTypeNames)) {
		if s := cTypeNames[k]; s != "" {
			g.writes(s)
			fallback = false
		}
	}
	if fallback {
		g.printf("%s%s", g.pkgPrefix, innermost.Name().String(g.tm))
	}

	for i := 0; i < numPointers; i++ {
		g.writeb('*')
	}

	g.writeb(' ')
	g.writes(varNamePrefix)
	g.writes(varName)

	x = n
	for ; x != nil && x.Decorator().Key() == t.KeyOpenBracket; x = x.Inner() {
		g.writeb('[')
		g.writes(x.ArrayLength().ConstValue().String())
		g.writeb(']')
	}

	return nil
}

var numTypeBounds = [256][2]*big.Int{
	t.KeyI8:    {big.NewInt(-1 << 7), big.NewInt(1<<7 - 1)},
	t.KeyI16:   {big.NewInt(-1 << 15), big.NewInt(1<<15 - 1)},
	t.KeyI32:   {big.NewInt(-1 << 31), big.NewInt(1<<31 - 1)},
	t.KeyI64:   {big.NewInt(-1 << 63), big.NewInt(1<<63 - 1)},
	t.KeyU8:    {zero, big.NewInt(0).SetUint64(1<<8 - 1)},
	t.KeyU16:   {zero, big.NewInt(0).SetUint64(1<<16 - 1)},
	t.KeyU32:   {zero, big.NewInt(0).SetUint64(1<<32 - 1)},
	t.KeyU64:   {zero, big.NewInt(0).SetUint64(1<<64 - 1)},
	t.KeyUsize: {zero, zero},
	t.KeyBool:  {zero, one},
}

var cTypeNames = [...]string{
	t.KeyI8:      "int8_t",
	t.KeyI16:     "int16_t",
	t.KeyI32:     "int32_t",
	t.KeyI64:     "int64_t",
	t.KeyU8:      "uint8_t",
	t.KeyU16:     "uint16_t",
	t.KeyU32:     "uint32_t",
	t.KeyU64:     "uint64_t",
	t.KeyUsize:   "size_t",
	t.KeyBool:    "bool",
	t.KeyBuf1:    "puffs_base_buf1",
	t.KeyBuf2:    "puffs_base_buf2",
	t.KeyReader1: "puffs_base_reader1",
	t.KeyWriter1: "puffs_base_writer1",
}

var cOpNames = [256]string{
	t.KeyEq:        " = ",
	t.KeyPlusEq:    " += ",
	t.KeyMinusEq:   " -= ",
	t.KeyStarEq:    " *= ",
	t.KeySlashEq:   " /= ",
	t.KeyShiftLEq:  " <<= ",
	t.KeyShiftREq:  " >>= ",
	t.KeyAmpEq:     " &= ",
	t.KeyAmpHatEq:  " no_such_amp_hat_C_operator ",
	t.KeyPipeEq:    " |= ",
	t.KeyHatEq:     " ^= ",
	t.KeyPercentEq: " %= ",

	t.KeyXUnaryPlus:  "+",
	t.KeyXUnaryMinus: "-",
	t.KeyXUnaryNot:   "!",

	t.KeyXBinaryPlus:        " + ",
	t.KeyXBinaryMinus:       " - ",
	t.KeyXBinaryStar:        " * ",
	t.KeyXBinarySlash:       " / ",
	t.KeyXBinaryShiftL:      " << ",
	t.KeyXBinaryShiftR:      " >> ",
	t.KeyXBinaryAmp:         " & ",
	t.KeyXBinaryAmpHat:      " no_such_amp_hat_C_operator ",
	t.KeyXBinaryPipe:        " | ",
	t.KeyXBinaryHat:         " ^ ",
	t.KeyXBinaryPercent:     " % ",
	t.KeyXBinaryNotEq:       " != ",
	t.KeyXBinaryLessThan:    " < ",
	t.KeyXBinaryLessEq:      " <= ",
	t.KeyXBinaryEqEq:        " == ",
	t.KeyXBinaryGreaterEq:   " >= ",
	t.KeyXBinaryGreaterThan: " > ",
	t.KeyXBinaryAnd:         " && ",
	t.KeyXBinaryOr:          " || ",
	t.KeyXBinaryAs:          " no_such_as_C_operator ",

	t.KeyXAssociativePlus: " + ",
	t.KeyXAssociativeStar: " * ",
	t.KeyXAssociativeAmp:  " & ",
	t.KeyXAssociativePipe: " | ",
	t.KeyXAssociativeHat:  " ^ ",
	t.KeyXAssociativeAnd:  " && ",
	t.KeyXAssociativeOr:   " || ",
}
