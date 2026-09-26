# Cells revision

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding
- EPIC: 20241225-125809

- Goal (user, 2026-09-25): introduce an abstraction between cells (storage) and the in-language representation, mainly for piths.
- (user, 2026-09-24) Storage and observation may differ; the pith is the bridge (hermetic principle). Every node is observable; an opaque node shows a minimal pith.
- [fact] Current encoding: cells-encoding.md (from `reducer/cells_impl.h`, `cells_api.h`, `cells_node.c`).
- [fact] Cells still carry `OP_FN0..2` and `APPLY`, while the encoder no longer emits either: applications are `{@}` data since 2026-09-24.
- Related: reword cells 20260924-100255, pith list 20260805-162238, tag structure 20260209-162003, exec 20260903-085029.
- (user, 2026-09-25) Wire bytes from `0x80` leave room for about 42 families in three arities, more for single-arity tags. Today's tags separate storage layouts: nyad, i64, bytes, some opcodes. Direction: tie tags to the semantic objects behind the language instead; `APPLY` and some opcodes already are.
- Direction (user, 2026-09-25): as above, so below — executing requires an exactly shaped node. For example `{@} lhs: f rhs: x` rather than any list headed by `{@}`, so it pairs with the cell form `APPLY lhs: ref-to-f rhs: ref-or-inline-x`.
- (agent) Tension: 2026-08-30 put APPLY in machine control only and application in data as the `{@}` marker; the encoder emits `[{@}, meta, f, x]` lists (2026-09-24). An exact-shaped application node stored as `APPLY` would make data and control share one cell form, with `exec` as the only door. Needs a ruling.
- Direction (user, 2026-09-25): piths are views into a node, a special reference with state. Rule 3 advances a pith, and it can be stored in a variable, keeping the view. Encoding (e.g. an i64) is secondary. (agent) This matches July LENS: inspection state, derived by Rule 3, unforgeable, non-callable.
- Next (user, 2026-09-25): categorize current runtime object kinds, grammar first, then execution rules. First pass: object-kinds.md.
- Decision (user, 2026-09-25): runtime objects are first-class, above all storable. Machine state (pending applications, TERM) is not a runtime object.
- ~~(agent) Open tension for the CESK rules: 2026-08-30 rules both "Rule 3 does not force; identifier dereference is the only indirection" and "a pending machine application in the operand is evaluated to expose its root shape". The exception only matters if operands can be pending when Rule 3 fires: yes under July's callee-first `RUN-APPLY`, no under branch-first evaluation as pinned the same day. Task 20260711-131257.~~ resolved by the 2026-09-25 ruling below.
- Decision (user, 2026-09-25): Rule 3 never evaluates. Applications persist and Rule 3 observes them like any node (arity 2); identifier dereference is its only indirection, which is how programs use binders' non-locality with Rule 3. To inspect a result rather than the computation, bind it with `$fn` and inspect the identifier. This departs from treecalcul.us, where an application cannot reach a Rule 3 branch, and the spec should say so.
- ~~Open: what Rule 3 sees of an application depends on this task: a `[{@}, meta, f, x]` list is a list cell; an exact `APPLY` node is a fork of `f` and `x`.~~ corrected (user, 2026-09-25): an application, like any runtime node, may have its own pith, so storage and observation are separate choices.
- Open: the pith of an application (task 20260805-162238) and, separately, its storage (this task).
- Decision (user, 2026-09-26): as above, so below means opcodes must be constructible from raw forms and recognizable when looked at; recognition is part of evaluation semantics.
- Decision (user, 2026-09-26): opcodes and other special instructions are lists whose first element is a special short string, a runtime object of about 8 bytes that the evaluator recognizes quickly. To the language it is just a string. The form matches the lowered syntax: a list headed by a designator. That keeps opcodes close to their pith forms, human-readable lists; pith and storage stay separate concepts but close.
- (agent) Consistent with the encoder's `[{@}, meta, f, x]` and with 2026-09-24: tags are designator strings, interning is a hidden speedup, general values are leaves.
- (agent) This answers the storage question for opcodes and `{@}`: lists with a designator head, not per-state cell tags. The exact-shape direction of 2026-09-25 now reads as: the designator list is the exact shape.
- Session notes: 2026-09-25.md, 2026-09-26.md.
