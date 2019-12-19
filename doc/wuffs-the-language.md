# Wuffs the Language

The Wuffs project is both a programming language and a standard library written
in that programming language (and then e.g. transpiled to C). For more details
on the latter, see the [Wuffs the Library](/doc/wuffs-the-library.md) document.
As for Wuffs the Language, it is an imperative C-like *memory-safe* language
that should look roughly familiar to somebody versed in one of C, C++, Go,
Java, JavaScript, Rust, etc. The major differentiating features are:

- [Facts](/doc/note/facts.md) and compile-time
  [assertions](/doc/note/assertions.md).
- Type [refinements](/doc/glossary.md#refinement-type).
- [Coroutines](/doc/note/coroutines.md) and an [effects](/doc/note/effects.md)
  system. The `!` and `?` exclamation and question marks denote impure and
  coroutine effects.
- No way to dynamically allocate or free [memory](/doc/note/memory-safety.md).

Minor features or restrictions include:

- Nullable and non-nullable pointers, spelled `nptr T` and `ptr T`.
- Integrated [I/O](/doc/note/io-input-output.md).
- [Iterate loops](/doc/note/iterate-loops.md).
- Public vs private API is marked with the `pub` and `pri` keywords. Visibility
  boundaries are at the package level, unlike C++ or Java's type level.
- No string type, only [slices](/doc/note/slices-arrays-and-tables.md) of
  `base.u8`. Literals like `"#bad checksum"` are actually
  [statuses](/doc/note/statuses.md).
- No variable shadowing. All local variables must be declared before any other
  statements in a function body.
- Like Go, semi-colons can be omitted. Similarly, the `()` parentheses around
  an `if` or `while` condition are optional, but the `{}` curly braces are
  mandatory. There is no 'dangling else' ambiguity.
- Labeled jumps look like `break.loopname` and `continue.loopname`, for a
  matching `while.loopname`.

Wuffs code is formatted by the
[`wuffsfmt`](https://godoc.org/github.com/google/wuffs/cmd/wuffsfmt) program.


## Types

Types read from left to right: `ptr array[100] base.u32` is a non-null pointer
to a 100-element array of unsigned 32-bit integers. Types can also be
[refined](/doc/glossary.md#refinement-type).


## Structs

Structs are a list of fields, enclosed in parentheses: `struct foo(x: base.u32,
y: base.u32)`, possibly extended with a second list of [optionally
initialized](/doc/note/initialization.md#partial-zero-initialization) fields.
The struct name, `foo`, may be followed by a question mark `?`, which means
that its methods may be [coroutines](/doc/note/coroutines.md).


## Functions

All functions are methods (with an implicit `this` argument). There are no
free-standing functions.

Function definitions read from left to right. `func foo.bar(x: base.u32, y:
base.u32) base.u32` is a function (a method on the `foo` struct type) that
takes two `base.u32`s and returns a `base.u32`. Each argument must be named at
the call site. It is `m = f.bar(x: 10, y: 20)`, not `m = f.bar(10, 20)`.


## Operators

There is no operator precedence. A bare `a * b + c` is an invalid expression.
You must explicitly write either `(a * b) + c` or `a * (b + c)`.

Some binary operators (`+`, `*`, `&`, `|`, `^`, `and`, `or`) are also
associative: `(a + b) + c` and `a + (b + c)` are equivalent, and can be spelled
`a + b + c`.

The logical operators, `&&` and `||` and `!` in C, are spelled `and` and `or`
and `not` in Wuffs. Not-equals is spelled `<>`, as the `!` exclamation mark is
reserved for impure effects.

Expressions involving the standard arithmetic operators (e.g. `*`, `+`), will
not compile if overflow is possible. Some of these operators have alternative
'tilde' forms (e.g. `~mod*`, `~sat+`) which provide
[modular](/doc/glossary.md#modular-arithmetic.md) and
[saturating](/doc/glossary.md#saturating-arithmetic.md) arithmetic. By
definition, these never overflow.

The `as` operator, e.g. `x as T`, converts an expression `x` to the type `T`.


## Introductory Example

A simple Wuffs the Language program, unrelated to Wuffs the Library, is
discussed in [/hello-wuffs-c](/hello-wuffs-c).
