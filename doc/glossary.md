# Glossary

#### Arithmetic

Addition `(x + y)`, multiplication `(x * y)`, etc.

#### Interval Arithmetic

See the [interval arithmetic](/doc/note/interval-arithmetic.md) note.

#### Modular Arithmetic

Arithmetic that wraps around at a certain modulus, such as `256` for the
`base.u8` type. For example, if `x` is a `base.u8` with value `200`, then `(x +
70)` has value `270`, but `(x ~mod+ 70)` has value `14`.

#### Saturating Arithmetic

Arithmetic that stops at certain bounds, such as `0` and `255` for the
`base.u8` type. For example, if `x` is a `base.u8` with value `200`, then `(x +
70)` has value `270`, but `(x ~sat+ 70)` has value `255`.
