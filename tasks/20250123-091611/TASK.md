# Opcode and value tagging representation

- STATUS: OPEN
- PRIORITY: 100
- TAGS: epic, opcode

- (user, spec) Opcodes are machine-native operations selected by `$`. General shape: `$ns: {namespace} op: {opcode} <params>`.
- (user, spec) Each opcode decides whether and how to evaluate each input. Opcodes are curried by application.
- (user, spec) Opcode states: `Continue(next-state, inputs)`, `Dispatch(result)`, hard trap.
- (user, 2026-07-09) Opcodes are shape-compatible node families, not delta encodings.
- (user, 2026-07-11) Each transition state is its own node type. An unsaturated opcode shows as a stem under Rule 3. The saturating application dispatches at once.
- (user, 2026-07-11) Protocol failures are uncatchable hard traps. Recoverable errors are `[{:ok}, v] | [{:error}, d]` data.
- (user, 2026-08-30) An in-flight opcode value is a code closure. Its pith shows the selected opcode and inputs as written.
- (user, 2026-08-30) Some `$` forms are inert data; executing them traps.
- Open: opcode piths, task 20260805-162238.
- History: items.md.

## Tasks

- [x] 20260201-183607 Inspect natives and opcodes from the calculus
- [ ] 20260209-162003 Tag structure for node kinds
- [ ] 20260924-111836 K-V lists as a language concept
