This directory contains multiple programs to fuzz Wuffs' implementations of
various codecs. For example, `gif_fuzzer.cc` is a program to fuzz Wuffs' GIF
implementation.

They are typically run indirectly, by a fuzzing framework such as
[OSS-Fuzz](https://github.com/google/oss-fuzz). There is an [OSS-Fuzz pull
request](https://github.com/google/oss-fuzz/pull/1172) to add Wuffs to its list
of projects.

When working on these files, it is possible to run them directly on an explicit
test suite, in order to speed up the edit-compile-run cycle. Look for
`WUFFS_CONFIG__FUZZLIB_MAIN` for more details.
