# Introduction

## Why?

Usually, when you present a new language, they ask you why would you do that. My answer: multiple reasons.

Here they are, one by one:

1. I want to make *my* language to exercise my PLD and SWE skills, especially working with calculi and
making the language practical
2. I like `C` programming language and its infrastructure
3. But I don't like `C` enough to use it raw; I need some kind of enhancement for it; I came to the conclusion that
`C` would benefit from entirely new frontend and meta compile-time capabilities

As usual, during the development, the language has turned into lisp, which is fine, since lisp brings
us closer to God.

`Vetochka` has following distinct characteristics, that allow it to stand out among other lisp-flavored languages:
1. Unique unified syntax, built with tagged tree encoding in mind. This way, you could
build DSLs and interpreters as `Vetochka` programs, introducing any `meta` level you desire
2. Tagged tree encoding: everything in the language is a tagged tree composed of primitive values. It allows you to represent different objects, functions and even machine-native primitives, like I/O operations.
3. Syntax, Execution and Storage are separated: Syntax allows you to write tagged trees succinctly, Execution
    states that, from the point of the language, everything *consists of* (piths) and *built from* (construction) tagged trees,
    Storage allows to represent certain tagged trees as native machine values, simplifying reduction and reducing resource usage
4. Piths: everything in the language, including runtime values, is described by its *pith* -- a lazily built
    view (lens), describing essentials of this particular value. It consists only of primitive values.
5. Staged evaluation: `Vetochka` doesn't have quotation as a separate notion -- evaluation and substitution are separate `Vetochka`'s core
    operations, in contrast to usual eager/lazy evaluation strategies of lambda calculus.

## How this came to be

TODO: add shoulders and inspirations for the language

If there is a single phrase that describes the core language spirit, it is this:

> As above, so below

\newpage

