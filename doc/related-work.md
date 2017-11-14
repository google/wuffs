# Related Work

Puffs is an industrial project, not an academic project, and this Related Work
section is not as extensive or as rigorous as would be found in an academic
thesis or journal article. The underlying goal is usefulness, not novelty. For
Puffs' users, it'd perfectly fine to have multiple implementations of a good
idea, even if the later ones wouldn't earn a Ph. D.

A number of other projects are listed below. Puffs is not exactly like any of
them, for one reason or another. We're not claiming that "use a state of the
art theorem prover" or "write it in Rust" are unworkable approaches. They're
just different approaches, with different trade-offs.

For example, Puffs is primarily concerned about *safety* and *performance*.
Formal *correctness*, such as being able to mathematically express that "when
this function returns, that array's elements will be sorted in increasing
order", is less of a concern for Puffs the Language than for other projects.
This is especially as higher level concerns like "correctly implements the JPEG
specification" are less amenable to formalization than foundational concepts
like searching and sorting.

For Puffs the Library, confidence about correctness is instead based on
testing, both unit testing and for e.g. media codecs, corpora of test files. A
small corpus is included in the Puffs source code repository. Other, much
larger corpora (e.g. based on a sample of a web crawl), are not included for
download size and copyright reasons.


## Static Checkers

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

Similarly, [Whiley](http://whiley.org/) is a programming language with extended
static checking. In contrast to Puffs, its `int` type uses unbounded
arithmetic, instead of e.g. 32-bit twos-complement representation. That can be
easier for theorem provers to reason about, but this can have a performance
impact when such integers are used in inner loops. In theory, a compiler could
recognize "an `int` restricted to the range [0, 99]" as a `uint8` instead of a
potentially 'big integer', but even so, it may improve performance to
explicitly use a `uint32` here even when a `uint8` would do.

[Spark ADA](http://libre.adacore.com/tools/spark-gpl-edition/) targets a subset
of the Ada programming language, which again is more complicated than Puffs. It
also allows for pluggable theorem provers such as Z3, similar to
[Dafny](http://research.microsoft.com/dafny). This can increase programmer
productivity in that it can take less effort to prove more programs safe, but
it also affects reproducibility (when some programs that are provable in one
programmer's configuration are unprovable in another) and the size of the
trusted computing base. It's also not obvious up front what effect different
theorem provers, and their different heuristics, will have on compile times or
proof times.

In constrast, Puffs' proof system is fast (and dumb) instead of smart (and
slow). For example, [Vale (Verified Assembly Language for
Everest)](https://github.com/project-everest/vale) is a promising approach that
uses Dafny to verify implementations of cryptographic algorithms. However,
["Vale: Verifying High-Performance Cryptographic Assembly
Code"](http://www.andrew.cmu.edu/user/bparno/papers/vale.pdf) reports in Table
1 that verification times are measured in *minutes*. Puffs aims for
*sub-second* compilation times, including proofs of safety.

Once again, we're not claiming that these other approaches are unworkable, or
not useful, just different, with different trade-offs.


## Why a New Language?

Simpler languages are easier to prove things about. Macros, inheritance,
closures, generics, operator overloading, goto's and built-in container types
are all useful features, but as mentioned in the top-level
[README](../README.md), Puffs is not a general purpose programming language.
Instead, its focus is on compile-time provable memory safety, with C-like
performance, for CPU-intensive file format decoders.

Nonetheless, Puffs is an imperative language, not a functional language,
despite the long history of functional languages and provability. Inner loop
performance usage matters, and it is easier to match the optimization
techniques of incumbent C libraries with a C-like language than with a
functional language. Compared to something like
[F\*](https://www.fstar-lang.org/), [Idris](https://www.idris-lang.org/) or
[Liquid Haskell](https://ucsd-progsys.github.io/liquidhaskell-blog/), Puffs has
no garbage collector overhead, as it has no garbage collector.

Memory usage also matters, again considering per-pixel costs can be multiplied
by millions of pixels. It is important to represent e.g. an RGBA pixel as four
u8 values (or one u32), not as four naturally-sized-for-the-CPU integers or
four 'big integers'.

[F\* / KreMLin](https://github.com/FStarLang/kremlin) is a subset of F\* that
is similar in some sense to Puffs, in that it transpiles to C and cares about
both safety and performance. Unlike Puffs, it is a functional language (with
dependent types), not an imperative one (with a simpler type system), and uses
a sophisticated theorem prover like Z3, with the same trade-offs as discussed
above for Spark ADA and Dafny. It also tackles a more complicated problem, in
attempting to prove correctness properties, not just safety properties. Further
reading is in ["Everest: Towards a Verified, Drop-in Replacement of
HTTPS"](https://project-everest.github.io/assets/snapl2017.pdf) and ["Verified
Low-Level Programming Embedded in F\*"](https://arxiv.org/pdf/1703.00053.pdf).

[Cryptol](https://github.com/GaloisInc/cryptol) is another project that, like
Everest (including F\* / KreMLin and its HACL\* sub-project, and Dafny / Vale),
focuses on cryptographic algorithms, rather than Puffs' focus on file formats,
and also relies on a sophisticated theorem prover like Z3.

Once again, we're not claiming that these other approaches are unworkable, or
not useful, just different, with different trade-offs.


## Why Not Rust?

Puffs transpiles to standard C99 code with no dependencies, and should run
anywhere that has a C99 compliant C compiler. An existing C/C++ project, such
as the Chromium web browser, can choose to replace e.g. libpng with Puffs PNG,
without needing any additional toolchains. Sure, languages like D and Rust have
great binary-level interop with C code, and Mozilla are reporting progress with
parsing media formats in Rust, but it's still an operational hurdle to grow a
project's build process and to assess build times and binary sizes.

[Nom](https://github.com/Geal/nom) is a parser combinator library in Rust,
described in ["Writing parsers like it is
2017"](http://spw17.langsec.org/papers/chifflier-parsing-in-2017.pdf). Puffs
differs from nom by itself, in that Puffs is an end to end implementation, not
limited to that part of a file format that is easily expressible as a formal
grammar. In particular, it also handles entropy encodings such as LZW (for
GIF), ZLIB (for PNG) and Huffman (for JPEG, TODO). Puffs differs from nom
combined with other Rust code (e.g. a Rust LZW implementation) in that bounds
and overflow checks not just ubiquitous but also completely eliminated at
compile time.

[Kaitai Struct](http://kaitai.io/) is in a similar space, generating safe
parsers for multiple target programming languages from one declarative
specification. Again, Puffs differs in that it is a complete (and performant)
end to end implementation, not just for the structured parts of a file format.
Repeating a point in the previous paragraph, the difficulty in decoding the GIF
format isn't in the regularly-expressible part of the format, it's in the LZW
compression. [Kaitai's GIF parser](http://formats.kaitai.io/gif/index.html)
returns the compressed LZW data as an opaque blob.

Once again, we're not claiming that these other approaches are unworkable, or
not useful, just different, with different trade-offs.


---

Updated on November 2017.
