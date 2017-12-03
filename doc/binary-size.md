# Binary Size

*Preliminary* measurements of `wuffs genlib` libraries' binary size on x86_64
are below. Lower is better.

TODO: re-do these numbers as we've spun std/zlib out of std/deflate.

    clang-dynamic:
    -rwxr-xr-x 1 nigeltao eng 38352 Nov  9 22:59 libwuffs.so
    -rw-r--r-- 1 nigeltao eng 22624 Nov  9 22:59 std-flate.lo
    -rw-r--r-- 1 nigeltao eng  9456 Nov  9 22:59 std-gif.lo

    clang-static:
    -rw-r----- 1 nigeltao eng 32966 Nov  9 22:59 libwuffs.a
    -rw-r--r-- 1 nigeltao eng 22648 Nov  9 22:59 std-flate.o
    -rw-r--r-- 1 nigeltao eng  9480 Nov  9 22:59 std-gif.o

    gcc-dynamic:
    -rwxr-xr-x 1 nigeltao eng 42504 Nov  9 22:59 libwuffs.so
    -rw-r--r-- 1 nigeltao eng 24776 Nov  9 22:59 std-flate.lo
    -rw-r--r-- 1 nigeltao eng 13520 Nov  9 22:59 std-gif.lo

    gcc-static:
    -rw-r----- 1 nigeltao eng 39102 Nov  9 22:59 libwuffs.a
    -rw-r--r-- 1 nigeltao eng 24728 Nov  9 22:59 std-flate.o
    -rw-r--r-- 1 nigeltao eng 13536 Nov  9 22:59 std-gif.o


## Comparison

Below are some standard C libraries shipped as part of Debian Testing as of
November 2017. The numbers aren't directly comparable, as these libraries have
a richer API, especially in providing an encoder and not just a decoder. Still,
it is a reference point for e.g. Wuffs (deflate + gzip + zlib) vs libz and
Wuffs gif vs libgif.

    dynamic:
    -rw-r--r-- 1 root root 105088 Jan 29  2017 /lib/x86_64-linux-gnu/libz.so.1.2.8
    -rw-r--r-- 1 root root  38816 Aug  1 16:06 /usr/lib/x86_64-linux-gnu/libgif.so.7.0.0

    static:
    -rw-r--r-- 1 root root 142810 Jan 29  2017 /usr/lib/x86_64-linux-gnu/libz.a
    -rw-r--r-- 1 root root  51216 Aug  1 16:06 /usr/lib/x86_64-linux-gnu/libgif.a


---

Updated on December 2017.
