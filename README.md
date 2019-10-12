# Wrangling Untrusted File Formats Safely

([Formerly known as
Puffs](https://groups.google.com/d/topic/puffslang/ZX-ymyf8xh0/discussion):
Parsing Untrusted File Formats Safely).

Wuffs is a domain-specific language and library for wrangling untrusted file
formats safely. Wrangling includes parsing, decoding and encoding. Examples of
such file formats include images, audio, video, fonts and compressed archives.

Unlike the C programming language, Wuffs is safe with respect to buffer
overflows, integer arithmetic overflows and null pointer dereferences. The key
difference between Wuffs and other memory-safe languages is that all such
checks are done at compile time, not at run time. *If it compiles, it is safe*,
with respect to those three bug classes.

The aim is to produce software libraries that are as safe as Go or Rust,
roughly speaking, but as fast as C, and that can be used anywhere C libraries
are used. This includes very large C/C++ products, such as popular web browsers
and operating systems (using that term to include desktop and mobile user
interfaces, not just the kernel).

The trade-off in aiming for both safety and speed is that Wuffs programs take
longer for a programmer to write, as they have to explicitly annotate their
programs with proofs of safety. A statement like `x += 1` unsurprisingly means
to increment the variable `x` by `1`. However, in Wuffs, such a statement is a
compile time error unless the compiler can also prove that `x` is not the
maximal value of `x`'s type (e.g. `x` is not `255` if `x` is a `u8`), as the
increment would otherwise overflow. Similarly, an integer arithmetic expression
like `x / y` is a compile time error unless the compiler can also prove that
`y` is not zero.

Wuffs is not a general purpose programming language. While technically
possible, it is unlikely that a Wuffs compiler would be worth writing in Wuffs.


## What Does Wuffs Code Look Like?

The [`std/lzw/decode_lzw.wuffs`](./std/lzw/decode_lzw.wuffs) file is a good
example. See the "Poking Around" section below for more guidance.


## What Does Compile Time Checking Look Like?

For example, making this one-line edit to the GIF codec leads to a compile time
error. `wuffs gen` fails to generate the C code, i.e. fails to compile
(transpile) the Wuffs code to C code:

```diff
diff --git a/std/lzw/decode_lzw.wuffs b/std/lzw/decode_lzw.wuffs
index f878c5e..f10dcee 100644
--- a/std/lzw/decode_lzw.wuffs
+++ b/std/lzw/decode_lzw.wuffs
@@ -98,7 +98,7 @@ pub func lzw_decoder.decode?(dst ptr buf1, src ptr buf1, src_final bool)() {
                        in.dst.write?(x:s)

                        if use_save_code {
-                               this.suffixes[save_code] = c as u8
+                               this.suffixes[save_code] = (c + 1) as u8
                                this.prefixes[save_code] = prev_code as u16
                        }
```

```
$ wuffs gen std/gif
check: expression "(c + 1) as u8" bounds [1 ..= 256] is not within bounds [0 ..= 255] at
/home/n/go/src/github.com/google/wuffs/std/lzw/decode_lzw.wuffs:101. Facts:
    n_bits < 8
    c < 256
    this.stack[s] == (c as u8)
    use_save_code
```

In comparison, this two-line edit will compile (but the "does it decode GIF
correctly" tests then fail):

```diff
diff --git a/std/lzw/decode_lzw.wuffs b/std/lzw/decode_lzw.wuffs
index f878c5e..b43443d 100644
--- a/std/lzw/decode_lzw.wuffs
+++ b/std/lzw/decode_lzw.wuffs
@@ -97,8 +97,8 @@ pub func lzw_decoder.decode?(dst ptr buf1, src ptr buf1, src_final bool)() {
                        // type checking, bounds checking and code generation for it).
                        in.dst.write?(x:s)

-                       if use_save_code {
-                               this.suffixes[save_code] = c as u8
+                       if use_save_code and (c < 200) {
+                               this.suffixes[save_code] = (c + 1) as u8
                                this.prefixes[save_code] = prev_code as u16
                        }
```

```
$ wuffs gen std/gif
gen wrote:      /home/n/go/src/github.com/google/wuffs/gen/c/gif.c
gen unchanged:  /home/n/go/src/github.com/google/wuffs/gen/h/gif.h
$ wuffs test std/gif
gen unchanged:  /home/n/go/src/github.com/google/wuffs/gen/c/gif.c
gen unchanged:  /home/n/go/src/github.com/google/wuffs/gen/h/gif.h
test:           /home/n/go/src/github.com/google/wuffs/test/c/gif
gif/basic.c     clang   PASS (8 tests run)
gif/basic.c     gcc     PASS (8 tests run)
gif/gif.c       clang   FAIL test_lzw_decode: bufs1_equal: wi: got 19311, want 19200.
contents differ at byte 3 (in hex: 0x000003):
  000000: dcdc dc00 00d9 f5f9 f6df dc5f 393a 3a3a  ..........._9:::
  000010: 3a3b 618e c8e4 e4e4 e5e4 e600 00e4 bbbb  :;a.............
  000020: eded 8f91 9191 9090 9090 9190 9192 9192  ................
  000030: 9191 9292 9191 9293 93f0 f0f0 f1f1 f2f2  ................
excerpts of got (above) versus want (below):
  000000: dcdc dcdc dcd9 f5f9 f6df dc5f 393a 3a3a  ..........._9:::
  000010: 3a3a 618e c8e4 e4e4 e5e4 e6e4 e4e4 bbbb  ::a.............
  000020: eded 8f91 9191 9090 9090 9090 9191 9191  ................
  000030: 9191 9191 9191 9193 93f0 f0f0 f1f1 f2f2  ................

gif/gif.c       gcc     FAIL test_lzw_decode: bufs1_equal: wi: got 19311, want 19200.
contents differ at byte 3 (in hex: 0x000003):
  000000: dcdc dc00 00d9 f5f9 f6df dc5f 393a 3a3a  ..........._9:::
  000010: 3a3b 618e c8e4 e4e4 e5e4 e600 00e4 bbbb  :;a.............
  000020: eded 8f91 9191 9090 9090 9190 9192 9192  ................
  000030: 9191 9292 9191 9293 93f0 f0f0 f1f1 f2f2  ................
excerpts of got (above) versus want (below):
  000000: dcdc dcdc dcd9 f5f9 f6df dc5f 393a 3a3a  ..........._9:::
  000010: 3a3a 618e c8e4 e4e4 e5e4 e6e4 e4e4 bbbb  ::a.............
  000020: eded 8f91 9191 9090 9090 9090 9191 9191  ................
  000030: 9191 9191 9191 9193 93f0 f0f0 f1f1 f2f2  ................

wuffs-test-c: some tests failed
wuffs test: some tests failed
```

# Background

Decoding untrusted data, such as images downloaded from across the web, have a
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

In comparison, in Wuffs, all bounds checks and arithmetic overflow checks
happen at compile time, with zero run time overhead.


# Getting Started

Wuffs code (that is proved safe via explicit assertions) is compiled to C code
(with those assertions removed) - it is transpiled. If you are a C/C++
programmer and just want to *use* the C edition of the Wuffs standard library,
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
modules. For example, the PNG codec (TODO) requires the deflate codec, but they
are separate files, since HTTP can use also deflate compression (also known as
gzip or zlib, roughly speaking) without necessarily processing PNG images.


## Getting Deeper

If you want to modify the Wuffs standard library, or compile your own Wuffs
code, you will need to do a little more work, and will have to install at least
the Go toolchain in order to build the Wuffs tools. To run the test suite, you
might also have to install C compilers like clang and gcc, as well as C
libraries (and their .h files) like libjpeg and libpng, as some tests compare
that Wuffs produces exactly the same output as these other libraries.

Running `go get -v github.com/google/wuffs/cmd/...` will download and install
the Wuffs tools. Change `get` to `install` to re-install those programs without
downloading, e.g. after you've modified their source code, or after a manually
issued `git pull`. The Wuffs tools that you'll most often use are `wuffsfmt`
(analogous to `clang-format`, `gofmt` or `rustfmt`) and `wuffs` (roughly
analogous to `make`, `go` or `cargo`).

You should now be able to run `wuffs test`. If all goes well, you should see
some output containing the word "PASS" multiple times.


## Poking Around

Feel free to edit the `std/lzw/decode_lzw.wuffs` file, which implements the GIF
LZW decoder. After editing, run `wuffs gen std/gif` or `wuffs test std/gif` to
re-generate the C edition of the Wuffs standard library's GIF codec, and
optionally run its tests.

Try deleting an assert statement and re-running `wuffs gen`. The result should
be syntactically valid, but a compile error, as some bounds checks can no
longer be proven.

Find the line `var bits u32`, which declares the bits variable and initializes
it to zero. Try adding `bits -= 1` on a new line of code after it. Again,
`wuffs gen` should fail, as the computation can underflow.

Similarly, replacing the line `var n_bits u32` with `var n_bits u32 = 10`
should fail, as an `n_bits < 8` assertion, a pre-condition, a few lines further
down again cannot be proven.

Similarly, changing the `4095` in `var prev_code u32[..= 4095]` either higher
or lower should fail.

Try adding `assert false` at various places, which should obviously fail, but
should also cause `wuffs gen` to print what facts the compiler can prove at
that point. This can be useful when debugging why Wuffs can't prove something
you think it should be able to.


## Running the Tests

If you've changed any of the tools (i.e. changed any `.go` code), re-run `go
install -v github.com/google/wuffs/cmd/...` and `go test
github.com/google/wuffs/lang/...`.

If you've changed any of the libraries (i.e. changed any `.wuffs` code), run
`wuffs test` or, ideally, `wuffs test -mimic` to also check that Wuffs' output
mimics (i.e. exactly matches) other libraries' output, such as giflib for GIF,
libpng for PNG, etc.

If your library change is an optimization, run `wuffs bench` or `wuffs bench
-mimic` both before and after your change to quantify the improvement. The
mimic benchmark numbers should't change if you're only changing `.wuffs` code,
but seeing zero change in those numbers is a sanity check on any unrelated
system variance, such as software updates or virus checkers running in the
background.


## Directory Layout

- `lang` holds the Go libraries that implement the Wuffs language: tokenizer,
  AST, parser, renderer, etc. The Wuffs tools are written in Go, but as
  mentioned above, Wuffs transpiles to C code, and Go is not necessarily
  involved if all you want is to use the C edition of Wuffs.
- `lib` holds other Go libraries, not specific to the Wuffs language per se.
- `internal` holds internal implementation details, as per Go's [internal
  packages](https://golang.org/s/go14internal) convention.
- `cmd` holds Wuffs' command line tools, also written in Go.
- `std` holds the Wuffs standard library's code. The initial focus is on
  popular image codecs: BMP, GIF, JPEG, PNG, TIFF and WEBP.
- `gen` holds the transpiled editions of that standard library. The initial
  focus is generating C code. Later on, the repository might include generated
  Go and Rust code.
- `release` holds the releases of the Wuffs standard library.
- `test` holds the regular tests for the Wuffs standard library.
- `fuzz` holds the fuzz tests for the Wuffs standard library.
- `script` holds miscellaneous utility programs.
- `doc` holds documentation.
- `example` holds example programs.

For a guide on how various things work together, the "99ff8e2 Let fields have
default values" commit is an example of adding new Wuffs syntax and threading
that all the way through to C code generation and testing.


# Documentation

- [Changelog](./doc/changelog.md)
- [Related Work](./doc/related-work.md)
- [Roadmap](./doc/roadmap.md)
- [Wuffs the Language](./doc/wuffs-the-language.md)
- Wuffs the Library (TODO)

Measurements:

- [Benchmarks](./doc/benchmarks.md)
- [Binary Size](./doc/binary-size.md)
- [Compatibility](./doc/compatibility.md)


# Status

Proof of concept. Version 0.1 at best. API and ABI aren't stabilized yet. There
are plenty of tests to create, docs to write and TODOs to do. The compiler
undoubtedly has bugs. Assertion checking needs more rigor, especially around
side effects and aliasing, and being sufficiently well specified to allow
alternative implementations. Lots of detail needs work, but the broad
brushstrokes are there.


# Discussion

The mailing list is at
[https://groups.google.com/forum/#!forum/wuffs](https://groups.google.com/forum/#!forum/wuffs).


# Contributing

The [CONTRIBUTING.md](./CONTRIBUTING.md) file contains instructions on how to
file the Contributor License Agreement before sending any pull requests (PRs).
Of course, if you're new to the project, it's usually best to discuss any
proposals and reach consensus before sending your first PR.


# License

Apache 2. See the LICENSE file for details.


# Disclaimer

This is not an official Google product, it is just code that happens to be
owned by Google.


---

Updated on October 2019.
