# Design the shared syntax and its lowering

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax
- KIND: INITIATIVE
- PARENT: 20260926-050128

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
