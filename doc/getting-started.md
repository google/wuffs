# Getting Started

If you're looking to **write your own Wuffs code** outside of its standard
library, e.g. you want to safely decode your own custom file format, see the
[Wuffs the Language](/doc/wuffs-the-language.md) document and the
[/hello-wuffs-c](/hello-wuffs-c) example instead of this document.

If you're looking to just **use the Wuffs standard library**, e.g. you want to
safely decode some gzip'ed data in your C program, see the [Wuffs the
Library](/doc/wuffs-the-library.md) document and the [other examples](/example)
instead of this document.

If you're looking to **modify the Wuffs language**, it's probably best to ask
the [mailing list](https://groups.google.com/forum/#!forum/wuffs).

If you're looking to **modify the Wuffs standard library**, keep reading.

---

First, install the Go toolchain in order to build the Wuffs tools. To run the
test suite, you might also have to install C compilers like clang and gcc, as
well as C libraries (and their .h files) like libjpeg and libpng, as some tests
compare that Wuffs produces exactly the same output as these other libraries.

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

Find the `var bits : base.u32` line, which declares the bits variable,
implicitly initialized to zero. Try adding `bits -= 1` on a new line of code
after all of the `var` lines. Again, `wuffs gen` should fail, as the
computation can underflow.

Similarly, changing the `4095` in `var prev_code : base.u32[..= 4095]` either
higher or lower should fail.

Try adding `assert false` at various places, which should obviously fail, but
should also cause `wuffs gen` to print what facts the compiler knows at that
point. This can be useful when debugging why Wuffs can't prove something you
think it should be able to.


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
mimic benchmark numbers shouldn't change if you're only changing `.wuffs`
code, but seeing zero change in those numbers is a sanity check on any
unrelated system variance, such as software updates or virus checkers running
in the background.
