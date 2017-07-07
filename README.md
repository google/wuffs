# Parsing Untrusted File Formats Safely

Puffs is a domain-specific language and library for parsing untrusted file
formats safely. Examples of such file formats include images, audio, video,
fonts and compressed archives.

Unlike the C programming language, Puffs is safe with respect to buffer
overflows, integer arithmetic overflows and null pointer dereferences. The key
difference between Puffs and other memory-safe languages is that all such
checks are done at compile time, not at run time. *If it compiles, it is safe*,
with respect to those three bug classes.

The aim is to produce software libraries that are as safe as Go or Rust,
roughly speaking, but as fast as C, and that can be used anywhere C libraries
are used. This includes very large C/C++ products, such as popular web browsers
and operating systems (using that term to include desktop and mobile user
interfaces, not just the kernel).

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


## What Does Puffs Code Look Like?

The [`std/gif/decode_lzw.puffs`](./std/gif/decode_lzw.puffs) file is a good
example. See the "Your First Edit" section below for more guidance.


## What Does Compile Time Checking Look Like?

For example, making this one-line edit to the GIF codec leads to a compile time
error. `puffs gen` fails to generate the C code, i.e. fails to compile
(transpile) the Puffs code to C code:

    $ git diff
    diff --git a/std/gif/decode_lzw.puffs b/std/gif/decode_lzw.puffs
    index f878c5e..f10dcee 100644
    --- a/std/gif/decode_lzw.puffs
    +++ b/std/gif/decode_lzw.puffs
    @@ -98,7 +98,7 @@ pub func lzw_decoder.decode?(dst ptr buf1, src ptr buf1, src_final bool)() {
                            in.dst.write?(x:s)

                            if use_save_code {
    -                               this.suffixes[save_code] = c as u8
    +                               this.suffixes[save_code] = (c + 1) as u8
                                    this.prefixes[save_code] = prev_code as u16
                            }

    $ puffs gen std/gif
    check: expression "(c + 1) as u8" bounds [1..256] is not within bounds [0..255] at
    /home/n/go/src/github.com/google/puffs/std/gif/decode_lzw.puffs:101. Facts:
        n_bits < 8
        c < 256
        this.stack[s] == (c as u8)
        use_save_code

In comparison, this two-line edit will compile (but the "does it decode GIF
correctly" tests then fail):

    $ git diff
    diff --git a/std/gif/decode_lzw.puffs b/std/gif/decode_lzw.puffs
    index f878c5e..b43443d 100644
    --- a/std/gif/decode_lzw.puffs
    +++ b/std/gif/decode_lzw.puffs
    @@ -97,8 +97,8 @@ pub func lzw_decoder.decode?(dst ptr buf1, src ptr buf1, src_final bool)() {
                            // type checking, bounds checking and code generation for it).
                            in.dst.write?(x:s)

    -                       if use_save_code {
    -                               this.suffixes[save_code] = c as u8
    +                       if use_save_code and (c < 200) {
    +                               this.suffixes[save_code] = (c + 1) as u8
                                    this.prefixes[save_code] = prev_code as u16
                            }

    $ puffs gen std/gif
    gen wrote:      /home/n/go/src/github.com/google/puffs/gen/c/gif.c
    gen unchanged:  /home/n/go/src/github.com/google/puffs/gen/h/gif.h
    $ puffs test std/gif
    gen unchanged:  /home/n/go/src/github.com/google/puffs/gen/c/gif.c
    gen unchanged:  /home/n/go/src/github.com/google/puffs/gen/h/gif.h
    test:           /home/n/go/src/github.com/google/puffs/test/c/gif
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

    puffs-test-c: some tests failed
    puffs test: some tests failed


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
- `doc` holds documentation.
- `example` holds example programs.

For a guide on how various things work together, the "99ff8e2 Let fields have
default values" commit is an example of adding new Puffs syntax and threading
that all the way through to C code generation and testing.


# Documentation

- [Puffs the Language](./doc/puffs-the-language.md)
- Puffs the Library (TODO)


# Related Work


## Existing Static Checkers

Puffs is a language by itself, integrated with a compiler, not something
embedded in the comments of another language's program, supported by a separate
program checker, as the bounds check elimination affects the generated code.

[Checked C](https://www.microsoft.com/en-us/research/project/checked-c/) has a
similar goal of safety, but aims to be backwards compatible with C, including
the idiomatic ways in which C code plays fast and loose with pointers and
arrays, and implicitly inserting run time checks while maintaining existing
data layout (e.g. the sizeof a struct type) for interoperability with other
systems. This is a much harder problem than Puffs' approach of a new, simple,
domain-specific programming language.

Extended Static Checker for Java (ESC/Java) and its successor
[OpenJML](http://www.openjml.org/), which obviously target the Java programming
language, similarly has to analyze a language that is more complicated that
Puffs. Java is also not commonly used for writing e.g. low level image codecs,
as the language lacks unsigned integer types, it is garbage collected and
idiomatic code often allocates.

[Spark ADA](http://libre.adacore.com/tools/spark-gpl-edition/) targets a subset
of the Ada programming language, which again is more complicated than Puffs. It
also allows for pluggable theorem provers such as Z3. This can increase
programmer productivity in that it can take less effort to prove more programs
safe, but it also affects reproducibility: when some programs that are provable
in one programmer's configuration are unprovable in another. It's also not
obvious up front what effect different theorem provers, and their different
heuristics, will have on compile times or proof times. In constrast, Puffs'
proof system is fast (and dumb) instead of smart (and slow).

We're not claiming that "use a state of the art theorem prover" is an
unworkable approach. It's just a different approach, with different trade-offs.


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


## Why Not Rust?

Puffs transpiles to standard C99 code with no dependencies, and should run
anywhere that has a C99 compliant C compiler. An existing C/C++ project, such
as the Chromium web browser, can choose to replace e.g. libpng with Puffs PNG,
without needing any additional toolchains. Sure, languages like D and Rust have
great binary-level interop with C code, and Mozilla are reporting progress with
parsing media formats in Rust, but it's still a non-zero operational hurdle to
grow a project's build process and to assess build times and binary sizes.

Again, we're not claiming that "write it in Rust" is an unworkable approach.
It's just a different approach, with different trade-offs.

[Nom](https://github.com/Geal/nom) is a
[promising](http://spw17.langsec.org/papers/chifflier-parsing-in-2017.pdf)
parser combinator library, in Rust. Puffs differs from nom by itself, in that
Puffs is an end to end implementation, not limited to that part of a file
format that is easily expressible as a formal grammar. In particular, it also
handles entropy encodings such as LZW (for GIF), ZLIB (for PNG, TODO) and
Huffman (for JPEG, TODO). Puffs differs from nom combined with other Rust code
(e.g. a Rust LZW implementation) in that bounds and overflow checks not just
ubiquitous but also completely eliminated at compile time. Once again, we're
not claiming that nom or Rust are unworkable, just different.


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

*Preliminary* `puffs bench` throughput numbers for the GIF codec are below.
Higher is better.

"Mimic" tests check that Puffs' output mimics (i.e. exactly matches) other
libraries' output, such as giflib for GIF, libpng for PNG, etc. "Mimic"
benchmarks give the numbers for those other libraries, as shipped with the OS,
measured here on Ubunty 14.04 LTS "Trusty".

The 1k, 10k, etc. numbers are approximately how many bytes of pixel data there
is in the decoded image. For example, the `test/testdata/harvesters.*` images
are 1165 × 859 (approximately 1000k pixels) and a GIF image (a paletted image)
is 1 byte per pixel.

    name                          speed
    gif_puffs_decode_1k/clang      389MB/s ± 2%
    gif_puffs_decode_10k/clang     137MB/s ± 0%
    gif_puffs_decode_100k/clang    121MB/s ± 0%
    gif_puffs_decode_1000k/clang   124MB/s ± 0%

    gif_mimic_decode_1k/clang      158MB/s ± 1%
    gif_mimic_decode_10k/clang    94.4MB/s ± 0%
    gif_mimic_decode_100k/clang    100MB/s ± 0%
    gif_mimic_decode_1000k/clang   102MB/s ± 0%

    gif_puffs_decode_1k/gcc        406MB/s ± 1%
    gif_puffs_decode_10k/gcc       158MB/s ± 0%
    gif_puffs_decode_100k/gcc      138MB/s ± 0%
    gif_puffs_decode_1000k/gcc     142MB/s ± 0%

    gif_mimic_decode_1k/gcc        158MB/s ± 0%
    gif_mimic_decode_10k/gcc      94.4MB/s ± 0%
    gif_mimic_decode_100k/gcc      100MB/s ± 0%
    gif_mimic_decode_1000k/gcc     102MB/s ± 0%

The mimic numbers measure the pre-compiled library that shipped with the OS, so
it is unsurprising that they don't depend on the C compiler (clang or gcc) used
to run the test harness.


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

Updated on July 2017.
