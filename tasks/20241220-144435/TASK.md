# Specify and implement shared syntax and lowering

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- KIND: TASK
- PARENT: 20260926-050128

- Work (agent): maintain one grammar, parser, formatter, and lowering path for source programs and program-generated syntax.
- Done when (agent): the open syntax choices in `items.md` are resolved, documented, and covered by parser, formatter, and encoder tests.

## Current basis

- Decision (user, spec): source is UTF-8. Comments use `;;`, nested `#| |#`, and `#;`. `...` continues a line.
- Decision (user, spec): byte strings use balanced `{...}` without escapes. Integers currently use decimal notation. `do` and `end` are reserved.
- Decision (user, 2026-08-30): ~~adjacency classifies operator runs~~ superseded by explicit grammar constraints (2026-10-20). Tight postfixes must touch their receiver. See `2026-10-20.md` and the historical `adr-1-operator-adjacency.md`.
- Decision (user, spec): newlines insert `;` after expression-ending tokens in layout-active contexts.
- Decision (user, 2026-08-30): applications have five surface forms: `f(args)`, `f[list]`, `f{bytes}`, labeled application, and `f do end`.
- Decision (user, spec): nyads use `~[]`, `~[x]`, and `~[x, y]`. Opcodes start with `$`. Annotations use `@[...]`.
- Decision (user, 2026-08-30): labeled expressions are first-class. The parser imposes no operator precedence. vf interprets mixed infix chains.
- Decision (user, spec): lowering produces inert terms with `{@}` for application. Grouping parentheses disappear.
- Decision (user, 2026-07-11): there is no language-level quotation.
- Decision (user, 2026-09-24): ASCII-only identifiers are an implementation limitation. The admitted Unicode characters remain open.
- Direction (user): number syntax should be extensible, like string syntax. The initial implementation needs i64.
- Boundary (agent): 20260926-091318 owns literal representations, tags, metadata, and k-v node shapes. This task implements their agreed lowering.
- Decision (user, 2026-10-20): `@[...] expression` is shorthand for the semantics of `tag`. Implement those semantics directly; do not lower the annotation to a call through the name `tag`. The tagging construction is tracked in 20260926-091318.

## Evidence and unresolved behavior

- Recorded result (agent, 2026-09-24): parser alignment landed in `0ed9c28`. This is historical evidence, not a fresh test result.
- Finding (agent, 2026-09-29): formatting `f(x,   y)` produces `f(x, y)\n`. Formatting that output adds `;` before the final newline.
- Open (user): whether canonical formatting must be idempotent. The existing test promises that the formatted result parses again.
- Rejected (user): the older curly-infix, `s{}` prefix, and parenthesized-statement designs. The archive retains their rationale and examples.

- History (agent): `archive/20261020-000000/records.md` preserves the original records. Its SUMMARY maps former subjects to their current owners.
