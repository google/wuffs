# Glossary

#### Arithmetic

Addition `(x + y)`, multiplication `(x * y)`, etc.

#### Dependent Type

A type that depends on another value. For example, a variable `n`'s type might
be "the length of `s`", for some other
[slice](/doc/note/slices-arrays-and-tables.md)-typed variable `s`. Dependent
types are *a* way to implement [bounds checking](/doc/note/bounds-checking.md),
but they're not the only way. Wuffs does not use them.

#### Interval Arithmetic

See the [interval arithmetic](/doc/note/interval-arithmetic.md) note.

#### Modular Arithmetic

Arithmetic that wraps around at a certain modulus, such as `256` for the
`base.u8` type. For example, if `x` is a `base.u8` with value `200`, then `(x +
70)` has value `270` and would overflow, but `(x ~mod+ 70)` has value `14`.

#### Operator Precedence

A ranking of arithmetic operators so that an expression like `a + b * c` is
unambiguous (for the computer), equivalent to exactly one of `(a + b) * c` or,
more conventionally, `a + (b * c)`. While good for brevity, Wuffs does not have
operator precedence: the bare `a + b * c` is an invalid Wuffs expression and
the parentheses must be explicit. The ambiguity (for the human) can be a source
of bugs in other [security-concious file format
parsers](https://github.com/jbangert/nail/issues/7).

Some binary operators (`+`, `*`, `&`, `|`, `^`, `and`, `or`) are also
associative: `(a + b) + c` and `a + (b + c)` are equivalent, and can be written
in Wuffs as `a + b + c`.

#### Refinement Type

A basic type further constrained to a subset of its natural
[range](/doc/note/ranges-and-rects.md). For example, if `x` has type `base.u8[0
..= 99]` then it is an unsigned 8-bit integer whose value must be less than
`100`. Without the refinement, `x` could be as high as `255`.

`base.u8[0 ..= 99]` and `base.u32[0 ..= 99]` are two different types. They can
have different run-time representations.

Non-nullable pointer types can be thought of as a refinement of regular pointer
types, where the refined range excludes the `nullptr` value.

#### Saturating Arithmetic

Arithmetic that stops at certain bounds, such as `0` and `255` for the
`base.u8` type. For example, if `x` is a `base.u8` with value `200`, then `(x +
70)` has value `270` and would overflow, but `(x ~sat+ 70)` has value `255`.
