# General host interaction: opcodes, native bindings, opaque handles

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode
- KIND: INITIATIVE
- PARENT: 20260926-050130

- (user, spec) Opcodes are machine-native operations selected by `$`. General shape: `$ns: {namespace} op: {opcode} <params>`.
- (user, spec) Each opcode decides whether and how to evaluate each input. Opcodes are curried by application.
- (user, spec) Opcode states: `Continue(next-state, inputs)`, `Dispatch(result)`, hard trap.
- (user, 2026-07-09) Opcodes are shape-compatible node families, not delta encodings.
- ~~(user, 2026-07-11) Each transition state is its own node type. An unsaturated opcode shows as a stem under Rule 3. The saturating application dispatches at once.~~ superseded by the 2026-09-26 ruling: opcodes are designator-headed lists.
- (user, 2026-07-11) Protocol failures are uncatchable hard traps. Recoverable errors are `[{:ok}, v] | [{:error}, d]` data.
- (user, 2026-08-30) An in-flight opcode value is a code closure. Its pith shows the selected opcode and inputs as written.
- (user, 2026-08-30) Some `$` forms are inert data; executing them traps.
- Open: opcode piths, task 20260805-162238.
- History: items.md.
- Decision (user, 2026-09-26): as above, so below means opcodes must be constructible from raw forms and recognizable when looked at; recognition is part of evaluation semantics.
- Decision (user, 2026-09-26): opcodes and other special instructions are lists whose first element is a special short string, a runtime object of about 8 bytes that the evaluator recognizes quickly. To the language it is just a string. The form matches the lowered syntax: a list headed by a designator. That keeps opcodes close to their pith forms, human-readable lists; pith and storage stay separate concepts but close.
- (agent) Consistent with the encoder's `[{@}, meta, f, x]` and with 2026-09-24: tags are designator strings, interning is a hidden speedup, general values are leaves.
