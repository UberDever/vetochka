# Introduction

## Why?

Usually, when you present a new language, they ask you why would you do that. My answer: multiple reasons.

Here they are, one by one:

1. I want to make *my* language to exercise my PLD and SWE skills
2. I like `C` programming language and its infrastructure
3. But I don't like `C` enough to use it raw; I need some kind of enhancement for it; I came to the conclusion that
`C` would benefit from entirely new frontend and meta compile-time capabilities
4. I found an elegant calculus that unified programming languages under a single `tree` notion and I wanted to
exercise it in the real programming language

TODO: readjust:
<!-- Thus, the idea for `Vetochka` was born: a new language, characterised by the following statements:

1. Syntax is very expressive and yet simple enough to analyze and construct
2. Reflection is built in the language as one of its main pillars: unified term representation and a rule for their introspection
3. Language can build itself without quotation, what `tree-calculus` refers to as *intensionality*
4. There's an executable subset of a language (called further `v0`), that allows to compute terms and inspect them, and
is used as a layer to build compilers/interpreters for other languages (called further `vf`)
5. Since the syntax is inert and yet expressive, and since there is a `v0` subset that allows to compute and inspect
arbitrary terms, it is possible to write any compiler in `v0` from this syntax `vf` to any other language
6. This, in turn, is used to write `vmeta` (general simple language for writing compilers/analyzers) and `vsystem` (more
expressive frontend to `C` that compiles directly to `C` and includes more semantic analysis work that your usual `C` compiler does) that would live as a single syntax and would be executed on the same `v0` machine -->

`Vetochka` has following distinct characteristics, that allow it to stand out among other lisp-flavored languages:
1. Unique unified syntax, built with tagged tree encoding in mind. This way, you could
build DSLs and interpreters as `Vetochka` programs, introducing any `meta` level you desire
2. Tagged tree encoding: everything in the language is a tagged tree (unless specified otherwise) which
    could represent values, functions and even machine-native primitives, like I/O operations. Every tag is also a term.
3. Syntax, Execution and Storage are separated: Syntax allows you to write tagged trees succinctly, Execution
    states that, from the point of the language, everything *consists of* (piths) and *built from* (construction) tagged trees,
    Storage allows to represent certain tagged trees as native machine values, simplifying reduction and reducing resource usage
4. Piths: everything in the language, including runtime values, is described by its *pith* -- a lazily built
    view (lens), describing essentials of this particular tagged tree and these are made
    as a dual to lowered syntax trees
5. Staged evaluation: `Vetochka` doesn't have quotation as a separate notion -- evaluation and substitution are separate `Vetochka`'s core  
    operations, in contrast to usual eager/lazy evaluation strategies of lambda calculus

## How this came to be

This language is not a new thing in itself. Rather, as they say, it stands on shoulders of giants. Nowadays, you
usually don't need to develop something completely new from scratch: there's no need nor capability for a particular
individual to do such a thing. That said, we still need to advance. We can't use stinky old tech from XX century.
There's a notion that defined this language direction from the ground up: [porosity](#source-porosity). My understanding is:
a language is bound to be unsuccessful if it doesn't interact with existing infrastructure. There are exceptions, but
usually you need to use giants' shoulders to have at least a chance of success. That said, I'm not strictly interested in
the success, rather, I'm trying to use things that were done right and proven to be useful multiple times.

Shoulders for this language:

- Tree/Triage calculus
- Lambda calculus
- C Programming Language
- ML
- Scheme

[task 20260529-103915](../../20260529-103915/TASK.md): explain each and every one, make a source link, link each to a particular vetochka level

If there is a single phrase that describes the core language spirit, it is this:

> As above, so below

\newpage
