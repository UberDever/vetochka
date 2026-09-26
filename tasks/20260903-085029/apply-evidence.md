# `{@}`: evidence on execution, inspection and forcing

Gathered 2026-09-25 from tasks and docs. The 2026-09-03 experiment is excluded.
Newest first within each part.

## Current framing

- Execution: (user, 2026-08-30) applications are `{@}` markers always; even bootstrap code is
  inert and must be forced. `exec` is a host-side, top-level, apply-once routine, the only door
  into machine control. "APPLY exists only in machine control; in data, application is the `{@}`
  marker" (agent wording, adopted in the same entry). Inside the machine, application is
  callee-first (July `RUN-APPLY`).
- Inspection: (user, 2026-08-30) Rule 3 never evaluates `{@}` data; markers are data with their own
  pith. Building `[{@}, f, x]` executes only the list constructors (staging entry).
- Forcing: (user, 2026-08-30) no implicit forcing; timing comes from control position only.
  Rule 3 does not force. The one exception: a pending machine application in the operand is
  evaluated to expose its root shape; July `SHAPE-APPLY` survives for pending control only.
- (user, 2026-08-30) No specialize stage: execution is opcodes applying until no applications
  remain. The machine does not walk `{@}` on its own; it runs `exec` on a supplied value.
- Direction (user, 2026-08-30): `exec` is likely not user-accessible; applying one value to another
  already interprets code in the language.

## 2026-09-24 (user drunk; review sober)

- Decision: applications carry meta, `[{@}, meta, f, x]`; the encoder emits `{@}` data, not APPLY
  cells (committed in `0ed9c28`).
- Direction: `exec` ignores meta when it turns `[{@}, meta, f, x]` into a machine application.
  (agent, unruled) the rule should cover every opcode that reads raw syntax; wording "meta never
  changes what execution computes".
- [fact] Tree book 5.4: only programs (normal forms) are tagged, not computations; `{@}` data are
  programs in that sense.

## Spec today

- `02_syntax.md`: "`{@}` marks application and isn't part of the syntax, only notation for the
  cells to come"; the lowered tree "isn't executed until it comes into executable position";
  markers let programs "construct and inspect arbitrary application trees without the need for
  their execution".
- `03_v0.md`: task 20260903-085029 — "`exec`, the host apply-once entry that forces a value term
  containing `{@}`". No CESK rules yet.

## History

- 2026-07-11 (user+agent): `{$}` renamed `{@}`; "`{@}` is normalized-term data mapping to `APPLY`".
  Rule 3 obtains the current triage view only and does not force APPLY; later that day, Rule 3
  shape demand on a top-level APPLY schedules it and resumes (`SHAPE-APPLY`), never classifying
  APPLY by storage arity. The reducer's arity dispatch is a regression from `e0b7b3f`.
- 2026-07-09 (user): APPLY is "structurally inspectable as code, shapeless as a value; present from
  parse time".
- 2026-06-11 (user): every application is a binary APPLY cell; `{$}` is only its textual notation.
- 2026-06-04 (user): apply nodes are written `f(x)` and stored as `$ f x`; the VM schedules them.
- 2026-05-31 (user): store apply nodes in the parsed tree, or application info and homoiconicity
  are lost.
- 2026-02-22 (user): application implicit by position; `$` shown only for clarity.

## Gaps

- ~~The spec's "only notation for the cells to come" predates 2026-08-30, when `{@}` became data.~~ closed (user, 2026-09-26): the spec now shows applications as `[{@}, meta, f, x]` data.
  (agent, 2026-08-30, unruled) its "not part of the language" status dissolves into an ordinary
  inert node type.
- Pith of an un-executed `{@}` datum: open (task 20260805-162238).
- ~~What "apply-once" means (one rule, one saturation, or run to a value): open.~~ settled (user, 2026-09-26): one step, one chosen rule; see the exec task.
- The reducer still dispatches APPLY cells; nothing yet turns `{@}` data into them. That is
  `exec`'s job.
