# Current scheme: capture-avoiding substitution with private priming

Implemented as `SaturatingCasMachine` in `cas.py`; `python3 run_cas.py` runs the catalogue
(`cas-results.md`). All 37 cases pass, and the v0 compiler gives `{42}` and `{7}`.

## Rules

1. Identifier `[[{:id}], <name>, ~[{:private}, <base> <prime>]]`. The base is a run id plus a
   counter, issued by the parser or `unique`; the prime starts at 0. Identity is base and prime.
2. A name written in a declaration slot (header position, clause left side) declares that name.
   A computed declaration is built as data: the header or clause list arrives as a value.
3. Substitution is simultaneous per activation: a binder applies all clause values bound so far
   to each clause when it is forced and to its body; a lambda keeps its arguments aside (privately,
   like a CLOSURE environment) and substitutes them together at the last argument.
4. Shadowing: substitution stops at a lambda or binder that declares the identity substituted.
5. Capture avoidance: if a value entering a scope has a free identity that the scope declares,
   that declaration is primed (smallest free prime) in its scope first. This covers nested
   lambdas and binders, a binder's later clauses, and a lambda's parameters not yet bound.
6. A hole is bound when code is assembled around it within one activation; a value substituted
   into an already existing declaration keeps its holes free.
7. Unset names (proposed, implemented): a selected body that mentions a name its pattern left
   unset for the actual shape is an execution error.
8. Entering a body runs a binder however it is reached.

## The drawbacks, sorted

| # | drawback | kind | status |
|---|---|---|---|
| 1 | rewriting up to renaming, not literal | fundamental to capture avoidance | accepted |
| 2 | built vs substituted binds differently | fundamental, by design | accepted; "built" now means within one activation, so clause order and currying no longer matter |
| 3 | programs that bound holes by substitution | fundamental | accepted; the compiler is unaffected |
| 4 | computed declarations need data | my framing error: a written slot declares its name, as in any lambda calculus | not a drawback; data assembly now works through helpers too (E2, E3) |
| 5 | code-shaped data gets primed | my overstatement: priming is a renaming, bytes are private | not a drawback |
| 6 | comparing code up to renaming | not new: `unique` identities already differ per run | a library helper if needed |
| 7 | free identities per substitution | implementation cost | cache per tree; an environment machine pays it only when a value carrying holes enters a scope that declares them |
| — | sequential substitution leaked clause order and argument order (E2, E3) | my machine's bug | fixed by rule 3 |

## What rule 3 changes

Inspecting a partial application shows the lambda with updated private state but without its
bound arguments, which are kept aside as an environment machine keeps them. Before, the cases
showed the arguments substituted.

## Open

- Confirm rule 7 (unset names as an error).
- Confirm that partial applications hide their bound arguments (rule 3).
