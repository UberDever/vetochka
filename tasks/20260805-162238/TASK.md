# Complete the node type and pith list

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- EPIC: 20241220-144435

- [fact] The spec lists only `nyad0..2` node types, each with its pith.
- (user, spec) A pith is the introspectable internals of a node, and it is dual: a pith built from scratch represents the same node.
- (user, 2026-08-30) Closure pith: declarators and body syntax, no env slot. Code closure pith: selected opcode and inputs as written.
- (user, 2026-08-30) LENS was renamed to pith.
- Open (agent, 2026-08-30): the pith of an un-executed `{@}` datum.
- Decision (user, 2026-09-24): every node is observable. A truly opaque node shows a minimal pith, for example a plain integer. Rule 3 never traps for lack of a pith.
- ~~(2026-07-11) a node whose kind has no pith traps at Rule 3 (`SHAPE-NO-LENS` in `docs/spec/03_v0.md`)~~ superseded by the ruling above. It only stood in for unfinished pith definitions.
- [fact] Since 2026-08-30 TERMs and machine frames are machine-internal, never data, so they never reach Rule 3.
- Open for the spec's Pith section (user writes): every node has a pith; a pith shows one layer and its children are piths; nyad-built terms are their own pith, primitive node types define theirs.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
