# Surface syntax and grammar: history

- 2024-12-20: grammar.md, the first shared parser note. Historical.
- 2026-02-12 (user): wisp-style syntax; `s{}` strings without escapes.
- 2026-02-15 (user): lisp plus wisp; `<prefix>{contents}` literals.
- 2026-02-16 (user): `s[]` list sugar.
- 2026-05-28 (user): algol-style statement syntax inspired by elixir; no keywords besides `end` and labels; no precedence.
- 2026-05-31 (user): labels, `~expr => :expr`.
- 2026-06-11 (user): newline and ASI implementation guidance, in slop_field.md in the machine epic.
- 2026-07-11 (user): shared syntax split from v0 and vf semantics.
- ~~2026-08-07 (user): explicit delta arity `^0`, `^1[x]`, `^2[x, y]`~~ superseded by nyads `~[...]`, 2026-08-30.
- 2026-08-30 (user): nyad rename, ADR 1, C6 and R1, block promotion, group erasure, `$` as a primary.
- Examples: wisp_syntax_example.md, vetochka_word_count_example.md, newer_syntax.tree.md, vetochka_collections_vec.tree.md, vetochka_uniform.tree.md.
- Dialogues: adr-1-operator-adjacency.md, prefix-infix-operators.md, labels-kv.md, Naming-a-node-nyad-2026-08-30.md.
