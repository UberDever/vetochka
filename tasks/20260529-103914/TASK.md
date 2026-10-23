# Specify and implement evaluation and the v0 machine

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, reducer, spec
- KIND: TASK
- PARENT: 20260926-050130

- Work (agent): define and implement the v0 evaluation rules, one-step host entry, and control machinery.
- Done when (agent): matching rules and tests cover triage, identifiers, applications as data, sequencing, traps, and proper tail calls.

## Current redesign direction

- Direction (user, 2026-10-23): abandon triage as the evaluation basis and build from its principles. Remove rules 1 and 2. Retain the construction behavior of rule 0 and the structural inspection behavior of Rule 3, with suitable new names. Use lambda as the basis for evaluation.
- Scope (agent): `items.md` owns the redesign work. The earlier contract below is historical input for review, not an instruction to implement the full triage evaluator. Coordinate bindings with 20241225-140425 and custom tagging with 20260926-091318.

## Earlier evaluation contract

- Decision (user, 2026-08-30): evaluation follows triage rules 0..3 plus identifier handling. The recorded direction is branch-first, without implicit forcing.
- Decision (user, 2026-08-30): control position determines evaluation timing. Rule 2 copies raw operands. Duplicated effects are accepted.
- Decision (user, 2026-09-25): Rule 3 never evaluates applications. It observes them as nodes, with arity 2.
- Decision (user, 2026-09-25): a value can be finished while its branches contain application data. No recursive normalization is required.
- Decision (user, 2026-09-25): identifier dereference is Rule 3's only indirection. Bind a computation's result before inspecting that result.
- Qualification (agent): older rules that forced pending operands are superseded. Earlier callee-first drafts need reconciliation, not silent reuse.
- Decision (user, 2026-08-30): there is no specialize stage. The machine does not autonomously traverse `{@}` data.
- Decision (user, 2026-07-09): the machine owns application dispatch, first by family, then by arity.
- Decision (user, 2026-08-30): statement sequences use a BLOCK frame. Sequencing is not merely syntax sugar.
- Direction (user): push a frame only when a subresult is needed. Tail calls replace control rather than grow the machine stack.
- Constraint (user): native C calls use trampolines, without machine re-entry from C.

## Host entry and unresolved choices

- Decision (user, 2026-09-26): `exec` chooses and executes one applicable rule. The CESK rules define evaluation without separate deep-evaluation behavior.
- Direction (user): `exec` is a host-side entry and likely not directly available to programs.
- Open (agent): whether the host or a specified driver repeats the step until a value.
- Direction (user, 2026-09-24, pending review): converting `[{@}, meta, f, x]` to machine control ignores its metadata.
- Open (agent): the scope of that principle, especially with versioned observation in 20260926-091318. A universal metadata-ignoring rule was not approved.
- Open (agent): minimal machine state and ZINC-style argument handling. Older candidate mechanisms remain in the archive.
- Recorded defect (agent): Rule 3 dispatches on storage arity in `public/reducer/reducer.c`. The replacement must use the agreed observable shape.
- Evidence (agent): `apply-evidence.md` preserves the execution and inspection analysis. The historical July CESK catalogue is a starting point, not current authority.
- Boundary (agent): 20260926-091318 owns representations and piths, 20241225-140425 owns binders, and 20250123-091611 owns native and host operations.

## Delivery

- Decision (user+agent, 2026-09-26): implement vertical slices with rules, C implementation, and tests together.
- Rejected (user+agent): a long specification-only phase and a code-first phase that drifts from the agreed semantics.
- Sequence (agent): `items.md` carries the approved order as steps within the consolidated tasks.

- History (agent): `archive/20261020-000000/records.md` preserves the original records. Its SUMMARY maps former subjects to their current owners.
