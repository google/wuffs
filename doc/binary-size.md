# Binary Size

Binary size measurements of `wuffs genlib` libraries on x86\_64 are below.
Lower is better.

    clang8-dynamic:
    -rwxr-xr-x 1 tao tao 90488 Oct 19 05:34 libwuffs.so
    -rw-r--r-- 1 tao tao  7096 Oct 19 05:34 wuffs-base.lo
    -rw-r--r-- 1 tao tao  2192 Oct 19 05:34 wuffs-std-adler32.lo
    -rw-r--r-- 1 tao tao 19296 Oct 19 05:34 wuffs-std-crc32.lo
    -rw-r--r-- 1 tao tao 20568 Oct 19 05:34 wuffs-std-deflate.lo
    -rw-r--r-- 1 tao tao 26720 Oct 19 05:34 wuffs-std-gif.lo
    -rw-r--r-- 1 tao tao  6592 Oct 19 05:34 wuffs-std-gzip.lo
    -rw-r--r-- 1 tao tao  5808 Oct 19 05:34 wuffs-std-lzw.lo
    -rw-r--r-- 1 tao tao  6584 Oct 19 05:34 wuffs-std-zlib.lo

    clang8-static:
    -rw-r--r-- 1 tao tao 101560 Oct 19 05:34 libwuffs.a
    -rw-r--r-- 1 tao tao   7176 Oct 19 05:34 wuffs-base.o
    -rw-r--r-- 1 tao tao   2176 Oct 19 05:34 wuffs-std-adler32.o
    -rw-r--r-- 1 tao tao  20296 Oct 19 05:34 wuffs-std-crc32.o
    -rw-r--r-- 1 tao tao  20432 Oct 19 05:34 wuffs-std-deflate.o
    -rw-r--r-- 1 tao tao  26856 Oct 19 05:34 wuffs-std-gif.o
    -rw-r--r-- 1 tao tao   6840 Oct 19 05:34 wuffs-std-gzip.o
    -rw-r--r-- 1 tao tao   5776 Oct 19 05:34 wuffs-std-lzw.o
    -rw-r--r-- 1 tao tao   6608 Oct 19 05:34 wuffs-std-zlib.o

    gcc9-dynamic:
    -rwxr-xr-x 1 tao tao 94464 Oct 19 05:34 libwuffs.so
    -rw-r--r-- 1 tao tao  9784 Oct 19 05:34 wuffs-base.lo
    -rw-r--r-- 1 tao tao  2512 Oct 19 05:34 wuffs-std-adler32.lo
    -rw-r--r-- 1 tao tao 19568 Oct 19 05:34 wuffs-std-crc32.lo
    -rw-r--r-- 1 tao tao 20752 Oct 19 05:34 wuffs-std-deflate.lo
    -rw-r--r-- 1 tao tao 27936 Oct 19 05:34 wuffs-std-gif.lo
    -rw-r--r-- 1 tao tao  7232 Oct 19 05:34 wuffs-std-gzip.lo
    -rw-r--r-- 1 tao tao  6552 Oct 19 05:34 wuffs-std-lzw.lo
    -rw-r--r-- 1 tao tao  7360 Oct 19 05:34 wuffs-std-zlib.lo

    gcc9-static:
    -rw-r--r-- 1 tao tao 106120 Oct 19 05:34 libwuffs.a
    -rw-r--r-- 1 tao tao   9784 Oct 19 05:34 wuffs-base.o
    -rw-r--r-- 1 tao tao   2448 Oct 19 05:34 wuffs-std-adler32.o
    -rw-r--r-- 1 tao tao  19488 Oct 19 05:34 wuffs-std-crc32.o
    -rw-r--r-- 1 tao tao  20632 Oct 19 05:34 wuffs-std-deflate.o
    -rw-r--r-- 1 tao tao  27528 Oct 19 05:34 wuffs-std-gif.o
    -rw-r--r-- 1 tao tao   7144 Oct 19 05:34 wuffs-std-gzip.o
    -rw-r--r-- 1 tao tao   6448 Oct 19 05:34 wuffs-std-lzw.o
    -rw-r--r-- 1 tao tao   7248 Oct 19 05:34 wuffs-std-zlib.o


## Comparison

Below are some standard C libraries shipped as part of Debian Testing as of
October 2019. The numbers aren't directly comparable, as these libraries have a
richer API, especially in providing an encoder and not just a decoder. Still,
it is a reference point for e.g. Wuffs (adler32 + crc32 + deflate + gzip +
zlib) vs libz and Wuffs (gif + lzw) vs libgif.

    dynamic:
    -rw-r--r-- 1 root root 113088 Aug  6 16:44 /lib/x86_64-linux-gnu/libz.so.1.2.11
    -rw-r--r-- 1 root root  34656 Jun  5  2018 /usr/lib/x86_64-linux-gnu/libgif.so.7.0.0

    static:
    -rw-r--r-- 1 root root  50800 Jun  5  2018 /usr/lib/x86_64-linux-gnu/libgif.a
    -rw-r--r-- 1 root root 143250 Aug  6 16:44 /usr/lib/x86_64-linux-gnu/libz.a


---

Updated on October 2019.
