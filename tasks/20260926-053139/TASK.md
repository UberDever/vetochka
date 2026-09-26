# Lowered form of literals

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- KIND: TASK
- PARENT: 20241220-144435

- Decision (user, 2026-09-26): to preserve "as above, so below", literals come in two tagged forms: a pure tree built from nyads, and a tree with an opaque payload. The two probably get different tags. Motivation: a string can be built out of nyads, and a string can be disassembled into nyads.
- Done when: the spec has a rewrite rule for integer and byte-string literals in both forms, and the encoder emits them.
- [fact] (agent) Today the encoder emits bare native leaves for literals (`bytecode_source.c`, `encode_token`).
- (agent) This revisits rulings from 2026-09-24, marked for sober review: general values are leaves (20260209-162003); byte strings carry no `meta` (20260903-085028). Pure-tree literals follow Jay's encoding (tree book 3.7, 4.7).
- Open (agent): the tag names; whether both forms carry `meta`; whether names and designators inside tagged nodes (`{x}` in `[{:id}, meta, {x}]`) stay bare leaves.
