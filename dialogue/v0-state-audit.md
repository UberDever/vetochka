# v0 state audit

Status: reconstruction aid, not a semantic specification.

This file separates mechanical repository evidence from direct user statements. It
makes no new decision. A claim is **verified** only when it is either present in a
committed spec/ledger record or stated directly by the user in the current dialogue.
A committed record can still be superseded; it is evidence of prior state, not
automatic current authority.

## Repository baseline

- Latest committed L5 work: `a66220f` (2026-07-21), preceded by `907874e`
  (CESK transitions), `d126d83` (nameful `TERM` model), and `2f24752` (opcodes).
- `docs/spec/03_v0.md`, `dialogue/v0-layers-restoration.md`, and memory have
  uncommitted edits. They must not silently be treated as accepted specification.
- Current C implementation remains behind this design: it has old `OP_FN*`, not
  the L5 runtime families described here.

## Verified current direction

### Layers and syntax

- L0 is pure triage calculus; L1 cells are inert storage.
- L2 parser normalization is nameful inert term data. Labels normalize as
  `[{:label}, {name}, payload]`; bare blocks as `[{:block}, S1, ...]`; lists are
  proper lists.
- v0 is a vf subset. vf owns extended syntax interpretation.
- No language-level quote, `specialize`, `execute`, or `eval` operation.

### Activation and lexical context

- Activation/specialization happens on executable position, not data position.
- Current committed L3 direction is nameful lexical `TERM(root, E)`; the old public
  `LOCAL` semantic model is superseded. `E` is opaque.
- Runtime application is callee-first and retains the pending operand with caller
  lexical context.

### Errors and effects

- Expected/user failure is ordinary `{:ok, value} | {:error, diagnostic}` data.
- Runtime/protocol failure is an uncatchable v0 trap. The host retains an optional
  diagnostic term handle and stops execution; exact diagnostic layout is open.
- v0 is effectful. Source/transition order determines host-effect and trap order.

### `$` protocols

- Initial selector inventory: `f`, `fn`, `load_file`, `parse_term`.
- `$f` takes validated binder syntax then `do:` single-expression syntax body.
  Body is specialized at construction under defining lexical context plus binder.
- `$fn` takes validated parameter-list syntax then positional bare block syntax.
- `with:` is explicitly removed from current v0 work.
- `to:` is the current committed candidate for closure call syntax; its remaining
  curried/overapplication semantics are open.

## Verified presentation/process decisions

- Opcode states are recorded as nested state trees, not wide tables.
- State IDs are allocated only after state names/transitions stabilize.
- Mermaid, if used, is a projection rather than authoritative semantics.

## Settled opcode representation

Each opcode transition state is its own concrete cell node type, in one global
opcode-state table. Illustrative states:

```text
OPCODE_SELECT       // $
OPCODE_F            // $ f: x
OPCODE_F_DO         // $ f: x do: body
OPCODE_F_DO_TO      // $ f: x do: body to: argument
...
OPCODE_EXT1 ... OPCODE_EXT4
```

`OPCODE_EXT1`–`OPCODE_EXT4` are four separate extension/namespace states for
arbitrary future states. This is not an open representation choice. Remaining work
is each state's wire layout and LENS/CEK behavior.

## Unverified / must not be relied upon

- Exact relation of activated `$` application spines to L5 `SHAPE-APPLY` and LENS.
  Existing notes describe alternatives; no current rule should be inferred.

- Exact LENS projections/provenance for TERM, ID, opcode, closure, bytes, and
  integer nodes.
- Closure versus opcode taxonomy, closure call state, eager stopping condition,
  repeated `to:`, overapplication, `$fn` sequencing, builders, and cell ABI.
- Exact `load_file`/`parse_term` source spelling and result ABI in the committed
  spec: working-tree edits exist but have not been audited as final.

## Stale items to remove only after representation audit

- Old `with:` state/tree entries.
- L2's old special-list-form open item.
- L4 wording that calls facilities bare native names if `$load_file`/`$parse_term`
  remains current.
## Document consolidation decision

Selected structure:

```text
01 Foundation       tree calculus
02 Shared syntax + cells
03 v0 runtime       activation, CESK, runtime states, host facilities
04 vf
```

L0 is foundation, not technical v0 layer. L1 moves into shared syntax/cells. L3/L4/L5
consolidate into v0 runtime. Content refactor deferred.

## Next research task

Do not extend `$fn`, lists, or builders.

Define the LENS/CEK behavior for settled opcode states: how `$` activation reaches
`DOLLAR_SELECT`, how an opcode is shaped for Rule 3, and how selected children
retain provenance.
