Puffs is a domain-specific language and library for parsing untrusted file
formats safely. Examples of such file formats include images, audio, video,
fonts and compressed archives.

Unlike the C programming language, Puffs is safe with respect to buffer
overflows, integer arithmetic overflows and null pointer dereferences. The key
difference between Puffs and other memory-safe languages is that all such
checks are done at compile time, not at run time. The aim is to produce
software libraries that are as safe as Go or Rust, roughly speaking, but as
fast as C, and that can be used anywhere C libraries are used. This includes
very large C/C++ products, such as popular web browsers and operating systems
(using that term to include desktop and mobile user interfaces, not just the
kernel).

The trade-off in aiming for both safety and speed is that Puffs programs take
longer for a programmer to write, as they have to explicitly annotate their
programs with proofs of safety. A statement like `x += 1` unsurprisingly means
to increment the variable `x` by `1`. However, in Puffs, such a statement is a
compile time error unless the compiler can also prove that `x` is not the
maximal value of `x`'s type (e.g. `x` is not `255` if `x` is a `u8`), as the
increment would otherwise overflow. Similarly, an integer arithmetic expression
like `x / y` is a compile time error unless the compiler can also prove that
`y` is not zero.

Puffs is not a general purpose programming language. While technically
possible, it is unlikely that a Puffs compiler would be worth writing in Puffs.


# What Does It Look Like?

The `std/gif/decode_lzw.puffs` file is a good example. See the "Your First
Edit" section below for more guidance.


# Background

