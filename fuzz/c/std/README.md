This directory contains multiple programs to fuzz Wuffs' implementations of
various codecs. For example, `gif_fuzzer.c` is a program to fuzz Wuffs' GIF
implementation.

They are typically run indirectly, by a fuzzing framework such as
[OSS-Fuzz](https://github.com/google/oss-fuzz). That repository's
`projects/wuffs` directory contains the complementary configuration for this
directory's code.

When working on these files, it is possible to run them directly on an explicit
test suite, in order to speed up the edit-compile-run cycle. Look for
`WUFFS_CONFIG__FUZZLIB_MAIN` for more details, and in `seed_corpora.txt` for
suggested test data.
