# Cells revision

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding
- KIND: INITIATIVE
- PARENT: 20260926-050129

- Goal (user, 2026-09-25): introduce an abstraction between cells (storage) and the in-language representation, mainly for piths.
- Storage and observation may differ; piths bridge them (user, 2026-09-24): see 20260805-162238.
- [fact] Current encoding: cells-encoding.md (from `reducer/cells_impl.h`, `cells_api.h`, `cells_node.c`).
- [fact] Cells still carry `OP_FN0..2` and `APPLY`, while the encoder no longer emits either: applications are `{@}` data since 2026-09-24.
- (user, 2026-09-25) Wire bytes from `0x80` leave room for about 42 families in three arities, more for single-arity tags. Today's tags separate storage layouts: nyad, i64, bytes, some opcodes. Direction: tie tags to the semantic objects behind the language instead; `APPLY` and some opcodes already are.
- Direction (user, 2026-09-25): as above, so below — executing requires an exactly shaped node. For example `{@} lhs: f rhs: x` rather than any list headed by `{@}`, so it pairs with the cell form `APPLY lhs: ref-to-f rhs: ref-or-inline-x`.
- (agent) Tension: 2026-08-30 put APPLY in machine control only and application in data as the `{@}` marker; the encoder emits `[{@}, meta, f, x]` lists (2026-09-24). An exact-shaped application node stored as `APPLY` would make data and control share one cell form, with `exec` as the only door. Needs a ruling.
- Next (user, 2026-09-25): categorize current runtime object kinds, grammar first, then execution rules. First pass: object-kinds.md.
- Decision (user, 2026-09-25): runtime objects are first-class, above all storable. Machine state (pending applications, TERM) is not a runtime object.
- ~~(agent) Open tension for the CESK rules: 2026-08-30 rules both "Rule 3 does not force; identifier dereference is the only indirection" and "a pending machine application in the operand is evaluated to expose its root shape". The exception only matters if operands can be pending when Rule 3 fires: yes under July's callee-first `RUN-APPLY`, no under branch-first evaluation as pinned the same day. Task 20260711-131257.~~ resolved by the 2026-09-25 ruling below.
- Rule 3 never evaluates (user, 2026-09-25): see 20260529-103914.
- ~~Open: what Rule 3 sees of an application depends on this task: a `[{@}, meta, f, x]` list is a list cell; an exact `APPLY` node is a fork of `f` and `x`.~~ corrected (user, 2026-09-25): an application, like any runtime node, may have its own pith, so storage and observation are separate choices.
- Open: the pith of an application (task 20260805-162238) and, separately, its storage (this task).
- Opcodes are designator-headed lists (user, 2026-09-26): see 20250123-091611.
- (agent) This answers the storage question for opcodes and `{@}`: lists with a designator head, not per-state cell tags. The exact-shape direction of 2026-09-25 now reads as: the designator list is the exact shape.
- Session notes: 2026-09-25.md, 2026-09-26.md.