Parsing untrusted data, such as images downloaded from across the web, have a
long history of security vulnerabilities. As of 2017, libpng is over 18 years
old, and the [PNG specification is dated 2003](https://www.w3.org/TR/PNG/), but
that well examined C library is still getting [CVE's published in
2017](https://www.cvedetails.com/vulnerability-list/vendor_id-7294/year-2017/Libpng.html).

Sandboxing and fuzzing can mitigate the danger, but they are reactions to C's
fundamental unsafety. Newer programming languages remove entire classes of
potential security bugs. Buffer overflows and null pointer dereferences are
amongst the most well known.

Less well known are integer overflow bugs. Offset-length pairs, defining a
sub-section of a file, are seen in many file formats, such as OpenType fonts
and PDF documents. A conscientious C programmer might think to check that a
section of a file or a buffer is within bounds by writing `if (offset + length
< end)` before processing that section, but that addition can silently
overflow, and a maliciously crafted file might bypass the check.

A variation on this theme is where `offset` is a pointer, exemplified by
[capnproto's
CVE-2017-7892](https://github.com/sandstorm-io/capnproto/blob/master/security-advisories/2017-04-17-0-apple-clang-elides-bounds-check.md)
and [another
example](https://www.blackhat.com/docs/us-14/materials/us-14-Rosenberg-Reflections-on-Trusting-TrustZone.pdf).
For a pointer-typed offset, witnessing such a vulnerability can depend on both
the malicious input itself and the addresses of the memory the software used to
process that input. Those addresses can vary from run to run and from system to
system, e.g. 32-bit versus 64-bit systems and whether dynamically allocated
memory can have sufficiently high address values, and that variability makes it
harder to reproduce and to catch such subtle bugs from fuzzing.

In C, some integer overflow is *undefined behavior*, as per [the C99 spec
section 3.4.3](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf). In
Go, integer overflow is [silently
ignored](https://golang.org/ref/spec#Integer_overflow). In Rust, integer
overflow is [checked at run time in debug mode and silently ignored in release
mode](http://huonw.github.io/blog/2016/04/myths-and-legends-about-integer-overflow-in-rust/)
by default, as the run time performance penalty was deemed too great. In Swift,
it's a [run time
error](https://developer.apple.com/library/content/documentation/Swift/Conceptual/Swift_Programming_Language/AdvancedOperators.html#//apple_ref/doc/uid/TP40014097-CH27-ID37).
In D, it's [configurable](http://dconf.org/2017/talks/alexandrescu.pdf). Other
languages like Python and Haskell can automatically spill into 'big integers'
larger than 64 bits, but this can have a performance impact when such integers
are used in inner loops.

Even if overflow is checked, it is usually checked at run time. Similarly,
modern languages do their bounds checking at run time. An expression like
`a[i]` is really `if ((0 <= i) && (i < a.length)) { use a[i] } else { throw }`,
in mangled pseudo-code. Compilers for these languages can often eliminate many
of these bounds checks, e.g. if `i` is an iterator index, but not always all of
them.

The run time cost is small, measured in nanoseconds. But if an image decoding
library has to eat this cost per pixel, and you have a megapixel image, then
nanoseconds become milliseconds, and milliseconds can matter.

In comparison, in Puffs, all bounds checks and arithmetic overflow checks
happen at compile time, with zero run time overhead.


# Getting Started

Puffs code (that is proved safe via explicit assertions) is compiled to C code
(with those assertions removed) - it is transpiled. If you are a C/C++
programmer and just want to *use* the C edition of the Puffs standard library,
then clone the repository and look at the files in the `gen/c` and `gen/h`
directories. No other software tools are required and there are no library
dependencies, other than C standard library concepts like `<stdint.h>`'s
`uint32_t` type and `<string.h>`'s `memset` function.

If your C/C++ project is large, you might want both the .c files (adding each
to your build system) and the .h files. If your C/C++ project is small, you
might only need the .c files, not the .h files, as the .c files are designed to
be a [drop-in library](http://gpfault.net/posts/drop-in-libraries.txt.html).
For example, if you want a GIF decoder, you only need `gif.c`. See TODO for an
example. More complicated decoders might require multiple .c files - multiple
modules. For example, the PNG codec (TODO) requires the flate codec (TODO), but
they are separate files, since HTTP can use also flate compression (also known
as gzip or zlib, roughly speaking) without necessarily processing PNG images.


## Getting Deeper

If you want to modify the Puffs standard library, or compile your own Puffs
code, you will need to do a little more work, and will have to install at least
the Go toolchain in order to build the Puffs tools. To run the test suite, you
might also have to install C compilers like clang and gcc, as well as C
libraries (and their .h files) like libjpeg and libpng, as some tests (TODO)
compare that Puffs produces exactly the same output as these other libraries.

Puffs is not published yet, but the working assumption is that the eventual
home of the software project will be at `github.com/google/puffs`. After
cloning the repository, move it to that path under your `$GOPATH`.

Running `go install -v github.com/google/puffs/cmd/...` will then install the
puffs tools. The ones that you'll most often use are `puffsfmt` (analogous to
`clang-format`, `gofmt` or `rustfmt`) and `puffs` (roughly analogous to `make`,
`go` or `cargo`).

You should now be able to run `puffs test`. If all goes well, you should see
some output containing the word "PASS" multiple times.


## Your First Edit

Feel free to edit the `std/gif/decode_lzw.puffs` file, which implements the GIF
LZW decoder. After editing, run `puffs gen std/gif` or `puffs test std/gif` to
re-generate the C edition of the Puffs standard library's GIF codec, and
optionally run its tests.

Try deleting an assert statement and re-running `puffs gen`. The result should
be syntactically valid, but a compile error, as some bounds checks can no
longer be proven.

Find the line `var bits u32`, which declares the bits variable and initializes
it to zero. Try adding `bits -= 1` on a new line of code after it. Again,
`puffs gen` should fail, as the computation can underflow.

Similarly, replacing the line `var n_bits u32` with `var n_bits u32 = 10`
should fail, as an `n_bits < 8` assertion, a pre-condition, a few lines further
down again cannot be proven.

Similarly, changing the `4095` in `var prev_code u32[..4095]` either higher or
lower should fail.

Try adding `assert false` at various places, which should obviously fail, but
should also cause `puffs gen` to print what facts the compiler can prove at
that point. This can be useful when debugging why Puffs can't prove something
you think it should be able to.


## Directory Layout

- `lang` holds the Go libraries that implement the Puffs language: tokenizer,
  AST, parser, renderer, etc. The Puffs tools are written in Go, but as
  mentioned above, Puffs transpiles to C code, and Go is not necessarily
  involved if all you want is to use the C edition of Puffs.
- `cmd` holds Puffs' command line tools, also written in Go.
- `std` holds the Puffs standard library's code. The initial focus is on
  popular image codecs: BMP, GIF, JPEG, PNG, TIFF and WEBP.
- `gen` holds the transpiled editions of that standard library. The initial
  focus is generating C code. Later on, the repository might include generated
  Go and Rust code.
- `test` holds the tests for the Puffs standard library.
- `script` holds miscellaneous utility programs.
- `doc` holds additional documentation. TODO.
- `example` holds example programs. TODO.

For a guide on how various things work together, the "99ff8e2 Let fields have
default values" commit is an example of adding new Puffs syntax and threading
that all the way through to C code generation and testing.


# The Puffs Language

Puffs is an imperative, C-like language and much of it should look familiar to
somebody versed in any one of C, C++, D, Go, Java, JavaScript, Objective-C,
Rust, Swift, etc. Accordingly, this section is not an exhaustive specification,
merely a rough guide of where Puffs differs.

Like Go, [semi-colons can be omitted](https://golang.org/ref/spec#Semicolons),
and `puffsfmt` will remove them. Similarly, the body of an `if` or `while` must
be enclosed by curly `{}`s. There is no 'dangling else' ambiguity.


## Keywords

Puffs is a simple language, with only 21 keywords. 5 of these introduce
top-level concepts:

- `error`
- `func`
- `status`
- `struct`
- `use`

2 keywords distinguish between public and private API:

- `pri`
- `pub`

6 keywords deal with control flow within a function:

- `break`
- `continue`
- `else`
- `if`
- `return`
- `while`

5 keywords deal with assertions:

- `assert`
- `inv`
- `post`
- `pre`
- `via`

2 keywords deal with types:

- `nptr`
- `ptr`

1 keyword deals with local variables:

- `var`


## Operators

All binary operators have equal precedence. `a * b + c` is an invalid
expression. You must explicitly write either `(a * b) + c` or `a * (b + c)`.

Some binary operators (`+`, `*`, `&`, `|`, `^`, `and`, `or`) are also
associative: `(a + b) + c` and `a + (b + c)` are equivalent, and can be written
as `a + b + c`.

The logical operators, `&&` and `||` and `!` in C, are written as `and` and
`or` and `not` in Puffs.

TODO: ignore-overflow ops, equivalent to Swift's `&+`.

Converting an expression `x` to the type `T` is written as `x as T`.


## Types

Types read from left to right: `ptr [100] u32` is a pointer to a 100-element
array of unsigned 32-bit integers. `ptr` here means a non-null pointer. Use
`nptr` for a nullable pointer type.

Integer types can also be refined: `var x u32[10..20]` defines a variable x
that is stored as 4 bytes (32 bits) and can be combined arithmetically (e.g.
added, compared) with other `u32`s, but whose value must be between 10 and 20
inclusive. The syntax is reminiscent of Pascal's subranges, but in Puffs, a
subsequent assignment like `x = 21` or even `x = y + z` is a compile time error
unless the right hand side can be proven to be within range.

Types can also provide a default value, such as `u32[10..20] = 16`, especially
if zero is out of range. If not specified, the implicit default is zero.

Refinement bounds may be omitted, where the base integer type provides the
implicit bound. `var x u8[..5]` means that `x` is between 0 and 5. `var y
i8[-7..]` means that `y` is between -7 and +127. `var z u32[..]` is equivalent
to `var z u32`.

Refinement bounds must be constant expressions. `var x u32[..2+3]` is valid,
but `var x u32; var y u32[..x]` is not. Puffs does not have dependent types.
Relationships such as `y <= x` are expressible as assertions (see below), but
not by the type system.


## Structs

Structs are a list of fields, enclosed in parentheses: `struct point(x i32, y
i32)`. The struct name may be followed by a question mark `?`, which means that
its methods may be coroutines. (See below).


## Functions

Function signatures read from left to right: `func max(x i32, y i32)(z i32)` is
a function that takes two `i32`s and returns one `i32`. Two pairs of
parentheses are required: a function that in other languages would return
`void` in Puffs returns the empty struct `()`, also known as the unit type.

When calling a function, each argument must be named. It is `m = max(x:10,
y:20)` and not `m = max(10, 20)`.

The function name, such as `max`, may be followed by either an exclamation mark
`!` or a question mark `?` but not both. An exclamation mark means that the
function is impure, and may assign to things other than its local variables. A
question mark means that the function is impure and furthermore a coroutine: it
can be suspended while returning a (recoverable) status, such as needing more
input data, or a (fatal) error, such as being invalid with respect to a file
format. If suspended with a status, calling that function again will resume at
the suspension point, not necessarily at the top of the function body. If
suspended with an error, calling that function again will return the same
error.

Some functions are methods, with syntax `func foo.bar(etc)(etc)`, where `foo`
names a struct type and `bar` is the method name. Within the function body, an
implicit `this` argument will point to the receiving struct. Methods can also
be marked as impure or coroutines.


## Variables

Variable declarations are [hoisted like
JavaScript](https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/var),
so that all variables have function scope. Proofs are easier to work with, for
both humans and computers, when one variable can't shadow another variable with
the same name.


## Assertions

Assertions state a boolean typed, pure expression, such as `assert x >= y`,
that the compiler must prove. As a consequence, the compiler can then prove
that, if `x` and `y` have type `u32`, the expression `x - y` will not
underflow.

An assertion applies for a particular point in the program. Subsequent
assignments or impure function calls, such as `x = z` or `f!()`, can invalidate
previous assertions. See the "Facts" section below for more details.

Arithmetic inside assertions is performed in ideal integer math, working in the
integer ring ℤ. An expression like `x + y` in an assertion never overflows,
even if `x` and `y` have a realized (non-ideal) integer type like `u32`.

The `assert` keyword makes an assertion as a statement, and can occur inside a
block of code like any other statement, such as an assignment `x = y` or an
`if` statement. The `pre`, `post` and `inv` keywords are similar to `assert`, but
apply only to `func` bodies and `while` loops.

`pre` states a pre-condition: an assertion that must be proven on every
function call or on every entry to the loop. Loop entry means the initial
execution of the loop, plus every explicit `continue` statement that targets
that `while` loop, plus the implicit `continue` at the end of the loop. For
`while` loops, the pre-condition must be proven before the condition (the `x <
y` in `while x < y { etc }`) executes.

`post` states a post-condition: an assertion that must be proven on every
function return (whether by an explicit `return` statement or by an implicit
`return` at the end of the function body) or on every loop exit (whether by an
explicit `break` statement or by the implicit `break` when the condition
evaluates false).

For example, a `while` loop that contains no explicit `break` statements can
always claim the inverse of its condition as a post-condition: `while x < y,
post x >= y { etc }`.

`inv` states an invariant, which is simply an assertion that is both a
pre-condition and post-condition.

When a `func` or `while` has multiple assertions, they must be listed in `pre`,
`inv` and then `post` order.


## Facts

Static constraints are expressed in the type system and apply for the lifetime
of the variable, struct field or function argument. For example, `var x u32`
and `var p ptr u32` constrain `x` to be non-negative and `p` to be non-null.
Furthermore, `var y u32[..20]` constrains `y` to be less than or equal to `20`.

Dynamic constraints, also known as facts, are previously seen assertions,
explicit or implicit, that are known to hold at a particular point in the
program. The set of known facts can grow and shrink over the analysis of a
program's statements. For example, the assignment `x = 7` can both add the
implicit assertion `x == 7` to the set of known facts, as well as remove any
other facts that mention the variable `x`, as `x`'s value might have changed.
TODO: define exactly when facts are dropped or updated.

Similarly, the sequence `assert x < y; z = 3` would result in set of known
facts that include both the explicit assertion `x < y` and the implicit
assertion `z == 3`, if none of `x`, `y` and `z` alias another (e.g. they are
all local variables).

Puffs has two forms of non-sequential control flow: `if` branches (including
`if`, `else if`, `else if` chains) and `while` loops.

For an `if` statement, such as `if b { etc0 } else { etc1 }`, the condition `b`
is a known fact inside the if-true branch `etc0` and its inverse `not b` is a
known fact inside the if-false branch `etc1`. After that `if` statement, the
overall set of known facts is the intersection of the set of known facts after
each non-terminating branch. A terminating branch is a non-empty block of code
whose final statement is a `return`, `break`, `continue` or an `if`, `else if`
chain where the final `else` is present and all branches terminate. TODO: also
allow ending in `while true`?

For a `while` statement, such as `while b { etc }`, the set of known facts at
the start of the body `etc` is precisely the condition `b` plus all `pre` and
`inv` assertions. No other prior facts carry into the loop body, as the loop
can re-start coming from other points in the program (i.e. an explicit or
implicit `continue`) If the `while` loop makes no such `pre` or `inv`
assertions, no facts are known other than `b`.

Similarly, the set of known facts after the `while` loop exits is precisely its
`inv` and `post` assertions, and no other. In other words, a `while` loop must
explicitly list its state of the world just before and just after it executes.
This includes facts (i.e. invariants) about variables that are not mentioned at
all by the `while` condition or its body but are proven before the `while` loop
and assumed after it.


## Proofs

Puffs' assertion and bounds checking system is a proof checker, not a full SMT
solver or automated theorem prover. It verifies explicit annotations instead of
the more open-ended task of searching for implicit proof steps. This involves
more explicit work by the programmer, but compile times matter, so Puffs is
fast (and dumb) instead of smart (and slow).

Nonetheless, the Puffs syntax is regular (and unlike C++, does not require a
symbol table to parse), so it should be straightforward to transform Puffs code
to and from file formats used by more sophisticated proof engines.

Some rules are applied automatically by the proof checker. For example, if `x
<= 10` and `y <= 5` are both known true, whether by a static constraint (the
type system) or dynamic constraint (an asserted fact), then the checker knows
that the expression `x + y` is bounded above by `10 + 5` and therefore will not
overflow a `u8` (but would overflow a `u8[..12]`).

TODO: rigorously specify these automatic rules, when we have written more Puffs
code and thus have more experience on what rules are needed to implement
multiple, real world image codecs.

Other rules are built in to the proof checker but are not applied automatically
(see "fast... instead of smart" above). Such rules have double-quote enclosed
names that look a little like mathematical statements. They are axiomatic, in
that these rules are assumed, not proved, by the Puffs toolchain. They are
typically at a higher level than e.g. Peano axioms, as Puffs emphasizes
practicality over theoretical minimalism. As they are axiomatic, they endeavour
to only encode 'obvious' mathematical rules. For example, the rule named `"a <
b: a < c; c <= b"` is one expression of transitivity: the assertion `a < b` is
proven if both `a < c` and `c <= b` are provable, for some expression `c`.
Terms like `a`, `b` and `c` here are all integers in ℤ, they do not encompass
floating point concepts like negative zero, `NaN`s or rounding.

Such rules are invoked by the `via` keyword. For example, `assert n_bits < 12
via "a < b: a < c; c <= b"(c:width)` makes the assertion `n_bits < 12` by
applying that transitivity rule, where `a` is `n_bits`, `b` is `12` and `c` is
`width`. The trailing `(c:width)` syntax is deliberately similar to a function
call (recall that when calling a function, each argument must be named), but
the `"a < b: a < c; c <= b"` named rule is not a function-typed expression.

TODO: specify these built-in `via` rules, again after more experience.


## Miscellaneous Language Notes

Labeled `break` and `continue` statements enable jumping out of loops that
aren't the most deeply nested. The syntax for labels is `while:label` instead
of Java's `label:while`, as the former is slightly easier to parse, and Puffs
does not otherwise use labels for switch cases or goto targets.

TODO: describe the built in `buf1` and `buf2` types: 1- and 2-dimensional
buffers of bytes, such as an I/O stream or a table of pixel data.


# Related Work


## Why a New Language?

Simpler languages are easier to prove things about. Macros, inheritance,
closures, generics, operator overloading, goto's and built-in container types
are all useful features, but as mentioned at the top, Puffs is not a general
purpose programming language. Instead, its focus is on compile-time provable
memory safety, with C-like performance, for CPU-intensive file format decoders.

Nonetheless, Puffs is an imperative language, not a functional language,
despite the long history of functional languages and provability. Inner loop
performance usage matters, and it is easier to match the optimization
techniques of incumbent C libraries with a C-like language than with a
functional language. Compared to something like
[Liquid Haskell](https://ucsd-progsys.github.io/liquidhaskell-blog/), Puffs
also has no garbage collector overhead, as it has no garbage collector.

Memory usage also matters, again considering per-pixel costs can be multiplied
by millions of pixels. It is important to represent e.g. an RGBA pixel as four
u8 values (or one u32), not as four naturally-sized-for-the-CPU integers or
four big precision integers.

It is a language by itself, integrated with a compiler, not something embedded
in the comments of another language's program, supported by a separate program
checker, as the bounds check elimination affects the generated code.


## Why Not Rust?

Puffs transpiles to standard C99 code with no dependencies, and should run
anywhere that has a C99 compliant C compiler. An existing C/C++ project, such
as the Chromium web browser, can choose to replace e.g. libpng with Puffs PNG,
without needing any additional toolchains. Sure, languages like D and Rust have
great binary-level interop with C code, and Mozilla are reporting progress with
parsing media formats in Rust, but it's still a non-zero operational hurdle to
grow a project's build process and to assess build times and binary sizes.

We're not claiming that "write it in Rust" is an unworkable approach. It's just
a different approach, with different trade-offs.

[Nom](https://github.com/Geal/nom) is a
[promising](http://spw17.langsec.org/papers/chifflier-parsing-in-2017.pdf)
parser combinator library, in Rust. Puffs differs from nom by itself, in that
Puffs is an end to end implementation, not limited to that part of a file
format that is easily expressible as a formal grammar. In particular, it also
handles entropy encodings such as LZW (for GIF), ZLIB (for PNG, TODO) and
Huffman (for JPEG, TODO). Puffs differs from nom combined with other Rust code
(e.g. a Rust LZW implementation) in that bounds and overflow checks not just
ubiquitous but also completely eliminated at compile time. Again, we're not
claiming that nom or Rust are unworkable, just different.


# Status

Proof of concept. Version 0.1 at best. API and ABI aren't stabilized yet. There
are plenty of tests to create, docs to write and TODOs to do. The compiler
undoubtedly has bugs. Assertion checking needs more rigor, especially around
side effects and aliasing, and being sufficiently well specified to allow
alternative implementations. Lots of detail needs work, but the broad
brushstrokes are there.


## Compatibility

TODO: get a decently large corpus of test images from a web crawl. Ensure that
Puffs produces 100% identical decodings to the suite of giflib, libjpeg,
libpng, etc.

TODO: also trawl through Go's bug tracker for "this image failed to load".


## Benchmarks

TODO.


## Code Size

TODO.


## Roadmap

Short term:

- Do lots of TODOs.
- Implement coroutines, for streaming decoding.
- Finish the GIF decoder.
- Write the PNG decoder.
- Write the JPEG decoder.
- Write the PNG encoder.
- Design an image API that works with all these, including animated GIF.
- Write some example programs, maybe cpng a la cjpeg and cwebp.
- Add compatibility tests, benchmarks, code size comparison.
- Release as open source.

Medium term:

- Write the BMP/ICO decoder.
- Write the TIFF decoder.
- Write the WEBP decoder.
- Write the JPEG encoder.
- Get to 100% compatibility, on a large web crawl corpus, with libpng etc.
- Optimize the codecs; benchmark comparably to libpng etc. Might require SIMD.
- Ensure code size compares favorably to libpng etc.
- Fuzz it, both for crashers and for different output to libpng etc.

Long term:

- Ship with Google Chrome: safer code, smaller binaries, no regressions.
- Ship a version 1.0: stabilize the language and library APIs.
- Write a language spec.
- Generate Go code.
- Generate Rust code.

Very long term:

- Other file formats? Fonts? PDF? Audio? Video?
- Other image operations? Image scaling that's both safe and fast?


# Discussion

TODO: set up a mailing list.


# License

BSD-style with patent grant. See the LICENSE and PATENTS files for details.

---

Updated on June 2017.
