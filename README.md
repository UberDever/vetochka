[![Zig tests](https://github.com/UberDever/vetochka/actions/workflows/zig-test.yml/badge.svg)](https://github.com/UberDever/vetochka/actions/workflows/zig-test.yml)

# What is Vetochka?

`Vetochka` is a interpreted functional language, based on `tree-calculus`, particularly on `triage-calculus`.

# Why?

This project exists for 3 reasons:
1. I want to learn more about PLD and do so from a functional purist standpoint
1. I want to develop a practical, system level language with pure core
1. I'm one of those people who wants to develop their own `lisp`

This is why there is a big project development log and little actual documentation. Below
are main points that describe project development across years. 

1. I read original papers and made myself acquainted with the book and notation
    * sometime in winter 2024
1. Then, I designed a language: grammar + internal representation + reduction rules (unchanged from triage calculus)
    * sometime in winter 2024 - spring 2025
1. After some time, I decided to redesign a language from ground up, since I didn't want
    to encode effects as purely tree terms; so I designed opcodes + intrinsics, and syntax
    * sometime in winter 2025
1. This session has been taking 3 or so months and then I arrived at conclusion (see the entry at `17.02.2026` in `PROJECT.md`) that I need to do a deep dive into calculus itself
    and leverage its expressiveness to my advantage

# Trying it

Currently, there are only tests written in zig. Language interpreter is also embeddable, so
current design provides C headers + dynamicaly linked library to use in different applications.

To run zig tests: `zig build test-all`. There are also sanitized tests, to run them do `zig build test-all -Dsanitize=true`. Note that in this case I expect `gcc` to be installed in the system (currently only gcc was tested).
