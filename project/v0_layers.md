# v0 Layered Design (restated)

Status: OBSOLETE where it contradicts `.memsearch/memory/2026-08-30.md` and
`dialogue/binders.md` — notably: Layer 3 specialization (no specialize stage
exists; names live to runtime, die by env lookup), `LOCAL`, the closed-term
discipline, `do!`/`{fn}` spellings, and Layer 4 `{specialize}`.

Previous status: authoritative snapshot, updated 10.07.2026. Where this conflicts with
[`v0_cesk_draft.md`](../.memsearch/memory/v0_cesk_draft.md), this wins. Layer 5 not yet restated.
Decision history lives in [project history](../.memsearch/memory/project_history.md).

Claim labels used below: **implemented** describes current C code; **decided** is
target semantics; **open** must not drive implementation yet.

Each layer adds vocabulary. No layer uses notions introduced above it.

---

## Layer 0: Triage calculus

Syntax: `^` and application. Literals are leaf-shaped. Lists are right-nested
forks: `^(x, ^(y, ^(z, ^)))`, nil is `^`.

Rules (`$` explicit for clarity; application is positional in implementation):

```text
0a. ^ $ x            -> ^ x
0b. ^ x $ y          -> ^ x y
1.  ^ ^ x $ y        -> x
2.  ^ (^ x) y $ z    -> (x z) (y z)
3a. ^ (^ w x) y $ ^        -> w
3b. ^ (^ w x) y $ (^ u)    -> x u
3c. ^ (^ w x) y $ (^ u v)  -> y u v
```

Confluent: normal forms unique, reduction order unobservable. Order becomes
observable only with effects (Layer 5) and must be pinned there.

## Layer 1: Cells

Byte array of tree-shaped nodes. Vocabulary: `CELLS_NODE_TYPE_ITEMS` in
[`cells_api.h`](../reducer/cells_api.h).

- Existing: `DELTA0/1/2`, `VALUEF0/1/2`, `VALUEV0/1/2`, `APPLY`, `OP_FN0/1/2`, `REF`.
  `OP_FN*` is obsolete experimental encoding. `LOCAL` and generic opcode-state
  cells are target additions. Enum values are C API identities; wire tags differ.

**Opcodes over delta-encodings.** Opcode cells are target additions, not delta-term
encodings. Opcode state identifies its kind and transition; no separate opcode
family identity exists.

Speculative Layer 5 direction, not a Layer 1 design decision: runtime opcode values
may be kept stem-shaped, with completing application dispatching immediately rather
than exposing a saturated fork. If so, Rule 3 sees any opcode value as a stem.
Opcodes are not decomposable into deltas; deeper inspection needs intrinsics
(`get_type` / `get_payload`). Literals likewise: leaf-shaped, value family —
"literals are deltas" holds only shape-wise.

**APPLY is a distinct executable-cell node, not a fork.** It records application
structure; losing it loses homoiconicity. **Implemented:** parsing first produces
  `source_tree_t`; `bytecode_source_encode` later creates APPLY cells. Existing
  `OP_FN0/1/2` nodes describe old implementation, not current target semantics.

**Two inspection channels:**

1. *Structural* — traversal over syntax or nodes at rest. APPLY is an ordinary
   binary executable-cell node. Exact runtime syntax representation is open.
2. *Triage* — rule 3 branches on leaf/stem/fork shape. APPLY has no shape; how an
   APPLY yields a shape is a Layer 5 concern.

Summary: **APPLY is inspectable as code, shapeless as a value.**

Cells are storage only. A cell root becomes executable only after specialization
marks it as an executable root and Layer 5 activates/evaluates that root. Merely
storing an opcode/APPLY-shaped node does not run it.

## Layer 2: Syntax

**Decided:** v0 includes its surface syntax and sugar; there is no separate v1
merely for `do!`, statements, multiple parameters, or similar conveniences.
Syntax is inert and nameful. `do!` is syntax, never a runtime opcode.

**Implemented:** text parses to host `source_tree_t`; `bytecode_source_encode`
then lowers that tree into cells. Thus "parser emits syntax cells" is false for
current code.

**Open:** runtime `{parse_term}` must eventually return syntax that v0 code can
inspect and transform. Whether that representation reuses executable cells or
uses another explicit term form remains unsettled.

Tags (`:block`, `:id`, `:label`) describe source structure only. Application is
present in parsed structure; the APPLY cell is created during compilation.

