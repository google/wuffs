# Bounds Checking

For a slice or array `x`, an expression like `x[i .. j]` is invalid unless the
compiler can prove that `((0 <= i) and (i <= j) and (j <= x.length()))`. For
example, the expression `x[..]` is always valid because the implicit `i` and
`j` values are `0` and `x.length()`, and a slice or array cannot have negative
length.

Similarly, an expression like `x[i]` is invalid unless there is a compile-time
proof that `i` is in bounds: that `((0 <= i) and (i < x.length()))`. Proofs can
involve natural bounds (e.g. if `i` has type `base.u8` then `(0 <= i)` is
trivially true since a `base.u8` is unsigned),
[refinements](/doc/glossary.md#refinement-type) (e.g. if `i`has type
`base.u32[..= 100]` then 100 is an upper bound for `i`) and [interval
arithmetic](/doc/note/interval-arithmetic.md) (e.g. for an expression like
`x[m + n]`, the upper bound for `m + n` is the sum of the upper bounds of `m`
and `n`).

For example, if `a` is an `array[1024] base.u8`, and `expr` is some expression
of type `base.u32`, then `a[expr & 1023]` is valid, because the bitwise-and
means that the overall index expression is in the range `[0 ..= 1023]`,
regardless of `expr`'s range.

Proofs can also involve [facts](/doc/note/facts.md), dynamic constraints that
are not confined to the type system. For example, when the if-true branch of an
`if i < j { etc }` statement is taken, that `(i < j)` fact could combine with
another `(j <= x.length())` fact to deduce that `(i < x.length())` when inside
that if-true branch (but not necessarily outside of that if statement).

Wuffs does not have dependent types. Dependent types are one way to enforce
bounds checking, but they're not the only way, and there's more to programming
languages than their type systems.
