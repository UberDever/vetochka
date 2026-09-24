# Spec the let binding

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders, spec
- EPIC: 20241225-140425

- Decision (user, 2026-08-30): one form, `let[clauses] do ... end`. The loose block ends it.
- Clause kinds are infix operators: `=` binds, `<~` binds recursively, more can be added. Clauses lower to `[{:infix}, op, id, expr]`.
- Scoping is sequential. A list-shaped clause is a knot, like OCaml `and`, or `let rec ... and` with `<~`.
- `_ = expr` runs an effect. Blocks inside clauses need parens.
- [fact] The spec still shows the old `$let: x is:` sketch.
