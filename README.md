[![Zig tests](https://github.com/UberDever/vetochka/actions/workflows/zig-test.yml/badge.svg)](https://github.com/UberDever/vetochka/actions/workflows/zig-test.yml)

# What is Vetochka?

`Vetochka` is an interpreted functional language and at the same time a new frontend for `C`.

It treats C as the operational substrate: the generated code uses the ordinary C compiler, ABI, debugger, libraries, and mental model. The language exists to add structure around C, not to replace it: checked modules, explicit dependencies, analyzable declarations, compile-time execution, code generation, and project-level conventions that can be verified mechanically.

The research question is whether a small reflective term system can serve as both the compiler’s internal representation and the user-facing metaprogramming substrate, while still producing boring, inspectable C.

# Why?

I always struggled to keep my C consistent, since I'm a somewhat sloppy person.
I know C guys develop their own style and hack stuff together, but I want to go to other
direction -- invent a set of conventions (mainly for a particular project) that could be
inforced by metaprogramming utility. At the same time, I realized that C would benefit from
new frontend that is explicitly designed for metaprogramming, analysis and flexibility.

My goals:
1. Keep C a usual boring explicit language 
2. Soften it's edges, close the room for certain types of errors, make scaling less painful, make code more manageable and increase its analysis potential 
3. Keep integration with existing C stuff: same compiler, same debugger, same ABI, just a different frontend 
4. Research the space for metaprogramming via a niche calculus and imperative system's language 
5. Write compiler and tooling code

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
