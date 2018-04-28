# Wuffs the Language

Wuffs is an imperative, C-like language and much of it should look familiar to
somebody versed in any one of C, C++, D, Go, Java, JavaScript, Objective-C,
Rust, Swift, etc. Accordingly, this section is not an exhaustive specification,
merely a rough guide of where Wuffs differs.

Like Go, [semi-colons can be omitted](https://golang.org/ref/spec#Semicolons),
and `wuffsfmt` will remove them. Similarly, the body of an `if` or `while` must
be enclosed by curly `{}`s. There is no 'dangling else' ambiguity.


## Keywords

7 keywords introduce top-level concepts:

- `const`
- `error`
- `func`
- `packageid`
- `struct`
- `suspension`
- `use`

2 keywords distinguish between public and private API:

- `pri`
- `pub`

8 keywords deal with control flow within a function:

- `break`
- `continue`
- `else`
- `if`
- `iterate`
- `return`
- `while`
- `yield`

5 keywords deal with assertions:

- `assert`
- `inv`
- `post`
- `pre`
- `via`

2 keywords deal with types:

- `nptr`
- `ptr`

1 keyword deals with local variables:

- `var`

TODO: categorize try, io\_bind. Also: and, or, not, as, ref, deref, false,
true, in, out, this, u8, u16, etc.


## Operators

There is no operator precedence. `a * b + c` is an invalid expression. You must
explicitly write either `(a * b) + c` or `a * (b + c)`.

Some binary operators (`+`, `*`, `&`, `|`, `^`, `and`, `or`) are also
associative: `(a + b) + c` and `a + (b + c)` are equivalent, and can be written
as `a + b + c`.

The logical operators, `&&` and `||` and `!` in C, are written as `and` and
`or` and `not` in Wuffs.

TODO: ignore-overflow ops, equivalent to Swift's `&+`.

Converting an expression `x` to the type `T` is written as `x as T`.


## Types

Types read from left to right: `ptr array [100] u32` is a pointer to a
100-element array of unsigned 32-bit integers. `ptr` here means a non-null
pointer. Use `nptr` for a nullable pointer type.

Integer types can also be refined: `var x u32[10..20]` defines a variable x
that is stored as 4 bytes (32 bits) and can be combined arithmetically (e.g.
added, compared) with other `u32`s, but whose value must be between 10 and 20
inclusive. The syntax is reminiscent of Pascal's subranges, but in Wuffs, a
subsequent assignment like `x = 21` or even `x = y + z` is a compile time error
unless the right hand side can be proven to be within range.

Types can also provide a default value, such as `u32[10..20] = 16`, especially
if zero is out of range. If not specified, the implicit default is zero.

Refinement bounds may be omitted, where the base integer type provides the
implicit bound. `var x u8[..5]` means that `x` is between 0 and 5. `var y
i8[-7..]` means that `y` is between -7 and +127. `var z u32[..]` is equivalent
to `var z u32`.

Refinement bounds must be constant expressions. `var x u32[..2+3]` is valid,
but `var x u32; var y u32[..x]` is not. Wuffs does not have dependent types.
Relationships such as `y <= x` are expressible as assertions (see below), but
not by the type system.


## Structs

Structs are a list of fields, enclosed in parentheses: `struct point(x i32, y
i32)`. The struct name may be followed by a question mark `?`, which means that
its methods may be coroutines. (See below).


## Functions

Function signatures read from left to right: `func max(x i32, y i32)(z i32)` is
a function that takes two `i32`s and returns one `i32`. Two pairs of
parentheses are required: a function that in other languages would return
`void` in Wuffs returns the empty struct `()`, also known as the unit type.

When calling a function, each argument must be named. It is `m = max(x:10,
y:20)` and not `m = max(10, 20)`.

The function name, such as `max`, may be followed by either an exclamation mark
`!` or a question mark `?` but not both. An exclamation mark means that the
function is impure, and may assign to things other than its local variables. A
question mark means that the function is impure and furthermore a coroutine: it
can return a (recoverable) suspension code, such as needing more input data, or
return a (fatal) error code, such as being invalid with respect to a file
format. If suspended, calling that function again will resume at the suspension
point, not necessarily at the top of the function body. If an error was
returned, calling that function again will return the same error.

Some functions are methods, with syntax `func foo.bar(etc)(etc)`, where `foo`
names a struct type and `bar` is the method name. Within the function body, an
implicit `this` argument will point to the receiving struct. Methods can also
be marked as impure or coroutines.


## Variables

Variable declarations are [hoisted like
JavaScript](https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/var),
so that all variables have function scope. Proofs are easier to work with, for
both humans and computers, when one variable can't shadow another variable with
the same name.


## Assertions

Assertions state a boolean typed, pure expression, such as `assert x >= y`,
that the compiler must prove. As a consequence, the compiler can then prove
that, if `x` and `y` have type `u32`, the expression `x - y` will not
underflow.

An assertion applies for a particular point in the program. Subsequent
assignments or impure function calls, such as `x = z` or `f!()`, can invalidate
previous assertions. See the "Facts" section below for more details.

Arithmetic inside assertions is performed in ideal integer math, working in the
integer ring ℤ. An expression like `x + y` in an assertion never overflows,
even if `x` and `y` have a realized (non-ideal) integer type like `u32`.

The `assert` keyword makes an assertion as a statement, and can occur inside a
block of code like any other statement, such as an assignment `x = y` or an
`if` statement. The `pre`, `post` and `inv` keywords are similar to `assert`, but
apply only to `func` bodies and `while` loops.

`pre` states a pre-condition: an assertion that must be proven on every
function call or on every entry to the loop. Loop entry means the initial
execution of the loop, plus every explicit `continue` statement that targets
that `while` loop, plus the implicit `continue` at the end of the loop. For
`while` loops, the pre-condition must be proven before the condition (the `x <
y` in `while x < y { etc }`) executes.

`post` states a post-condition: an assertion that must be proven on every
function return (whether by an explicit `return` statement or by an implicit
`return` at the end of the function body) or on every loop exit (whether by an
explicit `break` statement or by the implicit `break` when the condition
evaluates false).

For example, a `while` loop that contains no explicit `break` statements can
always claim the inverse of its condition as a post-condition: `while x < y,
post x >= y { etc }`.

`inv` states an invariant, which is simply an assertion that is both a
pre-condition and post-condition.

When a `func` or `while` has multiple assertions, they must be listed in `pre`,
`inv` and then `post` order.


## Facts

Static constraints are expressed in the type system and apply for the lifetime
of the variable, struct field or function argument. For example, `var x u32`
and `var p ptr u32` constrain `x` to be non-negative and `p` to be non-null.
Furthermore, `var y u32[..20]` constrains `y` to be less than or equal to `20`.

Dynamic constraints, also known as facts, are previously seen assertions,
explicit or implicit, that are known to hold at a particular point in the
program. The set of known facts can grow and shrink over the analysis of a
program's statements. For example, the assignment `x = 7` can both add the
implicit assertion `x == 7` to the set of known facts, as well as remove any
other facts that mention the variable `x`, as `x`'s value might have changed.
TODO: define exactly when facts are dropped or updated.

Similarly, the sequence `assert x < y; z = 3` would result in set of known
facts that include both the explicit assertion `x < y` and the implicit
assertion `z == 3`, if none of `x`, `y` and `z` alias another (e.g. they are
all local variables).

Wuffs has two forms of non-sequential control flow: `if` branches (including
`if`, `else if`, `else if` chains) and `while` loops.

For an `if` statement, such as `if b { etc0 } else { etc1 }`, the condition `b`
is a known fact inside the if-true branch `etc0` and its inverse `not b` is a
known fact inside the if-false branch `etc1`. After that `if` statement, the
overall set of known facts is the intersection of the set of known facts after
each non-terminating branch. A terminating branch is a non-empty block of code
whose final statement is a `return`, `break`, `continue` or an `if`, `else if`
chain where the final `else` is present and all branches terminate. TODO: also
allow ending in `while true`?

For a `while` statement, such as `while b { etc }`, the set of known facts at
the start of the body `etc` is precisely the condition `b` plus all `pre` and
`inv` assertions. No other prior facts carry into the loop body, as the loop
can re-start coming from other points in the program (i.e. an explicit or
implicit `continue`) If the `while` loop makes no such `pre` or `inv`
assertions, no facts are known other than `b`.

Similarly, the set of known facts after the `while` loop exits is precisely its
`inv` and `post` assertions, and no other. In other words, a `while` loop must
explicitly list its state of the world just before and just after it executes.
This includes facts (i.e. invariants) about variables that are not mentioned at
all by the `while` condition or its body but are proven before the `while` loop
and assumed after it.


## Proofs

Wuffs' assertion and bounds checking system is a proof checker, not a full SMT
solver or automated theorem prover. It verifies explicit annotations instead of
the more open-ended task of searching for implicit proof steps. This involves
more explicit work by the programmer, but compile times matter, so Wuffs is
fast (and dumb) instead of smart (and slow).

Nonetheless, the Wuffs syntax is regular (and unlike C++, does not require a
symbol table to parse), so it should be straightforward to transform Wuffs code
to and from file formats used by more sophisticated proof engines.

Some rules are applied automatically by the proof checker. For example, if `x
<= 10` and `y <= 5` are both known true, whether by a static constraint (the
type system) or dynamic constraint (an asserted fact), then the checker knows
that the expression `x + y` is bounded above by `10 + 5` and therefore will not
overflow a `u8` (but would overflow a `u8[..12]`).

TODO: rigorously specify these automatic rules, when we have written more Wuffs
code and thus have more experience on what rules are needed to implement
multiple, real world image codecs.

Other rules are built in to the proof checker but are not applied automatically
(see "fast... instead of smart" above). Such rules have double-quote enclosed
names that look a little like mathematical statements. They are axiomatic, in
that these rules are assumed, not proved, by the Wuffs toolchain. They are
typically at a higher level than e.g. Peano axioms, as Wuffs emphasizes
practicality over theoretical minimalism. As they are axiomatic, they endeavour
to only encode 'obvious' mathematical rules. For example, the rule named `"a <
b: a < c; c <= b"` is one expression of transitivity: the assertion `a < b` is
proven if both `a < c` and `c <= b` are provable, for some expression `c`.
Terms like `a`, `b` and `c` here are all integers in ℤ, they do not encompass
floating point concepts like negative zero, `NaN`s or rounding.

Such rules are invoked by the `via` keyword. For example, `assert n_bits < 12
via "a < b: a < c; c <= b"(c:width)` makes the assertion `n_bits < 12` by
applying that transitivity rule, where `a` is `n_bits`, `b` is `12` and `c` is
`width`. The trailing `(c:width)` syntax is deliberately similar to a function
call (recall that when calling a function, each argument must be named), but
the `"a < b: a < c; c <= b"` named rule is not a function-typed expression.

TODO: specify these built-in `via` rules, again after more experience.


## Miscellaneous Language Notes

Labeled `break` and `continue` statements enable jumping out of loops that
aren't the most deeply nested. The syntax for labels is `while:label` instead
of Java's `label:while`, as the former is slightly easier to parse, and Wuffs
does not otherwise use labels for switch cases or goto targets.

TODO: describe the built in `buf1` and `buf2` types: 1- and 2-dimensional
buffers of bytes, such as an I/O stream or a table of pixel data.


---

Updated on April 2018.
