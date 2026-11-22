# Multi-argument pattern matching: how other systems avoid case explosion

Source: agent recall, not fetched or rechecked against the papers. Treat each line as a pointer to verify.

## The problem as stated by the user

A lambda with N parameters, each split by shape (leaf, fork, fruit), has 3^N one-level cases.
Writing every case explicitly explodes. Deeper patterns multiply further.
Baseline answer: list only the needed cases; anything else is an error.

## Mechanisms that keep the written form small

1. Variables and wildcards. A variable pattern matches anything, so one row covers all remaining shapes for that argument.
2. Ordered first-match rows. ML, Haskell and Scheme `match` try clauses top-down. A later row implicitly means
   "and none of the rows above matched", so negative conditions are never written.
3. Match failure is an error, not a fall-through to unknown behavior. OCaml raises `Match_failure`; Haskell fails with
   a non-exhaustive-pattern error. Compilers warn statically (Maranget, "Warnings for pattern matching", JFP 2007:
   exhaustiveness and useless-clause checking over the clause matrix).
4. The compiler, not the user, expands the clause matrix into tests:
   - decision trees: each test happens once, but code size can blow up (Maranget, "Compiling pattern matching to good
     decision trees", ML Workshop 2008, with column-selection heuristics);
   - backtracking automata: code size stays linear in the patterns, but a test can repeat (Augustsson 1985;
     Le Fessant and Maranget, "Optimizing pattern matching", ICFP 2001).
   The explosion moves into generated code, where heuristics bound it.
5. Laziness constrains test order. In Haskell, matching forces arguments, so the order of tests changes termination
   (Wadler's chapter in Peyton Jones, "The Implementation of Functional Programming Languages", 1987).

## Pattern calculus line (Barry Jay, also the author of tree calculus)

- Pattern calculus (Jay, "Pattern Calculus: Computing with Functions and Structures", 2009; bondi language):
  cases `p -> s` are first-class functions. A failed match returns a distinguished failure, not an error.
  Extension `p -> s | r` tries `p` and passes the argument to `r` on failure.
  Alternatives compose as separate values, one case at a time.
- Multiple arguments are curried cases or one case over a compound pattern.
- Static and dynamic pattern calculi let patterns themselves be computed. Matching a compound uses
  factorization (SF-calculus), which tree calculus's triage descends from.
- Rewriting calculus / rho-calculus (Cirstea, Kirchner): abstraction over patterns, match failure as a stuck or failure
  term, with a structure operator for alternatives.

## Related surface mechanisms

- Views (Wadler 1987), active patterns (F#), pattern synonyms (GHC): user-defined match functions keep
  patterns abstract over representation.
- `case-lambda` (Scheme) dispatches on arity only, not shape.

## Properties specific to vetochka

- [fact] Triage inspects the encoded tree without forcing (20.11.26, 3); shape tests never change termination.
  Unlike Haskell, a matcher can test arguments in any order; only row order is observable.
- [fact] A fork pattern also matches unevaluated code: an application tree `[[{$}], f, x]` is a fork.
  Patterns match code shape, not values; a program forces explicitly before matching when it means values.
- [assumption] The research goal (README: the term system is both IR and metaprogramming substrate) favors keeping
  the authored clause matrix visible in the v0 tree, so compile-time code can check exhaustiveness on it.
  Lowering multi-argument patterns to nested triage keeps the core small but erases that structure.
