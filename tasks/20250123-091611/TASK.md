# Specify and implement native operations and host integration

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode, host, spec
- KIND: TASK
- PARENT: 20260926-050130

- Work (agent): implement the native-operation protocol and the host services needed to run a v0 program from a file.
- Done when (agent): documented operations pass arithmetic, comparison, byte-string, protocol, and end-to-end hello-world tests.

## Native-operation contract

- Decision (user, spec): general opcode syntax is `$ns: {namespace} op: {opcode} <params>`. Core operations have shorthand forms.
- Decision (user, spec): each opcode controls whether and how its inputs are evaluated. Application curries inputs.
- Decision (user, spec): protocol states are `Continue(next-state, inputs)`, `Dispatch(result)`, and hard trap.
- Decision (user, 2026-07-11): protocol failures trap. Recoverable failures can use `[{:ok}, v]` or `[{:error}, d]` data.
- Decision (user, 2026-09-26): opcodes and special instructions are designator-headed lists, constructible from raw language forms.
- Decision (user, 2026-09-26): short-string recognition can optimize dispatch. Recognition remains evaluation semantics.
- Decision (user, 2026-08-30): some `$` forms are inert data and trap if executed.
- Decision (user, 2026-08-30): an in-flight opcode is a code closure. Its pith shows the selected opcode and inputs as written.
- Boundary (agent): 20260926-091318 owns the common value, tag, metadata, and pith representations. 20260529-103914 owns machine dispatch control.
- Scope (user+agent, 2026-09-26): initial native work includes i64 arithmetic and comparison, byte-string operations, and the recorded native tag-equality requirement.
- Open (agent): the remaining v0 native-operation set. Recheck tag equality against the representation review before implementation.

## Host boundary

- Direction (user): the host supplies source candidates. Bootstrap code decides how to load and run programs.
- Scope (user+agent, 2026-09-26): `load_file`, `parse_term`, print, and trap provide the initial host interface, informed by WASI.
- Direction (user): `parse_term` is pure, uses the complete shared syntax, and returns inert data. Reuse `public/source/`.
- Direction (user): native parsing is an implementation choice. No v0 capability machinery is required.
- Boundary (agent): 20260529-103914 owns `exec` and its driver contract. This task connects that entry to the host and bootstrap program.
- Open (agent, 2026-10-19): which host relations a reconstructed value preserves. The I/O discussion is exploratory, not approval to add effect handlers.
- Proposal (user, 2026-10-20): use `$[fn: [get, builtin]] do ... end` as the entrypoint shape for any bootstrapped program. The inputs provide a lookup function and a k-v namespace. This remains a proposal; reconcile it with other opcodes, their syntax, and the function parameter rules in 20241225-140425.

- History (agent): `archive/20261020-000000/records.md` preserves the original records. Its SUMMARY maps former subjects to their current owners.
