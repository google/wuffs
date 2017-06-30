// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

//go:generate go run gen.go

// puffs-gen-c transpiles a Puffs program to a C program.
//
// The command line arguments list the source Puffs files. If no arguments are
// given, it reads from stdin.
//
// The generated program is written to stdout.
package main

import (
	"bytes"
	"fmt"
	"math/big"
	"os"
	"os/exec"
	"strings"

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

func main() {
	generate.Main(func(pkgName string, tm *t.Map, files []*a.File) ([]byte, error) {
		g := &gen{
			pkgName: pkgName,
			tm:      tm,
			files:   files,
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

const userDefinedStatusBase = 128

var builtInStatuses = [...]string{
	// For API/ABI forwards and backwards compatibility, the very first two
	// statuses must be "status ok" (with generated value 0) and "error bad
	// version" (with generated value -2 + 1). This lets caller code check the
	// constructor return value for "error bad version" even if the caller and
	// callee were built with different versions.
	"status ok",
	"error bad version",
	// The order of the remaining statuses is less important, but should remain
	// stable for API/ABI backwards compatibility, where additional built in
	// status codes don't affect existing ones.
	"error bad receiver",
	"error bad argument",
	"error constructor not called",
	"error unexpected EOF", // Used if reading when closed == true.
	"status short read",    // Used if reading when closed == false.
	"status short write",
	"error closed for writes",
}

func init() {
	if len(builtInStatuses) > userDefinedStatusBase {
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
	buffer     bytes.Buffer
	pkgName    string
	tm         *t.Map
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

	g.printf("// Code generated by puffs-gen-c. DO NOT EDIT.\n\n")
	g.writes(baseHeader)
	g.writes("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")

	g.writes("// ---------------- Status Codes\n\n")
	g.writes("// Status codes are non-positive integers.\n")
	g.writes("//\n")
	g.writes("// The least significant bit indicates a non-recoverable status code: an error.\n")
	g.writes("typedef enum {\n")
	for i, s := range builtInStatuses {
		nudge := ""
		if strings.HasPrefix(s, "error ") {
			nudge = "+1"
		}
		g.printf("%s = %d%s,\n", g.cName(s), -2*i, nudge)
	}
	for i, s := range g.statusList {
		nudge := ""
		if s.isError {
			nudge = "+1"
		}
		g.printf("%s = %d%s,\n", s.name, -2*(userDefinedStatusBase+i), nudge)
	}
	g.printf("} puffs_%s_status;\n\n", g.pkgName)
	g.printf("bool puffs_%s_status_is_error(puffs_%s_status s);\n\n", g.pkgName, g.pkgName)
	g.printf("const char* puffs_%s_status_string(puffs_%s_status s);\n\n", g.pkgName, g.pkgName)

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
	g.printf("bool puffs_%s_status_is_error(puffs_%s_status s) {"+
		"return s & 1; }\n\n", g.pkgName, g.pkgName)
	g.printf("const char* puffs_%s_status_strings[%d] = {\n", g.pkgName, len(builtInStatuses)+len(g.statusList))
	for _, s := range builtInStatuses {
		if strings.HasPrefix(s, "status ") {
			s = s[len("status "):]
		} else if strings.HasPrefix(s, "error ") {
			s = s[len("error "):]
		}
		s = g.pkgName + ": " + s
		g.printf("%q,", s)
	}
	for _, s := range g.statusList {
		g.printf("%q,", g.pkgName+": "+s.msg)
	}
	g.writes("};\n\n")

	g.printf("const char* puffs_%s_status_string(puffs_%s_status s) {\n", g.pkgName, g.pkgName)
	g.writes("s = -(s >> 1); if (0 <= s) {\n")
	g.printf("if (s < %d) { return puffs_%s_status_strings[s]; }\n",
		len(builtInStatuses), g.pkgName)
	g.printf("s -= %d;\n", userDefinedStatusBase-len(builtInStatuses))
	g.printf("if ((%d <= s) && (s < %d)) { return puffs_%s_status_strings[s]; }\n",
		len(builtInStatuses), len(builtInStatuses)+len(g.statusList), g.pkgName)
	g.printf("}\nreturn \"%s: unknown status\";\n", g.pkgName)
	g.writes("}\n\n")

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
	for _, r := range name {
		if 'A' <= r && r <= 'Z' {
			b = append(b, byte(r+'a'-'A'))
		} else if ('a' <= r && r <= 'z') || ('0' <= r && r <= '9') || ('_' == r) {
			b = append(b, byte(r))
		} else if ' ' == r {
			b = append(b, '_')
		}
	}
	return string(b)
}

func (g *gen) gatherStatuses(n *a.Status) error {
	msg := n.Message().String(g.tm)
	if len(msg) < 2 || msg[0] != '"' || msg[len(msg)-1] != '"' {
		return fmt.Errorf("bad status message %q", msg)
	}
	msg = msg[1 : len(msg)-1]
	prefix := "status "
	isError := n.Keyword().Key() == t.KeyError
	if isError {
		prefix = "error "
	}
	s := status{
		name:    g.cName(prefix + msg),
		msg:     msg,
		isError: isError,
	}
	g.statusList = append(g.statusList, s)
	g.statusMap[n.Message()] = s
	return nil
}

func (g *gen) writeStruct(n *a.Struct) error {
	// For API/ABI compatibility, the very first field in the struct's
	// private_impl must be the status code. This lets the constructor callee
	// set "self->private_impl.status = etc_error_bad_version;" regardless of
	// the sizeof(*self) struct reserved by the caller and even if the caller
	// and callee were built with different versions.
	structName := n.Name().String(g.tm)
	g.writes("typedef struct {\n")
	g.writes("// Do not access the private_impl's fields directly. There is no API/ABI\n")
	g.writes("// compatibility or safety guarantee if you do so. Instead, use the\n")
	g.printf("// puffs_%s_%s_etc functions.\n", g.pkgName, structName)
	g.writes("//\n")
	g.writes("// In C++, these fields would be \"private\", but C does not support that.\n")
	g.writes("//\n")
	g.writes("// It is a struct, not a struct*, so that it can be stack allocated.\n")
	g.writes("struct {\n")
	if n.Suspendible() {
		g.printf("puffs_%s_status status;\n", g.pkgName)
		g.printf("uint32_t magic;\n")
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
				g.writes("uint32_t coro_state;\n")
				if err := g.writeVars(o.Body()); err != nil {
					return err
				}
				g.printf("} %s%s[%d];\n", cPrefix, o.Name().String(g.tm), maxDepth)
			}
		}
	}

	g.printf("} private_impl;\n } puffs_%s_%s;\n\n", g.pkgName, structName)
	return nil
}

func (g *gen) writeCtorSignature(n *a.Struct, public bool, ctor bool) {
	structName := n.Name().String(g.tm)
	ctorName := "destructor"
	if ctor {
		ctorName = "constructor"
		if public {
			g.printf("// puffs_%s_%s_%s is a constructor function.\n", g.pkgName, structName, ctorName)
			g.printf("//\n")
			g.printf("// It should be called before any other puffs_%s_%s_* function.\n",
				g.pkgName, structName)
			g.printf("//\n")
			g.printf("// Pass PUFFS_VERSION and 0 for puffs_version and for_internal_use_only.\n")
		}
	}
	g.printf("void puffs_%s_%s_%s(puffs_%s_%s *self", g.pkgName, structName, ctorName, g.pkgName, structName)
	if ctor {
		g.printf(", uint32_t puffs_version, uint32_t for_internal_use_only")
	}
	g.printf(")")
}

func (g *gen) writeCtorPrototype(n *a.Struct) error {
	if !n.Suspendible() {
		return nil
	}
	for _, ctor := range []bool{true, false} {
		g.writeCtorSignature(n, n.Public(), ctor)
		g.writes(";\n\n")
	}
	return nil
}

func (g *gen) writeCtorImpl(n *a.Struct) error {
	if !n.Suspendible() {
		return nil
	}
	for _, ctor := range []bool{true, false} {
		g.writeCtorSignature(n, false, ctor)
		g.printf("{\n")
		g.printf("if (!self) { return; }\n")

		if ctor {
			g.printf("if (puffs_version != PUFFS_VERSION) {\n")
			g.printf("self->private_impl.status = puffs_%s_error_bad_version;\n", g.pkgName)
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
				g.printf("puffs_%s_%s_constructor(&self->private_impl.%s%s,"+
					"PUFFS_VERSION, PUFFS_ALREADY_ZEROED);\n",
					g.pkgName, x.Name().String(g.tm), fPrefix, f.Name().String(g.tm))
			} else {
				g.printf("puffs_%s_%s_destructor(&self->private_impl.%s%s);\n",
					g.pkgName, x.Name().String(g.tm), fPrefix, f.Name().String(g.tm))
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

func (g *gen) writeExprAssociativeOp(n *a.Expr, rp replacementPolicy, depth uint32) error {
	opName := cOpNames[0xFF&n.ID0().Key()]
	for i, o := range n.Args() {
		if i != 0 {
			g.writes(opName)
		}
		if err := g.writeExpr(o.Expr(), rp, parenthesesMandatory, depth); err != nil {
			return err
		}
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
		g.printf("puffs_%s_%s", g.pkgName, n.Name().String(g.tm))
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
	t.KeyEq:       " = ",
	t.KeyPlusEq:   " += ",
	t.KeyMinusEq:  " -= ",
	t.KeyStarEq:   " *= ",
	t.KeySlashEq:  " /= ",
	t.KeyShiftLEq: " <<= ",
	t.KeyShiftREq: " >>= ",
	t.KeyAmpEq:    " &= ",
	t.KeyAmpHatEq: " no_such_amp_hat_C_operator ",
	t.KeyPipeEq:   " |= ",
	t.KeyHatEq:    " ^= ",

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
