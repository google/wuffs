# Benchmarks

*Preliminary* `wuffs bench -mimic` summarized throughput numbers for various
codecs are below. Higher is better.

"Mimic" tests check that Wuffs' output mimics (i.e. exactly matches) other
libraries' output. "Mimic" benchmarks give the numbers for those other
libraries, as shipped with the OS. These were measured on a Debian Testing
system as of November 2017, which means these compiler versions:

- clang/llvm 5.0.0
- gcc 7.2.0

and these mimic library versions:

- libgif 5.1.4
- zlib 1.2.8

Unless otherwise stated, the numbers below were measured on an Intel x86_64
Broadwell, and were taken as of git commit "693af47 Rename some
bcheckOptimizeXxx methods."


## Reproducing

The benchmark programs aim to be runnable "out of the box" without any
configuration or installation. For example, to run the `std/zlib` benchmarks:

    git clone https://github.com/google/wuffs.git
    cd wuffs/test/c/std
    gcc -O3 zlib.c
    ./a.out -bench
    rm a.out

A comment near the top of that .c file says how to run the mimic benchmarks.

The output of those benchmark programs is compatible with the
[benchstat](https://godoc.org/golang.org/x/perf/cmd/benchstat) tool. For
example, that tool can calculate confidence intervals based on multiple
benchmark runs, or calculate p-values when comparing numbers before and after a
code change. To install it, first install Go, then run `go get
golang.org/x/perf/cmd/benchstat`.


## wuffs bench

As mentioned above, individual benchmark programs can be run manually. However,
the canonical way to run the benchmarks (across multiple compilers and multiple
packages like GIF and PNG) is to use the `wuffs` command line tool, as it will
also re-generate (transpile) the C code whenever you edit the \*.wuffs code.
Running `go install -v github.com/google/wuffs/cmd/...` will install the Wuffs
tools. After that, you can say

    wuffs bench

or

    wuffs bench -mimic std/flate

or

    wuffs bench -ccompilers=gcc -reps=3 -focus=Benchmarkwuffs_gif_lzw std/gif


## CPU Scaling

CPU power management can inject noise in benchmark times. On a Linux system,
power management can be controlled with:

    # Query.
    cpupower --cpu all frequency-info --policy
    # Turn on.
    sudo cpupower frequency-set --governor powersave
    # Turn off.
    sudo cpupower frequency-set --governor performance


# Deflate (including gzip and zlib)

The 1k, 10k, etc. numbers are approximately how many bytes there in the decoded
output.

    name                             speed

    wuffs_adler32_10k/clang          2.42GB/s ± 3%
    wuffs_adler32_100k/clang         2.42GB/s ± 3%
    wuffs_flate_decode_1k/clang       133MB/s ± 3%
    wuffs_flate_decode_10k/clang      199MB/s ± 3%
    wuffs_flate_decode_100k/clang     229MB/s ± 4%
    wuffs_zlib_decode_10k/clang       185MB/s ± 1%
    wuffs_zlib_decode_100k/clang      210MB/s ± 3%

    wuffs_adler32_10k/gcc            3.22GB/s ± 1%
    wuffs_adler32_100k/gcc           3.23GB/s ± 0%
    wuffs_flate_decode_1k/gcc         152MB/s ± 1%
    wuffs_flate_decode_10k/gcc        250MB/s ± 1%
    wuffs_flate_decode_100k/gcc       298MB/s ± 1%
    wuffs_zlib_decode_10k/gcc         238MB/s ± 2%
    wuffs_zlib_decode_100k/gcc        270MB/s ± 2%

    mimic_adler32_10k                3.00GB/s ± 1%
    mimic_adler32_100k               2.91GB/s ± 2%
    mimic_flate_decode_1k             211MB/s ± 1%
    mimic_flate_decode_10k            270MB/s ± 2%
    mimic_flate_decode_100k           285MB/s ± 1%
    mimic_zlib_decode_10k             250MB/s ± 2%
    mimic_zlib_decode_100k            294MB/s ± 2%


# GIF

The 1k, 10k, etc. numbers are approximately how many bytes of pixel data there
are in the decoded image. For example, the `test/data/harvesters.*` images are
1165 × 859 (approximately 1000k pixels) and a GIF image (a paletted image) is 1
byte per pixel.

    name                             speed

    wuffs_gif_decode_1k/clang         346MB/s ± 1%
    wuffs_gif_decode_10k/clang        137MB/s ± 0%
    wuffs_gif_decode_100k/clang       118MB/s ± 0%
    wuffs_gif_decode_1000k/clang      120MB/s ± 0%

    wuffs_gif_decode_1k/gcc           399MB/s ± 1%
    wuffs_gif_decode_10k/gcc          141MB/s ± 0%
    wuffs_gif_decode_100k/gcc         128MB/s ± 0%
    wuffs_gif_decode_1000k/gcc        131MB/s ± 0%

    mimic_gif_decode_1k               147MB/s ± 0%
    mimic_gif_decode_10k             90.7MB/s ± 0%
    mimic_gif_decode_100k            95.4MB/s ± 0%
    mimic_gif_decode_1000k           97.8MB/s ± 0%

TODO: investigate why gcc 4.8 (Ubuntu Trusty) seems to generate faster code
than gcc 7.2 (Debian Testing):

    wuffs_gif_decode_1k/gcc           411MB/s ± 2%
    wuffs_gif_decode_10k/gcc          162MB/s ± 0%
    wuffs_gif_decode_100k/gcc         138MB/s ± 0%
    wuffs_gif_decode_1000k/gcc        141MB/s ± 0%


---

Updated on November 2017.
