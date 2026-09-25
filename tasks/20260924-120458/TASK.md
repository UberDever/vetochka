# Bring the parser up to date with the spec

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, reducer
- EPIC: 20241220-144435

- (user, 2026-09-24) Syntax here means parsing: generating nodes. Inspection (Rule 3, CESK, piths) is a separate step and may stay open. Planned as a separate session.
- [fact] Gaps in `reducer/` against `docs/new_spec/02_syntax.md`:
  - `^` is still lexed as a delta node (`source_tokenize.c`), from before the nyad rename; nyads are `~[...]`.
  - `~` is still a unary operator; it is now the nyad sigil.
  - Operators come from a fixed unary list, not the ADR 1 adjacency classification.
  - The encoder emits a `:group` node (`bytecode_source.c`); rule 4 erases parens.
- Pending decisions that change generated nodes:
  - The `meta` slot: position, always-present list, chunked payload. Task 20260924-111836.
  - Applications as `[{@}, meta, ...]`. Task 20260209-162003.
  - A rewrite rule for literals.
- Decision (user, 2026-09-24): the encoder emits applications as `{@}` data, `[{@}, meta, f, x]`, not APPLY cells. Per 2026-08-30, APPLY exists only in machine control; parser output is data.
- Decision (user, 2026-09-24): `meta` is inert data, for inspection only.
- Decision (user, 2026-09-24): `meta` is an empty list by default; source position only when the caller of the parse routine asks for it.
- Decision (user, 2026-09-24): follow the spec's ASI rule: no exception after an annotation's `]`. Write annotations on the same line as their expression.
- Decision (user, 2026-09-24): drop the `{fn}` / OP_FN0 recognition from the encoder; `$fn:` superseded it.
- Done (agent, 2026-09-24): lexer, parser, formatter and encoder follow `docs/new_spec/02_syntax.md`. Token classes and lexer state live in `reducer/source_impl.h`. The encoder takes `bytecode_source_options_t` by value (user); `source_meta` fills `meta` with `[line: n, col: n]`.
- [fact] `#;` is trivia for ASI: a `;` caused only by the discarded expression is dropped.
- ~~[fact] `^` and `Δ` are lexing errors~~ superseded (user, 2026-09-24): `^` is an operator character (spec updated); ASCII-only identifiers are an implementation drawback, task 20260924-123410.
- [fact] Tests: 25 pass, also under `-Dsanitize=true`. Rejection tests cover ADR 1, R1, nyad arity, `$`, `@`, and an annotation on its own line.
- Known nit: the canonical formatter writes two spaces before a `...` line break (older behaviour).

- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
