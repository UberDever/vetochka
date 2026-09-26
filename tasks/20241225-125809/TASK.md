# Cell encoding and reducer core

- STATUS: OPEN
- PRIORITY: 100
- TAGS: epic, encoding

- (user, spec) Cells store compact tree-shaped data and make up runtime memory. Every node is typed and has arity 0, 1 or 2.
- (user, 2026-07-11) One stable wire tag per runtime state. `NS1..NS4` and `AUX1..AUX4` are reserved. Bytes are allocated after states stabilize.
- (user, 2026-05-31) Applications are stored explicitly as apply nodes, to keep homoiconicity.
- [fact] (2026-07-10) The C parser builds a host `source_tree_t`; `bytecode_source_encode` then creates cells. The reducer is a flat loop dispatching by node arity. `OP_FN0/1/2` are an old experiment.
- Open: node types beyond nyads. See task 20260805-162238.
- History: items.md.

## Tasks

- [x] 20260201-183605 First cell node encoding
- [x] 20260201-183606 Bytecode VM with DP register and ENV stack
- [x] 20260201-183608 Tree as a byte stream with inline payload
- [x] 20260209-185016 Free list in cells for gc
- [x] 20260215-122341 Lexer and parser for textual bytecode in the interpreter
- [x] 20260215-122342 Bytecode dumping
- [x] 20260216-130542 Application by cell adjacency
- [x] 20260217-074816 Understand the triage calculus before adding semantics
- [x] 20260614-121150 Revisit byte encoding
- [x] 20260614-140231 Abstract opcode node creation in the source encoder
- [ ] 20260718-175927 Rule 3 dispatches on storage arity, wrong for APPLY
- [ ] 20260924-100255 Reword cells: runtime nodes with non-trivial piths
- [ ] 20260925-134730 Cells revision
