# Opcode framing: research and proposal

Status: open discussion. No specification change or new execution contract is approved.

## Framing and current model

- (user) The Execution section lists application, binder, closure, and triage as control values. The user asks how opcodes fit, drawing on earlier research.
- [proposal] (user) An opcode is a possibly control-changing intrinsic operation with a possible side effect.

## Evidence

- July research describes opcodes as argument-dependent state machines. Source: artifacts/20260322-120000/claude_opcode_do_research_2026-07-10.md. The artifact explicitly distinguishes research from authority.
- The committed July account records opcode states, ordered inputs, private payload, and immediate dispatch at saturation: (16.07.26, 0). Input policies distinguish retained syntax, evaluated inputs, and label routing: (16.07.26, 1). Transition outcomes are Continue, Dispatch, or a hard trap: (17.07.26, 0). Supporting source: artifacts/20260711-120000/2026-07-11.md.
- Later list/designator dispatch replaces per-state storage tags: (26.09.26, 1). The leaf/fork/blob starting point replaces the homogeneous tree substrate: (07.11.26, 0). Old stem projections and integer-state layouts cannot be assumed to specify the current model.

## Proposed interpretation and standing objection

- [proposal] (agent) Define an opcode as an interpreter-defined primitive operation, represented by a value, with a defined input and execution protocol. Its execution may produce a result, direct evaluation, or perform effects.
- [reasoning] (agent) Optional control changes and effects describe possible behavior but do not distinguish opcodes from closures. Interpreter-defined semantics provide the useful distinction. Ordinary CESK transitions already change control, so special control behavior needs a narrower meaning.
- [proposal] (agent) Retain the protocol insight without reinstating old state IDs, stem projections, or private payload rules. Pure arithmetic, host effects, and evaluation control need different contracts within this framing.

## Open questions

- Are binder and triage distinguished core forms, or members of the same intrinsic-operation category? No classification is selected.
- How do primitive results return to a continuation without unintended execution of returned syntax? Earlier return-protocol questions remain open.
