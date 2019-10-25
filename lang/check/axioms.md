# Axioms

This file lists Wuffs' axioms: a fixed list of built-in, self-evident rules to
combine existing facts to create new facts. For example, given numerical-typed
expressions `a`, `b` and `c`, the two facts that `a < c` and `c <= b` together
imply that `a < b`: less-than-ness is transitive.

Wuffs code represents this axiom by the string `"a < b: a < c; c <= b"`, and it
is invoked by the `assert` and `via` keywords, naming the rule and [binding the
expressions `a`, `b` and
`c`](https://github.com/google/wuffs/blob/4080840928c0b05a80cda0d14ac2e2615f679f1a/std/lzw/decode_lzw.wuffs#L99).

This file is not just documentation. It is
[parsed](https://github.com/google/wuffs/blob/master/lang/check/gen.go) to give
the list of axioms built into the Wuffs compiler. Run `go generate` after
editing this list.

---

- `"a < b: b > a"`
- `"a < b: a < c; c < b"`
- `"a < b: a < c; c == b"`
- `"a < b: a == c; c < b"`
- `"a < b: a < c; c <= b"`
- `"a < b: a <= c; c < b"`

---

- `"a > b: b < a"`

---

- `"a <= b: b >= a"`
- `"a <= b: a <= c; c <= b"`
- `"a <= b: a <= c; c == b"`
- `"a <= b: a == c; c <= b"`

---

- `"a >= b: b <= a"`

---

- `"a < (b + c): a < c; 0 <= b"`
- `"a < (b + c): a < (b0 + c0); b0 <= b; c0 <= c"`

---

- `"a <= (a + b): 0 <= b"`

---

- `"(a + b) <= c: a <= (c - b)"`