**Open after pipeline correction:** spans remain host diagnostic metadata, but
their key cannot be specified as a cell index before syntax representation settles.

Unspecialized code is **inspectable, not activatable**: identifiers are still
nameful data.

## Layer 3: Specialization

Specialization performs two jobs: (1) rewrite/lower sugared v0 syntax into the
minimal executable form; (2) compile that form into cells. Binding compilation
turns recognized constructs into runtime opcode states and names into indices.
**Names die here.** No runtime name lookup exists. Layer 5 must preserve the lexical
meaning of `LOCAL`.

Executable vocabulary:

```text
e ::= DELTA | I64(n) | BYTES(bs) | APPLY(e, e) | LOCAL(n) | OPCODE(state)
```

- `LOCAL(n)`: de Bruijn-style index; `0` is nearest binder.
- `OPCODE(state)`: runtime opcode state. State itself identifies opcode kind and
  transition; no separate family identity exists.

Output is a tree term in this extended vocabulary — not Layer 0 vocabulary.

**Superseded:** `{fn}` is no longer v0 syntax. Binding constructs lower from
`do!` function syntax into the minimal executable representation. Exact runtime
closure/opcode encoding remains open. Top-level definitions and cross-unit
references still lower through function application — a module is a function of
its imports:

```text
let x = e in body   ==   application of a lowered one-parameter function to e
```

Lexical scope is settled; its Layer 5 runtime representation is not. `LOCAL(n)` is
the specialized lexical address. `E`, `CLOSURE`, and `TERM` in the CESK draft are
candidates for giving it runtime meaning, not decisions.

Speculative surface direction:

```vetochka
do! fn: [x, y, ...] do ... end with: ...
```

The exact spelling above, plus a possible `do! fun:` expression form with implicit
parameter `it`, remains speculative rather than decided semantics.

**Recursion is intended to be derived, not primitive; `{rec}` is rejected.**
Speculative validation, pending Layer 5 argument/forcing decisions: the plain
Y-combinator works if a non-strict function/form mode receives `f(f)` unevaluated
and forces it in callee position. Historical notation below is not current v0
syntax; it records only that semantic experiment:

```text
fix = {fn}(F) do ({fn}(f) do F(f(f)) end)({fn}(f) do F(f(f)) end) end
fact = fix({form}(rec) do {fn}(n) do ... rec(n - 1) ... end end)
```

If Layer 5 keeps that non-strict mode, mutual recursion can bind one expression
yielding the list of functions. Cost: `f(f)` re-evaluates at each recursive call
(no memoization).

**Closed-term discipline.** Free names must resolve to native intrinsics; all
else must be bound inside the term. Output is closed: it contains no unresolved
names. Value injection is **by application**, not hidden binding injection:
compile with parameters, then apply.

## Layer 4: Backbone intrinsics

```text
{load_file}  :: path bytes -> bytes | diagnostic
{parse_term} :: bytes -> syntax root | diagnostic
{specialize} :: syntax root -> executable root | diagnostic
```

- `{load_file}`: read a file. The only effectful one.
- `{parse_term}`: bytes to Layer 2 syntax. Pure.
- `{specialize}`: syntax to executable root. Free names resolve against the fixed
  native registry or error. Pure. One C implementation, two entry points — seed
  (build time, on `boot.tree`, v0 core only) and runtime primitive.

Loop:

```text
{load_file}(path) -> bytes -> {parse_term} -> syntax
    -> traverse/transform (structural channel)
    -> {specialize}(syntax) -> executable root -> activate
```

Seed performs v0 lowering, lexical resolution, and cell emission only. Modules,
types, C generation, and policy belong to bootstrap code.

## Layer 5: Machine — NOT YET RESTATED

Settled constraints:

- Machine owns **all** application dispatch. Triage rules survive verbatim as the
  delta-family case.
- Opcode dispatch uses opcode state, not a separate family identity.
- No store or boxes in v0.
- Proper tail calls are required.
- ZINC-derived argument handling is required while it remains simple.
- Machine state and runtime entity kinds stay minimal and orthogonal.

Open: meaning of `LOCAL` at runtime; function values; strict/non-strict arguments;
machine state; argument spine and GRAB; under/exact/over-application; continuations;
evaluation order; effect duplication; exact function/opcode cell encoding.

[`v0_cesk_draft.md`](../.memsearch/memory/v0_cesk_draft.md) provides candidates for these questions, not settled semantics.
