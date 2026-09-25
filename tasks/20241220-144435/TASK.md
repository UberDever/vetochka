# Surface syntax and grammar

- STATUS: OPEN
- PRIORITY: 100
- TAGS: epic, syntax

- (user, spec) Source is UTF-8. Comments are `;;`, nestable `#| |#`, and `#;` for a datum. `...` continues a line.
- (user, spec) Strings are `{...}` with balanced braces and no escapes. Integers are decimal. `$` and `~` are special tokens. `do` and `end` are reserved.
- (user, 2026-08-30) Operator runs are classified by adjacency (ADR 1). Tight postfixes must be glued (R1).
- (user, spec) A newline becomes `;` in layout-active contexts after an expression-ending token.
- (user, 2026-08-30) Five application shapes: `f(args)`, `f[list]`, `f{bytes}`, `f x: v`, `f do end`. Labeled expressions are first-class.
- (user, spec) Nyads are `~[]`, `~[x]`, `~[x, y]`. Opcodes are `$ labeled_expression`. Annotations are `@[...]`.
- (user, spec) Thirteen rewrite rules lower syntax to inert terms. `{@}` marks application. Parens are erased. Mixed infix chains are left to vf.
- (user, 2026-08-30) The parser stays operator-policy-free. v0 is arbitrary syntax plus a small executable core; vf decides meaning.
- (user, 2026-07-11) There is no language-level quote.
- History: items.md.

## Tasks

- [ ] 20260212-163430 Extensible number literals
- [x] 20260215-122343 Curly-infix expressions
- [x] 20260216-100019 Drop the s prefix from literals
- [x] 20260405-181501 Grammar document with EBNF
- [x] 20260601-084008 do-blocks as standalone expressions
- [x] 20260606-141936 Newline-based semicolon insertion
- [ ] 20260805-162238 Complete the node type and pith list
- [x] 20260831-155152 Paren-statements candidate
- [ ] 20260903-085028 Metadata list in lowered nodes
- [x] 20260924-092637 Lowering of mixed infix chains
- [x] 20260924-120458 Bring the parser up to date with the spec
- [ ] 20260924-123410 Unicode identifiers
