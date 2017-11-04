# Benchmarks

*Preliminary* `puffs bench -mimic` summarized throughput numbers for the GIF
codec are below. Higher is better.

"Mimic" tests check that Puffs' output mimics (i.e. exactly matches) other
libraries' output. "Mimic" benchmarks give the numbers for those other
libraries, as shipped with the OS, measured here on Ubunty 14.04 LTS "Trusty".

Unless otherwise stated, these benchmarks were run on an Intel x86_64
Broadwell.


## Reproducing

The benchmark programs aim to be runnable "out of the box" without any
configuration or installation. For example, to run the `std/flate` benchmarks:

    # TODO: confirm that we will live at github.com/google/puffs
    git clone https://github.com/google/puffs.git
    cd puffs/test/c/std
    gcc -O3 flate.c
    ./a.out -bench
    rm a.out

A comment near the top of that .c file says how to run the mimic benchmarks.

The output of those benchmark programs is compatible with the
[benchstat](https://godoc.org/golang.org/x/perf/cmd/benchstat) tool. For
example, that tool can calculate confidence intervals based on multiple
benchmark runs, or calculate p-values when comparing numbers before and after a
code change. To install it, first install Go, then run `go get
golang.org/x/perf/cmd/benchstat`.

## puffs bench

As mentioned above, individual benchmark programs can be run manually. However,
the canonical way to run the benchmarks (across multiple compilers and multiple
packages like GIF and PNG) is to use the `puffs` command line tool, as it will
also re-generate (transpile) the C code whenever you edit the \*.puffs code.
Running `go install -v github.com/google/puffs/cmd/...` will install the Puffs
tools. After that, you can say

    puffs bench

or

    puffs bench -mimic std/flate

or

    puffs bench -ccompilers=gcc -reps=3 -focus=Benchmarkpuffs_gif_lzw std/gif


# GIF

The 1k, 10k, etc. numbers are approximately how many bytes of pixel data there
is in the decoded image. For example, the `test/testdata/harvesters.*` images
are 1165 × 859 (approximately 1000k pixels) and a GIF image (a paletted image)
is 1 byte per pixel.

    name                             speed
    puffs_gif_decode_1k/clang         335MB/s ± 2%
    puffs_gif_decode_10k/clang        124MB/s ± 1%
    puffs_gif_decode_100k/clang       113MB/s ± 1%
    puffs_gif_decode_1000k/clang      115MB/s ± 1%

    mimic_gif_decode_1k/clang         154MB/s ± 1%
    mimic_gif_decode_10k/clang       91.1MB/s ± 0%
    mimic_gif_decode_100k/clang      97.1MB/s ± 1%
    mimic_gif_decode_1000k/clang     98.7MB/s ± 2%

    puffs_gif_decode_1k/gcc           395MB/s ± 1%
    puffs_gif_decode_10k/gcc          161MB/s ± 2%
    puffs_gif_decode_100k/gcc         134MB/s ± 2%
    puffs_gif_decode_1000k/gcc        138MB/s ± 1%

    mimic_gif_decode_1k/gcc           154MB/s ± 1%
    mimic_gif_decode_10k/gcc         90.8MB/s ± 2%
    mimic_gif_decode_100k/gcc        97.7MB/s ± 0%
    mimic_gif_decode_1000k/gcc       98.7MB/s ± 0%

The mimic numbers measure the pre-compiled library that shipped with the OS, so
it is unsurprising that they don't depend on the C compiler (clang or gcc) used
to run the test harness.


---

Updated on November 2017.
