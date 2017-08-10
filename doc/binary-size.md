# Binary Size

*Preliminary* measurements of `puffs genlib` libraries' binary size, on x86_64
are below. Lower is better.

    clang-dynamic:
    -rwxr-x--- 1 nigeltao eng 35235 Aug 10 14:59 libpuffs.so
    -rw-r----- 1 nigeltao eng 18592 Aug 10 14:59 std-flate.lo
    -rw-r----- 1 nigeltao eng 10080 Aug 10 14:59 std-gif.lo

    clang-static:
    -rw-r----- 1 nigeltao eng 29576 Aug 10 14:59 libpuffs.a
    -rw-r----- 1 nigeltao eng 18360 Aug 10 14:59 std-flate.o
    -rw-r----- 1 nigeltao eng  9968 Aug 10 14:59 std-gif.o

    gcc-dynamic:
    -rwxr-x--- 1 nigeltao eng 39210 Aug 10 14:59 libpuffs.so
    -rw-r----- 1 nigeltao eng 21448 Aug 10 14:59 std-flate.lo
    -rw-r----- 1 nigeltao eng 12000 Aug 10 14:59 std-gif.lo

    gcc-static:
    -rw-r----- 1 nigeltao eng 34152 Aug 10 14:59 libpuffs.a
    -rw-r----- 1 nigeltao eng 21200 Aug 10 14:59 std-flate.o
    -rw-r----- 1 nigeltao eng 11704 Aug 10 14:59 std-gif.o


## Comparison

Below are some standard C libraries shipped as part of Ubuntu 14.04 LTS
"Trusty". The numbers aren't directly comparable, as these libraries have a
richer API, especially in providing an encoder and not just a decoder. Still,
it is a reference point for e.g. Puffs flate vs libz and Puffs gif vs libgif.

    dynamic:
    -rw-r--r-- 1 root root 100728 May 13  2013 /lib/x86_64-linux-gnu/libz.so.1.2.8
    -rw-r--r-- 1 root root  36000 Dec 16  2013 /usr/lib/x86_64-linux-gnu/libgif.so.4.1.6

    static:
    -rw-r--r-- 1 root root 142346 May 13  2013 /usr/lib/x86_64-linux-gnu/libz.a
    -rw-r--r-- 1 root root  51850 Dec 16  2013 /usr/lib/x86_64-linux-gnu/libgif.a


---

Updated on August 2017.
