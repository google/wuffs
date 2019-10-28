# Slices, Arrays and Tables

A slice is a pointer and a (variable) length: a one-dimensional, contiguous
sequence of elements, all of some type `T`. For example, a `slice base.u8` is a
slice of bytes, where `T` is what the C programming language would call
`uint8_t`.

An array is a pointer and a (fixed) length. The difference is that an array's
length is part of the type: an `array[10] base.u8` has exactly 10 elements, and
is a different type than `array[20] base.u8`. For a slice-typed variable `s`
and an array-typed variable `a`, `s`'s length can change over time, but `a`'s
length cannot. An array's length can be a compile-time only concept, and does
not need an at-run-time representation.

Assigning to a slice-typed variable sets a pointer and a length, but does not
copy the elements. It has O(1) algorithmic complexity. Assigning to an
array-typed variable copies the elements. It has O(length) algorithmic
complexity.

Both `s[i .. j]` and `a[i .. j]` refer to slices, ranging from the `i`'th
element (inclusive) to the `j`'th element (exclusive). `i` can be omitted,
implicitly equalling zero. `j` can be omitted, implicitly equalling the length.
For example, `s[.. 5]` contains the first five elements of `s`.


## Tables

A table is the two-dimensional analog of the one-dimensional slice. It is a
pointer, width, height and stride, all of which can vary over the course of a
table-typed variable.

Separate width and stride fields allow for taking sub-tables of other tables.
The diagram below shows an outer table with a width, height and stride of 10,
3, 10, and an inner table (aliasing the same elements) with width, height and
stride of 4, 2, 10. Note that the two tables have the same stride, but the
inner table has a width smaller than the stride.

```
+-----------------------------+
|00 01 02 03 04 05 06 07 08 09|
|     +-----------+           |
|10 11|12 13 14 15|16 17 18 19|
|     |           |           |
|21 21|22 23 24 25|26 27 28 29|
+-----+-----------+-----------+
```

Lengths, widths, heights and strides are all measured in number of elements,
even when an element occupies multiple bytes.


## Bounds Checking

An expression like `x[i .. j]` is invalid unless the compiler can prove that
`((0 <= i) and (i <= j) and (j <= x.length()))`. For example, the expression
`x[..]` is always valid because the implicit `i` and `j` values are `0` and
`x.length()`, and a slice or array cannot have negative length.

Similarly, an expression like `x[i]` is invalid unless there is a compile-time
proof that `i` is in bounds: that `((0 <= i) and (i < x.length()))`. Proofs can
involve natural bounds (e.g. if `i` has type `base.u8` then `(0 <= i)` is
trivially true since a `base.u8` is unsigned), refinements (e.g. if `i`has type
`base.u32[..100]` then 100 is an upper bound for `i`) and [interval
arithmetic](/doc/note/interval-arithmetic.md) (e.g. for an expression like
`x[m + n]`, the upper bound for `m + n` is the sum of the upper bounds of `m`
and `n`).

For example, if `a` is an `array[1024] base.u8`, and `expr` is some expression
of type `base.u32`, then `a[expr & 1023]` is valid, because the bitwise-and
means that the overall index expression is in the range `[0..1023]`, regardless
of `expr`'s range.

Proofs can also involve [facts](/doc/note/facts.md), dynamic constraints that
are not confined to the type system. For example, when the if-true branch of an
`if i < j { etc }` statement is taken, that `(i < j)` fact could combine with
another `(j <= x.length())` fact to deduce that `(i < x.length())` when inside
that if-true branch (but not necessarily outside of that if statement).

Wuffs does not have dependent types. Dependent types are one way to enforce
bounds checking, but they're not the only way, and there's more to programming
languages than their type systems.
