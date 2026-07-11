# Dialogue ledger: minimal Layer 5 machine

Mode: lightweight. Started 10.07.2026.

## Framing

Question: what is the smallest Layer 5 machine implementing settled Layers 0–4?
Pre-implementation: cheap to revise now, expensive after C implementation.

## Settled input

- [`project/v0_layers.md`](../../project/v0_layers.md) Layers 0–4 are authority.
- Machine owns executable application dispatch; triage rules are delta case.
- No store or boxes.
- ZINC-derived argument handling required while simple.
- Proper tail calls required.
- Minimize machine state first, then entity kinds; judge simplicity by dialogue,
  not raw counting.
- Effects/native inventory is above this decision; Layer 5 exposes dispatch only.
- `do!` is v0 syntax, not runtime opcode; `{fn}` syntax is obsolete.
- Specialization lowers syntax, resolves names, and emits executable cells.
- Opcode state alone identifies opcode kind/transition; no family field.
- Opcode values are stems; completing application dispatches immediately.

## Current implementation facts

- Parser produces `source_tree_t`; `bytecode_source_encode` later creates cells.
- Reducer is a flat C loop dispatching primarily by node arity.
- `OP_FN0/1/2` encode an older experiment, not target semantics.

## Speculation, not decisions

- `do! fn: [x, ...] do ... end with: ...`.
- Short `do! fun: <expr>` with implicit `it`.
- Every Layer 5 mechanism in [`v0_cesk_draft.md`](v0_cesk_draft.md), including `CLOSURE`,
  `TERM`, environment chains, forcing frames, and exact GRAB transitions.

## Required semantic problem

After specialization, `LOCAL(n)` names a lexical binding position. Layer 5 must
give that address runtime meaning while preserving lexical scope. No representation
has been selected.

## Open cruxes

1. Exact executable function/opcode cell produced from `do!`.
2. Whether runtime opcodes can remain partially applicable; if so, where supplied
   values reside.
3. Minimal lexical-binding representation.
4. Strict/non-strict argument representation and forcing semantics.
5. Minimal machine registers/stacks and continuation kinds.
6. Exact ZINC under/exact/overapplication rules.
7. Evaluation order and effect duplication.
8. PTC invariant and native trampoline rule.
9. `do!` sequencing lowering versus machine support.

## Superseded mistakes

- Runtime general `{do}`: false; `do!` is syntax.
- “Parser emits syntax cells”: false for current implementation.
- Promoting draft `E`, `CLOSURE`, or `TERM` into settled semantics: false.

## Decision

None yet for Layer 5.
