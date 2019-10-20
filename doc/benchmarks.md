# Benchmarks

`wuffs bench -mimic` summarized throughput numbers for various codecs are
below. Higher is better.

"Mimic" tests check that Wuffs' output mimics (i.e. exactly matches) other
libraries' output. "Mimic" benchmarks give the numbers for those other
libraries, as shipped with Debian. These were measured on a Debian Testing
system as of October 2019, which meant these compiler versions:

- clang/llvm 8.0.1
- gcc 9.2.1

and these "mimic" library versions, all written in C:

- libgif 5.1.4
- zlib 1.2.11

Unless otherwise stated, the numbers below were measured on an Intel x86\_64
Broadwell, and were taken as of Wuffs git commit ffdce5ef "Have bench-rust-gif
process animated / RGBA images".


## Reproducing

The benchmark programs aim to be runnable "out of the box" without any
configuration or installation. For example, to run the `std/zlib` benchmarks:

    git clone https://github.com/google/wuffs.git
    cd wuffs
    gcc -O3 test/c/std/zlib.c
    ./a.out -bench
    rm a.out

A comment near the top of that `.c` file says how to run the mimic benchmarks.

The output of those benchmark programs is compatible with the
[benchstat](https://godoc.org/golang.org/x/perf/cmd/benchstat) tool. For
example, that tool can calculate confidence intervals based on multiple
benchmark runs, or calculate p-values when comparing numbers before and after a
code change. To install it, first install Go, then run `go get
golang.org/x/perf/cmd/benchstat`.


## wuffs bench

As mentioned above, individual benchmark programs can be run manually. However,
the canonical way to run the benchmarks (across multiple compilers and multiple
packages like GIF and PNG) for Wuffs' standard library is to use the `wuffs`
command line tool, as it will also re-generate (transpile) the C code whenever
you edit the `std/*/*.wuffs` code. Running `go install -v
github.com/google/wuffs/cmd/...` will install the Wuffs tools. After that, you
can say

    wuffs bench

or

    wuffs bench -mimic std/deflate

or

    wuffs bench -ccompilers=gcc -reps=3 -focus=wuffs_gif_decode_20k std/gif


## Clang versus GCC

On some of the benchmarks below, clang performs noticably worse (e.g. 1.3x
slower) than gcc, on the same C code. A relatively simple reproduction was
filed as [LLVM bug 35567](https://bugs.llvm.org/show_bug.cgi?id=35567).


## CPU Scaling

CPU power management can inject noise in benchmark times. On a Linux system,
power management can be controlled with:

    # Query.
    cpupower --cpu all frequency-info --policy
    # Turn on.
    sudo cpupower frequency-set --governor powersave
    # Turn off.
    sudo cpupower frequency-set --governor performance


---

# Adler-32

The `1k`, `10k`, etc. numbers are approximately how many bytes are hashed.

    name                                             speed     vs_mimic

    wuffs_adler32_10k/clang8                         2.41GB/s  0.84x
    wuffs_adler32_100k/clang8                        2.42GB/s  0.84x

    wuffs_adler32_10k/gcc9                           3.24GB/s  1.13x
    wuffs_adler32_100k/gcc9                          3.24GB/s  1.12x

    mimic_adler32_10k                                2.87GB/s  1.00x
    mimic_adler32_100k                               2.90GB/s  1.00x


# CRC-32

The `1k`, `10k`, etc. numbers are approximately how many bytes are hashed.

    name                                             speed     vs_mimic

    wuffs_crc32_ieee_10k/clang8                      2.85GB/s  2.11x
    wuffs_crc32_ieee_100k/clang8                     2.87GB/s  2.13x

    wuffs_crc32_ieee_10k/gcc9                        3.38GB/s  2.50x
    wuffs_crc32_ieee_100k/gcc9                       3.40GB/s  2.52x

    mimic_crc32_ieee_10k                             1.35GB/s  1.00x
    mimic_crc32_ieee_100k                            1.35GB/s  1.00x


# Deflate

The `1k`, `10k`, etc. numbers are approximately how many bytes there in the
decoded output.

The `full_init` vs `part_init` suffixes are whether
[`WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED`](https://github.com/google/wuffs/blob/4080840928c0b05a80cda0d14ac2e2615f679f1a/internal/cgen/base/core-public.h#L83)
is unset or set.

    name                                             speed     vs_mimic

    wuffs_deflate_decode_1k_full_init/clang8          160MB/s  0.74x
    wuffs_deflate_decode_1k_part_init/clang8          199MB/s  0.92x
    wuffs_deflate_decode_10k_full_init/clang8         255MB/s  0.94x
    wuffs_deflate_decode_10k_part_init/clang8         263MB/s  0.97x
    wuffs_deflate_decode_100k_just_one_read/clang8    306MB/s  0.93x
    wuffs_deflate_decode_100k_many_big_reads/clang8   250MB/s  0.98x

    wuffs_deflate_decode_1k_full_init/gcc9            164MB/s  0.76x
    wuffs_deflate_decode_1k_part_init/gcc9            207MB/s  0.95x
    wuffs_deflate_decode_10k_full_init/gcc9           247MB/s  0.91x
    wuffs_deflate_decode_10k_part_init/gcc9           254MB/s  0.94x
    wuffs_deflate_decode_100k_just_one_read/gcc9      333MB/s  1.01x
    wuffs_deflate_decode_100k_many_big_reads/gcc9     261MB/s  1.02x

    mimic_deflate_decode_1k                           217MB/s  1.00x
    mimic_deflate_decode_10k                          270MB/s  1.00x
    mimic_deflate_decode_100k_just_one_read           329MB/s  1.00x
    mimic_deflate_decode_100k_many_big_reads          256MB/s  1.00x


## Deflate (C, miniz)

For comparison, here are [miniz](https://github.com/richgel999/miniz) 2.1.0's
numbers.

    name                                             speed     vs_mimic

    miniz_deflate_decode_1k/clang8                    174MB/s  0.80x
    miniz_deflate_decode_10k/clang8                   245MB/s  0.91x
    miniz_deflate_decode_100k_just_one_read/clang8    309MB/s  0.94x

    miniz_deflate_decode_1k/gcc9                      158MB/s  0.73x
    miniz_deflate_decode_10k/gcc9                     221MB/s  0.82x
    miniz_deflate_decode_100k_just_one_read/gcc9      250MB/s  0.76x

To reproduce these numbers, look in `test/c/mimiclib/deflate-gzip-zlib.c`.


## Deflate (Go)

For comparison, here are Go 1.12.10's numbers, using Go's standard library's
`compress/flate` package.

    name                                             speed     vs_mimic

    go_deflate_decode_1k                             45.4MB/s  0.21x
    go_deflate_decode_10k                            82.5MB/s  0.31x
    go_deflate_decode_100k                           94.0MB/s  0.29x

To reproduce these numbers:

    git clone https://github.com/google/wuffs.git
    cd wuffs/script/bench-go-deflate/
    go run main.go


## Deflate (Rust)

For comparison, here are Rust 1.37.0's numbers, using the
[alexcrichton/flate2-rs](https://github.com/alexcrichton/flate2-rs) and
[Frommi/miniz_oxide](https://github.com/Frommi/miniz_oxide) crates, which [this
file](https://github.com/sile/libflate/blob/77a1004edf6518a0badab7ce8837bc5338ff9bc3/README.md#an-informal-benchmark)
suggests is the fastest pure-Rust Deflate decoder.

    name                                             speed     vs_mimic

    rust_deflate_decode_1k                            104MB/s  0.48x
    rust_deflate_decode_10k                           202MB/s  0.75x
    rust_deflate_decode_100k                          218MB/s  0.66x

To reproduce these numbers:

    git clone https://github.com/google/wuffs.git
    cd wuffs/script/bench-rust-deflate/
    cargo run --release


# GIF

The `1k`, `10k`, etc. numbers are approximately how many pixels there are in
the decoded image. For example, the `test/data/harvesters.*` images are 1165 ×
859, approximately 1000k pixels.

The `bgra` vs `indexed` suffixes are whether to decode to 4 bytes (BGRA or
RGBA) or 1 byte (a palette index) per pixel, even if the underlying file format
gives 1 byte per pixel.

The `full_init` vs `part_init` suffixes are whether
[`WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED`](https://github.com/google/wuffs/blob/4080840928c0b05a80cda0d14ac2e2615f679f1a/internal/cgen/base/core-public.h#L83)
is unset or set.

The libgif library doesn't export any API for decode-to-BGRA or decode-to-RGBA,
so there are no mimic numbers to compare to for the `bgra` suffix.

    name                                             speed     vs_mimic

    wuffs_gif_decode_1k_bw/clang8                     461MB/s  3.18x
    wuffs_gif_decode_1k_color_full_init/clang8        141MB/s  1.85x
    wuffs_gif_decode_1k_color_part_init/clang8        189MB/s  2.48x
    wuffs_gif_decode_10k_bgra/clang8                  743MB/s  n/a
    wuffs_gif_decode_10k_indexed/clang8               200MB/s  2.11x
    wuffs_gif_decode_20k/clang8                       245MB/s  2.50x
    wuffs_gif_decode_100k_artificial/clang8           531MB/s  3.43x
    wuffs_gif_decode_100k_realistic/clang8            218MB/s  2.27x
    wuffs_gif_decode_1000k_full_init/clang8           221MB/s  2.25x
    wuffs_gif_decode_1000k_part_init/clang8           221MB/s  2.25x
    wuffs_gif_decode_anim_screencap/clang8           1.07GB/s  6.01x

    wuffs_gif_decode_1k_bw/gcc9                       478MB/s  3.30x
    wuffs_gif_decode_1k_color_full_init/gcc9          148MB/s  1.94x
    wuffs_gif_decode_1k_color_part_init/gcc9          194MB/s  2.54x
    wuffs_gif_decode_10k_bgra/gcc9                    645MB/s  n/a
    wuffs_gif_decode_10k_indexed/gcc9                 203MB/s  2.14x
    wuffs_gif_decode_20k/gcc9                         244MB/s  2.49x
    wuffs_gif_decode_100k_artificial/gcc9             532MB/s  3.43x
    wuffs_gif_decode_100k_realistic/gcc9              214MB/s  2.23x
    wuffs_gif_decode_1000k_full_init/gcc9             217MB/s  2.21x
    wuffs_gif_decode_1000k_part_init/gcc9             218MB/s  2.22x
    wuffs_gif_decode_anim_screencap/gcc9             1.11GB/s  6.24x

    mimic_gif_decode_1k_bw                            145MB/s  1.00x
    mimic_gif_decode_1k_color                        76.3MB/s  1.00x
    mimic_gif_decode_10k_indexed                     94.9MB/s  1.00x
    mimic_gif_decode_20k                             98.1MB/s  1.00x
    mimic_gif_decode_100k_artificial                  155MB/s  1.00x
    mimic_gif_decode_100k_realistic                  96.1MB/s  1.00x
    mimic_gif_decode_1000k                           98.4MB/s  1.00x
    mimic_gif_decode_anim_screencap                   178MB/s  1.00x


## GIF (Go)

For comparison, here are Go 1.12.10's numbers, using Go's standard library's
`image/gif` package.

    name                                             speed     vs_mimic

    go_gif_decode_1k_bw                               107MB/s  0.74x
    go_gif_decode_1k_color                           39.2MB/s  0.51x
    go_gif_decode_10k_bgra                            117MB/s  n/a
    go_gif_decode_10k_indexed                        57.8MB/s  0.61x
    go_gif_decode_20k                                67.2MB/s  0.69x
    go_gif_decode_100k_artificial                     151MB/s  0.97x
    go_gif_decode_100k_realistic                     67.2MB/s  0.70x
    go_gif_decode_1000k                              68.1MB/s  0.69x
    go_gif_decode_anim_screencap                      206MB/s  1.16x

To reproduce these numbers:

    git clone https://github.com/google/wuffs.git
    cd wuffs/script/bench-go-gif/
    go run main.go


## GIF (Rust)

For comparison, here are Rust 1.37.0's numbers, using the
[image-rs/image-gif](https://github.com/image-rs/image-gif) crate, easily the
top `crates.io` result for ["gif"](https://crates.io/search?q=gif).

    name                                             speed     vs_mimic

    rust_gif_decode_1k_bw                            89.2MB/s  0.62x
    rust_gif_decode_1k_color                         20.7MB/s  0.27x
    rust_gif_decode_10k_bgra                         74.5MB/s  n/a
    rust_gif_decode_10k_indexed                      20.4MB/s  0.21x
    rust_gif_decode_20k                              28.9MB/s  0.29x
    rust_gif_decode_100k_artificial                  79.1MB/s  0.51x
    rust_gif_decode_100k_realistic                   27.9MB/s  0.29x
    rust_gif_decode_1000k                            27.9MB/s  0.28x
    rust_gif_decode_anim_screencap                    144MB/s  0.81x

To reproduce these numbers:

    git clone https://github.com/google/wuffs.git
    cd wuffs/script/bench-rust-gif/
    cargo run --release


# Gzip (Deflate + CRC-32)

The `1k`, `10k`, etc. numbers are approximately how many bytes there in the
decoded output.

    name                                             speed     vs_mimic

    wuffs_gzip_decode_10k/clang8                      238MB/s  1.05x
    wuffs_gzip_decode_100k/clang8                     273MB/s  1.03x

    wuffs_gzip_decode_10k/gcc9                        239MB/s  1.06x
    wuffs_gzip_decode_100k/gcc9                       297MB/s  1.12x

    mimic_gzip_decode_10k                             226MB/s  1.00x
    mimic_gzip_decode_100k                            265MB/s  1.00x


# LZW

The `1k`, `10k`, etc. numbers are approximately how many bytes there in the
decoded output.

The libgif library doesn't export its LZW decoder in its API, so there are no
mimic numbers to compare to.

    name                                             speed     vs_mimic

    wuffs_lzw_decode_20k/clang8                       263MB/s  n/a
    wuffs_lzw_decode_100k/clang8                      438MB/s  n/a

    wuffs_lzw_decode_20k/gcc9                         266MB/s  n/a
    wuffs_lzw_decode_100k/gcc9                        450MB/s  n/a


# Zlib (Deflate + Adler-32)

The `1k`, `10k`, etc. numbers are approximately how many bytes there in the
decoded output.

    name                                             speed     vs_mimic

    wuffs_zlib_decode_10k/clang8                      237MB/s  0.96x
    wuffs_zlib_decode_100k/clang8                     272MB/s  0.92x

    wuffs_zlib_decode_10k/gcc9                        242MB/s  0.98x
    wuffs_zlib_decode_100k/gcc9                       294MB/s  0.99x

    mimic_zlib_decode_10k                             247MB/s  1.00x
    mimic_zlib_decode_100k                            296MB/s  1.00x


---

Updated on October 2019.
