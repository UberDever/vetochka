# Specify and implement values, observation, and reconstruction

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding, spec
- KIND: TASK
- PARENT: 20260926-050129

- Work (agent): define observable value structure, reconstruction, and its implementation through cells and piths.
- Done when (agent): the representation questions in `items.md` have explicit decisions, matching documentation, and implementation tests for the agreed contracts.
- Scope (user, 2026-09-26): begin with the runtime kinds in `object-kinds.md`. Boxes and further kinds remain later work.

## Observation and reconstruction

- Decision (user, spec): a pith exposes a node's internals. Constructing the same pith represents the same node.
- Qualification (user, 2026-08-30): binder boundaries have a re-quotation exception. 20241225-140425 owns that rule and its unresolved relation to reconstruction.
- Decision (user, 2026-09-24): every node has an observable pith. Rule 3 never traps merely because a kind lacks a pith.
- Direction for the spec (user): a pith exposes one layer, with piths as children. Nyad-built terms are their own pith.
- Decision (user, 2026-09-24): storage and observation may differ. Piths connect them and belong to runtime semantics, separately from node generation.
- Direction (user, 2026-09-25): a pith can be a storable view with state that Rule 3 advances.
- Open (agent): whether Rule 3 uses nodes directly or separate view objects. The earlier unforgeable, non-callable view description was an agent comparison.
- Decision (user, 2026-09-25): runtime objects are first-class and storable. Pending applications, TERM, and machine frames are internal machine state.
- Boundary (agent): 20260529-103914 owns Rule 3's evaluation behavior. 20250123-091611 owns the native-operation protocol.

## Tags, literals, and storage

- Decision (user, 2026-09-24): a colon-leading string in head position is a tag designator. Interning is a hidden optimization.
- Decision (user, 2026-09-24): tagged nodes refine basic structures. List operations can observe their heads and metadata.
- Decision (user, 2026-09-24): a proper list is recognized by walking its structure to `~[]`.
- Decision (user, 2026-09-24): construction of values is permitted. Some values require definition and application, rather than list assembly alone.
- Guideline (user+agent, 2026-09-24): a dedicated head is useful when the machine or Rule 3 needs to dispatch on the first node.
- Open (user): which kinds have dedicated heads and which use a general category. A generic `{:value}` head remains a proposal.
- Recorded rulings (user, 2026-09-24, pending review): i64 and strings are leaves, while opaque values have distinguishing heads. Native tag equality was considered inevitable.
- Decision (user, 2026-09-26): literals have two tagged forms, one with a pure nyad structure and one with an opaque payload.
- Open (agent): choose literal tags, metadata on both forms, and whether names and designators inside tagged nodes remain bare leaves.
- Open (agent): reconcile these literal forms with opaque leaves and tag designators. How tag strings themselves become observable also needs a decision.
- Goal (user, 2026-09-25): separate cell storage from language-visible representation.
- Direction (user, 2026-09-25): recognize exactly shaped executable nodes. Storage arity does not determine their observable structure.
- Decision (user, 2026-09-24, pending review): retain the Cells concept but describe runtime nodes with non-trivial piths and possibly more than two semantic children.
- Evidence (agent): `cells-encoding.md` records a storage snapshot. Its `APPLY` and `OP_FN0..2` tags need review against the agreed representations.

## Metadata and versions

- Decision (user, 2026-08-30): tagged lowered nodes carry metadata after the head. Provenance belongs in the tree.
- Recorded refinement (user, 2026-09-24, pending review): applications have metadata. Plain lists, nyads, and byte strings do not.
- Recorded direction (user, 2026-09-24, pending review): metadata fields are optional k-v entries. Metadata lists do not recursively receive metadata.
- Decision (user, 2026-09-24): `meta` is a reserved key, although its name remains debatable. Ordinary k-v entries follow positional entries.
- Open (user): reconcile optional `meta:` entries, an always-present metadata slot, and chunked payloads. The current rewrite rules use positional metadata and flat payloads.
- Open (user): an opt-in `parse_term` mode could locate every node. Whether every pith exposes metadata remains disputed.
- Requirement (user, 2026-10-19): observations are versioned language contracts. Their versions must be available in runtime data, beyond `requires v1.2.3`.
- Example (user): `[tag, [version: 1.2, ...], ...]`. This example does not settle numeric syntax, version encoding, or compatibility.
- Open (agent): what a version identifies, how versions coexist, and whether version metadata describes or selects behavior.

## Exploration, not settled semantics

- TODO direction (user, 2026-10-23): step back from Jay's direct tagging construction and assess a mechanism designed for Vetochka. `items.md` owns this assessment. Earlier Jay-style derivations remain evidence about that construction, not the selected design.
- Proposal (user, 2026-10-23): represent parsed applications as tagged forks and interpret them in VM head position. `2026-10-23.md` records the model and the unresolved boundary between returning syntax and executing it.
- Evidence (agent, 2026-10-23): `2026-10-23-application-inspection-transcript.md` preserves 40 original user and assistant messages from the September 25 and October 20 session JSONL files. It includes timestamps and source line numbers. The September exchange requires inspection without evaluation; the October exchange reopens the application representation and inspection mechanism.
- Research and open scope (agent, 2026-10-22): `2026-10-22.md` records the requested value-model work and a Scheme specification comparison. The corresponding TODO is in `items.md`; no value semantics were chosen by this research.
- Hypotheses (user, 2026-10-19): values may be characterized by their relations. Structure, reduction, and effects are candidate categories.
- Hypothesis (user): equality of relational profiles may imply identity. Such profiles need not be enumerable.
- Hypothesis (user): pith reconstruction should reproduce useful observable relations without exposing implementation internals, such as closure environments.
- Research (agent): contextual equivalence and environmental bisimulation offer relevant definitions and proof methods. They do not establish these hypotheses for Vetochka.
- Direction (user, 2026-10-19): explore `tag_apply` and `getTag_tag`. Re-derive any construction under Vetochka's triage rules.
- Boundary (agent): distinguish transparent annotations from dispatch heads and possible version selectors. Preserving application behavior does not imply identical reflective observations.
- Sources and limits (agent): `2026-10-19-research.md`. The user's model and open interpretations are in `2026-10-19.md`.
- Candidate (agent, 2026-10-20): `2026-10-20.md` derives a transparent tagging tree and structural readers under triage. Its integration remains open.
- Direction (user, 2026-10-20): explore the same hidden tagging for integer values and k-v pairs, with runtime optimizations behind the observable representation.
- Evidence (agent, 2026-10-20): `tagging_scratchpad.py` reproduces the candidate laws and integer example. `tagged-integer-trace.txt` preserves its expanded reductions.
- Experiment (agent, 2026-10-20): `exec.md` describes a pure-triage self-interpreter, with its construction in `exec_scratchpad.py` and generated term in `exec-core.vf`. Its temporary quotation format is not a language decision.
- Review constraint (user, 2026-09-26): reconsider the September 24 representation rulings before machine implementation. This rewrite does not resolve their conflicts.

- History (agent): `archive/20261020-000000/records.md` preserves the original records. Its SUMMARY maps former subjects to their current owners.
