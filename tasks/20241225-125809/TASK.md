# Compact cell storage

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding
- KIND: INITIATIVE
- PARENT: 20260926-050130

- (user, spec) Cells store compact tree-shaped data and make up runtime memory. Every node is typed and has arity 0, 1 or 2.
- (user, 2026-07-11) One stable wire tag per runtime state. `NS1..NS4` and `AUX1..AUX4` are reserved. Bytes are allocated after states stabilize.
- (user, 2026-05-31) Applications are stored explicitly as apply nodes, to keep homoiconicity.
- [fact] (2026-07-10) The C parser builds a host `source_tree_t`; `bytecode_source_encode` then creates cells. The reducer is a flat loop dispatching by node arity. `OP_FN0/1/2` are an old experiment.
- Open: node types beyond nyads. See task 20260805-162238.
- History: items.md.
