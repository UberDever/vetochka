# v0 machine (CESK)

- STATUS: OPEN
- PRIORITY: 100
- TAGS: epic, machine

- Decision (user, 2026-08-30): core evaluation is triage rules 0..3 exactly as in the spec, plus identifier handling. Branch-first, no implicit forcing.
- (user, 2026-08-30) Timing comes from control position only. Rule 2 copies raw operands; duplicated effects are accepted.
- (user, 2026-08-30) Rule 3 never evaluates `{@}` data. ~~A pending machine application in the operand is evaluated to expose its root shape.~~ superseded by the 2026-09-25 ruling: Rule 3 never evaluates.
- (user, 2026-08-30) No specialize stage: opcodes apply until no applications remain. The machine runs `exec` on a value and doesn't walk `{@}` on its own.
- (user, 2026-07-09) The machine owns all application dispatch, family first and arity second.
- (agent, 2026-08-30) The July rule catalogue in `docs/spec/03_v0.md` is a usable CESK seed. `SHAPE-APPLY` survives only for pending control; `RESTORE-DELTA` is dead.
- Open: ZINC-style argument handling and the minimal machine state. See archive/20260613-120000/minimal-machine.md.
- History: items.md.

## Tasks

- [x] 20260529-103916 Opcode efficiency versus triage fidelity
- [x] 20260529-103917 Shift perspective from reducer to VM
- [x] 20260606-141937 Compile before execution
- [x] 20260613-151555 Semantic boundary for specialized IR
- [ ] 20260711-131257 Write the v0 CESK rules
- [x] 20260711-131258 do...end sequencing
- [x] 20260711-131259 Proper tail calls
- Decision (user, 2026-09-25): Rule 3 never evaluates. Applications persist and Rule 3 observes them like any node (arity 2); identifier dereference is its only indirection, which is how programs use binders' non-locality with Rule 3. To inspect a result rather than the computation, bind it with `$fn` and inspect the identifier. This departs from treecalcul.us, where an application cannot reach a Rule 3 branch, and the spec should say so.
