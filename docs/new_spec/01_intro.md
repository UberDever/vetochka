# Introduction

## Why?

Usually, when you present a new lanuguage, they ask you why would you do that. My answer: multiple reasons.

Here they are, one by one:

1. I want to make *my* language to exercise my PLD and SWE skills
2. I like `C` programming language and its infrastructure
3. But I don't like `C` enough to use it raw; I need some kind of enhancement for it; I came to the conclusion that
`C` would benefit from entirely new frontend and meta compile-time capabilities
4. I found an elegant calculus that unified programming languages under a single `tree` notion and I wanted to
exercise it in the real programming language

Thus, the idea for `Vetochka` was born: a new language, characterised by the following statements:

1. Syntax is very expressive and yet simple enough to analyze and construct
2. Reflecion is built in the language as one of its main pillars: unified term representation and a rule for their introspection
3. Language can build itself without quotation, what `tree-calculus` refers to as *intensionality*
4. There's an executable subset of a language (called futher `v0`), that allows to compute terms and inspect them, and
is used as a layer to build compilers/interpreters for other languages (called further `vf`)
5. Since the syntax is inert and yet expressive, and since there is a `v0` subset that allows to compute and inspect
arbitrary terms, it is possible to write any compiler in `v0` from this syntax `vf` to any other language
6. This, in turn, is used to write `vmeta` (general simple language for writing compilers/analyzers) and `vsystem` (more
expressive frontend to `C` that compiles directly to `C` and includes more semantic analysis work that your usual `C` compiler does) that would live as a single syntax and would be executed on the same `v0` machine

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

TODO: explain each and every one, make a source link, link each to a particular vetochka level

\newpage
