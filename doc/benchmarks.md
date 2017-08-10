# Benchmarks

*Preliminary* `puffs bench -mimic` summarized throughput numbers for the GIF
codec are below. Higher is better.

"Mimic" tests check that Puffs' output mimics (i.e. exactly matches) other
libraries' output. "Mimic" benchmarks give the numbers for those other
libraries, as shipped with the OS, measured here on Ubunty 14.04 LTS "Trusty".

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


---

Updated on August 2017.
