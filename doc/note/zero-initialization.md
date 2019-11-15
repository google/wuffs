# Zero Initialization

Memory-safe programming languages typically initialize their variables and
fields to zero, so that there is no way to read an uninitialized value. By
default, Wuffs does so too, but in Wuffs' C/C++ form, there is the option to
leave some struct fields uninitialized (although local variables are always
zero-initialized), for performance reasons:

```
uint32_t flags = 0;
etc

// Setting this flag bit takes the option. Its presence or absence has no
// effect if the WUFFS_INITIALIZE__ALREADY_ZEROED flag bit is also set.
flags |= WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED;

wuffs_foo__bar__initialize(etc, flags);
```

With or without this flag bit set, the Wuffs compiler still enforces bounds and
arithmetic overflow checks. It's just that for potentially-uninitialized struct
fields, the compiler has weaker starting assumptions: their numeric types
cannot be
[refined](https://github.com/google/wuffs/blob/master/doc/glossary.md#refinement-type).

Even with this flag bit set, the Wuffs standard library also considers reading
from an uninitialized buffer to be a bug, and strives to never do so, but
unlike buffer out-of-bounds reads or writes, it is not a bug class that the
Wuffs compiler eliminates.

For those paranoid about security, leave this flag bit unset, so that
`wuffs_foo__bar__initialize` will zero-initialize the entire struct (unless the
`WUFFS_INITIALIZE__ALREADY_ZEROED` flag bit is also set).

Setting this flag bit (avoiding a fixed-size cost) gives a small absolute
improvement on micro-benchmarks, mostly noticable (in relative terms) only when
the actual work to do (the input) is also small. Look for
`WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED` in the
[benchmarks](https://github.com/google/wuffs/blob/master/doc/benchmarks.md) for
performance numbers.

In Wuffs code, a struct definition has two parts, although the second part's
`()` parentheses may be omitted if empty:

```
pub struct bar?(
    // Fields in the first part are always zero-initialized.
    x : etc,
    y : etc,
)(
    // Fields in the second part are optionally uninitialized, but are still
    // zero-initialized by default.
    //
    // Valid types for these fields are either unrefined numerical types, or
    // arrays of a valid type.
    //  - "base.u8" is ok.
    //  - "array[123] base.u8" is ok.
    //  - "array[123] base.u8[0 ..= 99]" is not, due to the refinement.
    z : etc,
)
```
