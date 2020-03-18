# Auto-Formatting

Within this repository:
- `C` code is formatted by `clang-format-5.0 -style=Chromium`.
- `Go` code is formatted by `gofmt`.
- `Wuffs` code is formatted by [`wuffsfmt`](/cmd/wuffsfmt).

Some C code has empty `//` line-comments, which look superfluous at first, but
force clang-format to break the line. This ensures one element per line (in a
long list) or having a function's name (not just its type) start a line. For
example, `grep ^wuffs_base__utf_8_ release/c/wuffs-unsupported-snapshot.c` (and
note the `^`) gives you a rough overview of Wuffs' UTF-8 related functions,
because of the forced line breaks:

```
size_t  //
wuffs_base__utf_8__encode(wuffs_base__slice_u8 dst, uint32_t code_point);
```
