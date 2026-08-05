# v0 layers restoration dialogue

## Framing

Question: what is the correct lightweight form for describing Vetochka v0 language + runtime so a usable spec can be formed now, with conversation logs preserving decision trace?

Stakes: foundational design/spec shape. Wrong form either overburdens the project or lets old drafts silently become decisions.

Depth: lightweight colloquium per layer; full colloquium only if Layer 5 machine design gets genuinely contested.

## Method constraints from user

- No current authority. Existing docs and memory are design hints only.
- Decisions are formed now in dialogue.
- If user hesitates about a decision, inspect existing docs/memory to find contradictions and previous thoughts.
- Avoid huge specification bureaucracy; use Pareto 80/20.
- Primary goal: formulate a spec first.
- Conversation logs are artifacts for tracing decisions back, not the main spec.

## Working spec form (candidate)

Use one compact spec document with per-section claim tags only where needed. Keep logs separate.

Minimal section pattern:
- Claim: what v0 says.
- Why: one-line rationale when non-obvious.
- Open: only blockers or known unsettled points.
- Notes: implementation facts or historical warnings, clearly non-normative.

## Assumptions register

- [fact] User rejects treating current docs as authority; they are hints.
- [fact] User wants 80/20 spec-first process, not full typed spec package.
- [fact] User wants layers kept in `docs/spec`.
- [fact] Existing old thoughts are valuable but should be date-marked as historical hints, not treated as current decisions.
- [assumption] Best next move is to define a compact spec skeleton, then walk layers using only blocking questions.

## Layer 0 pass — 2026-07-11

Output: `docs/spec/03_v0.md` created with Layer 0.

Decision draft:
- Layer 0 is only pure triage calculus: `^` and binary application.
- Shapes: leaf `^`, stem `^ x`, fork `^ x y`.
- Proper lists are right-nested forks, nil is `^`.
- Reduction rules 0a, 0b, 1, 2, 3a, 3b, 3c are included verbatim.
- No literals, names, syntax tags, cells, opcodes, machine state, effects, environments, modules, or evaluation order policy in Layer 0.

Rationale:
- Keep reflective substrate minimal.
- Higher layers may represent values as leaf/stem/fork-compatible, but this is not Layer 0 vocabulary.

Open:
- None for Layer 0 draft.

User TODO handling:
- Lower layers should not forward-link to higher layers as dependencies; higher layers may backward-link to lower layers.
- `$` should be local explanatory notation only; actual surface syntax may use ordinary call notation.
- Proper-list encoding should be presented as higher-layer convention using Layer 0 shapes, not as primitive Layer 0 syntax.

## Layer 1 pass — 2026-07-11

Output: `docs/spec/03_v0.md` extended with L1 Cells.

Decision draft:
- Layer 1 is storage, not evaluation.
- Cells store typed tree-shaped nodes with arity 0/1/2.
- Minimum families: `DELTA*`, `VALUEF*`, `VALUEV*`, `APPLY`, `REF`.
- `DELTA*` represent Layer 0 shapes.
- `APPLY` records application structure as data; it does not apply.
- `REF` is storage machinery, not a language-level value.
- Exact wire tags, allocation, ref width, free-list policy, and C enum values are implementation details unless later promoted to ABI.

Rationale:
- Preserve inspectable tree structure without mixing in machine behavior.
- Keep old cell/encoding work as historical hints.

Open:
- Exact direct node families and wire tags for `LOCAL`, generic `OPCODE`, and other specialization-introduced nodes. Current direction: real node families, not payload encodings hidden in `VALUE*`.
- Canonical text serialization for cells.

User TODO handling:
- Do not extend leaf/stem/fork vocabulary to all node families. In L1, use storage arity; only `DELTA0/1/2` directly represent L0 leaf/stem/fork.
- `REF` recorded as graph-organization/storage-layout machinery, not a language-level value.
- Inspected Claude project memory: old 09.07 memory treated opcodes as node families, not delta encodings, but current session does not accept it as authority. It supports the present direction.
- Inspected current cell implementation: `reducer/cells_impl.h` explicitly says MASK/CODE/layout combinations form stable bytecode ABI. Promoted current non-`OP_FN*` wire tags to L1 draft.
- Removed `OP_FN*` tags from current spec. Kept their existence only in logs/notes as implementation artifacts.
- `LOCAL` kept out of L1 decisions for now; revisit after Layer 3 specialization.
- Assessed formatter/codegen note: current code formats parsed `source_tree_t` canonically (`source_tree_format_canonical`) and encodes source trees into cells (`bytecode_source_encode`), but does not define canonical cell-to-text serialization.
- Canonical text serialization remains required but unspecified.

Historical hints date-marked:
- 2026-02-04: uniform leaf/stem/fork node idea, `get_type`/`get_payload` inspection.
- 2026-02-09: compact byte tags and variable-width refs.

## Layer 2 pass — 2026-07-11

Memory used:
- Searched memory for syntax, `do!`, `parse_term`, source representation, and v0 surface/core boundary.
- Relevant hints: 2026-07-10 memory says `do!` is syntax not runtime opcode, exact spelling open; parser currently emits `source_tree_t`; syntax representation at runtime remains open.

Output: `docs/spec/03_v0.md` extended with L2 Syntax.

Decision draft:
- Layer 2 is source language + inert parsed representation.
- Source text is UTF-8.
- v0 is minimal kernel syntax; parser-recognized forms not admitted here belong to a separate vf (`v*`) language.
- Syntax is nameful data and has no runtime behavior by itself.
- Current v0 grammar includes literals, identifiers, grouping, list syntax, postfix calls, labeled args, and `do ... end` block args.
- Newlines may become semicolons in layout-active contexts.
- Parsed source normalizes into bootstrap syntax terms; purely syntactic rewrites happen here.
- List syntax desugars during L2 normalization, not specialization.
- Normalized v0 tags include `:id`, `:block`, `:group`, `:label`; vf languages may add tags like `:annot`, `:selector`, `:prefix`, `:infix`.
- Application in syntax is source structure only; later translation may encode it as L1 `APPLY`.
- Spans are diagnostic metadata, not syntax meaning.

Rationale:
- Keep source inspectable and transformable without giving tags machine behavior.
- Shared parser may recognize more forms than v0 uses; those forms are specified in the vf spec, not v0.
- Specialization is reserved for context-dependent work: name binding, executable construct recognition, and runtime compilation decisions.

Open:
- Exact `do!` opcode-construction conventions; should derive from source structure, not separate syntax transformation.
- `{parse_term}` should return the same normalized syntax term produced by parsing/normalization, represented as a term; exact ownership/diagnostics/cell representation open.
- Old `docs/grammar.md` has been moved to memory as historical artifact; current grammar must be re-specified in layer docs.

## Grammar reconciliation / vf syntax — 2026-07-11

Memory used:
- Searched memory for grammar, vf/v*, annotations, infix, selectors, and v0/vf split.
- Relevant hints: 2026-05-28 syntax experiments introduced Algol-like/postfix syntax, annotations, no precedence, no mixed infix; 2026-07-10 emphasized `do!` as syntax, not runtime opcode.

Inspected: `docs/grammar.md` before moving it to memory.

Inconsistencies found:
- `docs/grammar.md` says v0 is only application/delta/i64/string, but current v0 syntax layer needs identifiers, grouping, lists, labeled arguments, and blocks as inert source structure.
- Grammar recognizes annotations, prefix/infix, selectors, bracket-call, byte-call; these were leaking into v0. Reclassified as vf syntax unless explicitly admitted by v0.
- `{fn}` / opcode protocol text in grammar is historical and belongs to later runtime/translation layers, not Layer 2 syntax.
- Cell encoding table in grammar belongs to translation layers, not syntax.
- `:list` normalization conflicts with v0 decision that list syntax desugars during L2 normalization. Assessed below and rejected as default preservation.

Output:
- Added separate `docs/spec/04_vf.md` for vf (`v*`) languages.
- Moved outdated `docs/grammar.md` to `.memsearch/memory/artifacts/grammar.md` as historical artifact.

Decision draft:
- Use notation `v*`; prose name is **vf**.
- Reject `vstar` as current spec name.
- vf languages are supersets of v0 built over the same parser substrate, not separate runtime layers.
- vf languages are not lowered by compiler developers as a fixed product feature; programmers write language code using v0 protocols for introspection, runtime term forging, and compilation.
- `vmeta` and `vsystem` are far-future speculative use-case names; keep as note only, not active design topic.
- v0 remains minimal kernel; parser-recognized forms outside v0 belong to vf languages.
- Current vf forms: annotations, bracket/list-payload application, byte-payload application, prefix syntax, flat single-operator infix, selectors.
- Annotation payload is comma-list.
- `do!` parses as ordinary identifier-like token; opcode construction is built from ordinary source structure.
- Ordinary list syntax should not be preserved as `:list`; it desugars to proper lists. If special source-level list shape is needed, introduce explicit tagged special form.

Open:
- Exact vf protocols exposed by v0 for introspection, runtime term forging, and programmer-written compilation.

## vf syntax substrate rule — 2026-07-11

Decision:
- Remove open question about special source-level list forms; it invented a need
  not demonstrated by current syntax.
- Ordinary lists desugar to proper lists, with no `:list` preservation.
- Application, labels, lists, annotations, prefix/infix shape, selectors, and byte
  payloads are sufficient syntax substrate for programmer-defined data/DSL forms.
- Parser supplies structure only. Programmer-written vf code assigns collection,
  declaration, and DSL semantics.
- Do not add literal grammar merely to make data structures prettier. Require a
  demonstrated semantic distinction that existing structure cannot preserve.

## vf inclusion rule — 2026-07-11

Decision:
- v0 is a vf subset: any v0 code may occur unchanged as ordinary vf code.
- This is semantic inclusion, not merely shared parser acceptance.
- vf adds source forms and programmer-written interpretation above v0; it does not
  cross a special bridge to call v0.
- `$` therefore occurs unchanged in vf as ordinary inherited v0 code. Its protocol
  semantics remain defined by v0.

## `$` protocol model — 2026-07-11

Confirmed model:
- `$` is special lexer/parser token, then participates in ordinary labeled/postfix
  application structure.
- `$f: x do: x + 1` is smallest one-parameter non-eager lambda primitive.
- `$force: e` is proposed explicit forcing operation for delayed terms.
- `$fn: [x, y] with: [42, 69] do body end` has `with:` as surface convenience
  exactly equivalent to postfix application of function to `[42, 69]`; no hidden
  capture/prebinding semantics.
- `$fn` is proposed eager multi-argument function form; `$form` proposed
  non-eager multi-argument analogue.
- `do:` / `end:` are ordinary labels wherever labels occur.

Assessment pending:
- Whether `$force` forces a supplied suspended argument only or activates arbitrary
  syntax data; latter conflicts with previous no-`eval`/no-user-visible activation
  decision.
- Whether `$fn` and `$form` must be raw protocols or derive from `$f` plus an
  explicit forcing/delay mechanism.
- Whether `with:` should be parser-level desugaring rather than opcode field/order
  semantics.

## Opcode privacy and function protocol — 2026-07-11

Decisions:
- No in-place term rewrites anywhere unless explicitly introduced later as a
  justified optimization.
- `$f: x do: body` is eager one-argument function with single-expression `do:`
  body.
- `$fn: [x, y, ...] do body-forms end` is eager multi-argument function form.
- `$form` is not a v0 protocol.
- `LOCAL(n)` is L3 output only; raw syntax and ordinary programs have no direct
  `LOCAL` constructor.
- Every opcode is generic `OPCODE(state)` in cells. State identifies kind,
  transition, and arity; no per-opcode node family/layout.
- On first activation source `$` specializes directly into generic opcode state;
  subsequent labeled arguments apply to transient opcode state.
- Opcodes have stable public term/cell structure and private unforgeable runtime
  state. All terms are public API, so public opcode interface must be explicit and
  Rule 3 inspectable.
- Lexical environment/captures are private implementation detail. No general
  capture/environment inspection or forging facility.
- Opcode state is called unsaturated/saturated, not leaf/stem/fork. Unsaturated
  opcode projects as stem when Rule 3 demands structural observation. Saturation
  dispatches immediately; no saturated opcode value is observable as fork.

Candidates / open:
- K does not safely delay lexical code that escapes without capture. Candidate
  replacement: eager `$f` thunk `$f: _ do: body`, forced by `thunk(^)`.
- Exact opcode public structure, state fields, closure payload, sequencing account,
  and `with:` semantics remain unresolved.

## `$` body block normalization — 2026-07-11

Decision:
- Bare `do ... end` supplied to `$fn` remains positional block term, normalized as
  `[{:block}, S1, S2, ...]`; it is not rewritten to `do: block` label.
- Current implementation and old grammar normalize labeled input `name: value` as
  generic tagged data `[{:label}, {name}, value]`. Dynamic `{:name}` tags remain
  unchosen design, not current decision.
- Generic opcode input history is ordered ordinary terms. It contains generic label
  terms, block terms, and raw postfix call/list/string terms in application order.
  State decides what each input term means.

## Opcode state and input policy — 2026-07-11

Decision:
- Public opcode state is a stable flat global integer enum. Debug names may map IDs
  to text but have no semantics.
- Every opcode state declares the policy for its next input. v0 presently has
  `syntax` (retain normalized term without evaluation) and `eager` (evaluate in
  caller context before transition); future policies must be explicit.
- An eager input evaluates to `V`, then creates a fresh next opcode state whose
  ordered public inputs append `V`. A syntax input appends the original normalized
  term. There is no in-place rewrite and no observable half-transition.
- `$` protocols are runtime opcode forms with state-defined input policy, not parser
  special forms. First-class thunk/force semantics remain open.

## Deferred opcode/L5 questions — 2026-07-11

Resume in this order:

1. Final opcode argument-record protocol and L2-to-opcode storage boundary. Candidate:
   `[{:pos}, ^, payload] | [{:label}, key, payload]`, held in ordered proper-list
   input history. Do not preserve separate list/string source spelling.
2. Saturation/dispatch protocol: which state transitions dispatch; diagnostics;
   behavior when application continues after a dispatch result.
3. Exact `$SELECT` routing table and allocation of flat state IDs.
4. Closure runtime representation: opaque lexical `E`, capture lifecycle, and
   `LOCAL(n)` frame/account model.
5. Precise eager evaluation model: evaluation target, left-to-right order, one-time
   evaluation, and under/exact/overapplication.
6. First-class thunk/force protocol. Candidate `$f: _ do: body`; K alone is unsafe
   for escaping lexical code. This is not decided.
7. `$fn` block sequencing semantic account and implementation equivalence.
8. `$fn with:` state protocol, evaluation order, and postfix-application equivalence.

Settled before pause:
- Public opcode state uses a flat global integer enum.
- Public opcode projection candidate accepted: `^ i64(state) inputs`, with private
  payload unreachable. `eval_step` handles opcode nodes explicitly.
- Inputs are an ordered proper list; no hidden vector/tuple.
- `syntax`, `eager`, and `route` are explicit state input policies.
- `route` accepts raw `[{:label}, {selector}, payload]`, selects an arm, and applies
  that arm's payload policy. `$load_file` / `$parse_term` use `$` selector spelling.
- Eager transition evaluates in caller context, appends the resulting value in a
  fresh state, and exposes no half-transition.
- Final opcode argument-storage protocol: `[:pos, ^, payload] | [:label, key,
  payload]`. L2 labels already have the latter normalized shape. At opcode entry,
  non-label postfix operands—including `[{:block}, ...]`—are wrapped as `:pos`;
  no list/string spelling metadata is retained.
- Every opcode transition returns exactly `Continue(next-state, inputs)`,
  `Dispatch(result)`, or `Error(diagnostic)`. Dispatch exposes no saturated opcode;
  remaining postfix application applies normally to its result.
- `$` constructs an effectful state machine; dispatch is the point where its state
  performs or initiates its designated action.
- `$SELECT` immediately validates `f` and `fn` parameter payloads before entering
  their continuation states; malformed binders/parameter lists hard-trap at routing.
- `$f` dispatch immediately specializes its `do:` body under the current lexical
  environment extended with its validated binder. The resulting function has closed
  executable code plus opaque capture; raw body syntax is not re-specialized per call.
- Executable `APPLY(A, B)` determines its callee `A` first and retains `B` as code
  until that callee's explicit input policy requires it. v0 eagerly evaluates only
  call operands of `$f`/`$fn` function values and routed payloads of `$load_file` /
  `$parse_term`; constructor binders/parameters/bodies are syntax. Raw `foo: bar`
  is inert data and hard-traps if used as an unrecognized v0 executable callee.
- `F_AFTER_PARAM` accepts only `[:label, {do}, body]` as syntax. It retains this
  label/body as code and dispatches closure construction; a positional bare block
  or another label hard-traps.
- ~~A function call must eliminate/instantiate all reachable `LOCAL` bindings before
  its lexical frame dies.~~ Superseded by `TERM(root; private E)` below.
- `TERM(root; private E)` is a real runtime family, like `OPCODE`. It retains the
  lexical environment of an open structural term, so function calls do not traverse
  or close-copy a body before L0 reduction. Triage treats `TERM` transparently: when
  it selects a root branch, that branch remains paired with the same `E`; direct
  `LOCAL(n)` lookup happens only when demanded in executable position.
- `TERM` environment is opaque/unforgeable. It is not a public structural child;
  public structural operations must not distinguish a term-wrapped list from its
  list root. Mutation is outside v0, so duplicated term captures are immutable.
- Semantically, open terms objectively carry `TERM(root, E)`. Implementation may
  thread `E` in CEK/CESK control without allocating wrappers on descent, but must
  preserve `(root, E)` as a runtime TERM value whenever it returns, is stored in
  structural data, becomes pending argument, or crosses a continuation/frame.
- Activation recursively resolves identifier syntax throughout an activated term,
  including inside lists and other nested structure, but does not evaluate those
  subterms unless a runtime rule says so. Only lexical binders resolve; any other
  identifier in executable activation traps. To forge arbitrary identifier syntax,
  construct `[{:id}, {name-bytes}]` as data and activate it only under a matching
  lexical binder.
- A list returns its instantiated proper L0 list structure without independently
  evaluating its elements.
- `FN_AFTER_PARAMS` accepts its body only as positional syntax
  `[:pos, ^, [{:block}, S1, S2, ...]]`; it rejects `do: expression`.
- v0 is effectful and curried: transition/source order determines host-effect and
  hard-trap order. Thus placing `$fn with:` before or after its block may observably
  differ; this is deliberate, not cosmetic.
- Full v0 opcode spec is authoritative as one nested tree per opcode state, with
  fields **ID**, **input shape**, **policy**, and **outcome** (including uniform
  diagnostics/traps). Mermaid is a readable projection only. Allocate numeric IDs
  in one pass after all v0 state names/transitions are approved.
- Recoverable backbone failure is ordinary result data:
  `[{:ok}, value] | [{:error}, diagnostic]`. User-defined errors and vf operate on
  these values. Non-local failure remains a separate opcode/runtime outcome.
- Execution-time protocol failures are uncatchable v0 traps. The host marks its
  execution state errored, retains an optional diagnostic term handle, and halts
  evaluation. This replaces the current textual `reducer_t._error`; invalid handle
  means no error. Diagnostic term contents remain deliberately unspecified.

## Diff review before L3 — 2026-07-11

Checked:
- `docs/spec/03_v0.md`
- `docs/spec/04_vf.md`
- `docs/spec/00_contents.md`
- session memory entries

Findings:
- Old 17:20 memory entry still used "v-family" and old open questions; marked it superseded in `.memsearch/memory/2026-07-11.md` by 17:35 vf entry.
- `docs/grammar.md` is intentionally deleted from docs and preserved at `.memsearch/memory/artifacts/grammar.md`.
- `docs/spec/03_v0.md` no longer contains `vstar` or in-file vf syntax section.
- Remaining `OP_FN*` mentions are notes about current implementation artifacts only.

## Layer 3 pass — 2026-07-11

Memory used:
- Searched memory for specialization, names dying, `LOCAL`, `OPCODE`, closed terms, and `do!`.
- Relevant hints: 2026-07-10 memory says specialization lowers normalized v0, kills names, creates runtime opcode objects; `LOCAL` runtime meaning remains Layer 5; `GLOBAL` was removed in favor of explicit application/value injection.

Output: `docs/spec/03_v0.md` extended with L3 Specialization.

Decision draft:
- L3 is activation-time specialization for terms in executable positions, not a user-called compile phase.
- No language-level quoting rule and no user-visible `{specialize}` function/opcode.
- A term has no intrinsic executable/data tag; role comes from position.
- Data position: ordinary value.
- Executable position: executable-position protocol activates term as syntax to execute.
- Any value may be placed in executable position; if it is not valid normalized executable syntax, activation errors.
- L3 input is term in executable position plus context supplied by that executable position.
- Example context source: `do! fn: [x] do ... end` supplies body binders; a term denoting identifier syntax for `x` resolves through that body context only when activated as that body.
- Same term outside executable position remains ordinary data.
- L3 abstract executable vocabulary: `DELTA`, `I64`, `BYTES`, `APPLY`, `LOCAL(n)`, `OPCODE(state)`.
- L3 performs context-dependent activation: syntax validation, lexical name resolution, native registry resolution, recognized executable construction conventions, cell emission/obtaining, diagnostics.
- Pure syntactic rewrites stay in L2.
- Names die in L3 activation; runtime has no name lookup.
- Unbound non-native names are activation/specialization errors.
- Output is closed; external values enter by explicit parameters/application, not hidden environment injection.
- No `GLOBAL` semantic node in current v0 spec.
- `LOCAL` and `OPCODE` are real L3 executable node families; exact L1 wire tags remain open.
- `do!`, if selected, defines executable-position protocols that supply activation contexts; `do!` itself is not runtime opcode.
- Runtime meaning of `LOCAL`, functions, closure representation, forcing, partial application belong to L5.

Open:
- Exact `$` executable-construction protocols.
- Exact `LOCAL`/`OPCODE` wire tags.
- Exact native registry inventory.
- Whether grouping tags are always erased before activation or can survive in explicit syntax-data protocols.

## `$` token and normalized application marker — 2026-07-11

Decision:
- `$` replaces provisional `do!` spelling.
- Lexer/parser treats `$` as special token admitted wherever an identifier callee can occur; it is not a separate constructor grammar.
- `$ f: x do: x + 1` parses by ordinary application/label rules, structurally like `foo f: x do: x + 1`.
- Every reserved block word immediately followed by `:` is a label in every labeled-argument position: `do:` and `end:` are labels; bare `do ... end` remains block syntax. Parser implementation needs this general disambiguation.
- L3 treats `$` as always bound only when activated in executable position; it selects executable-construction protocol. It is not runtime opcode.
- Replace old normalized-term application marker `{$}` with `{@}`.
- `{@}` is normalized term data and maps to `APPLY`; it does not conflict with source annotation `@[...]`, which exists only in L2 parser syntax.

Open:
- Exact `$` protocol states/labels and function construction semantics.

## Shared concrete syntax placement — 2026-07-11

Decision:
- Shared parser/normalization syntax is separate from v0 and vf semantics because
  it applies to every Vetochka language.
- Spec order is `01_introduction.md`, `02_concrete_syntax.md`, `03_v0.md`,
  `04_vf.md`.
- Renamed former `02_v0_layers.md` to `03_v0.md` and former `03_vf.md` to
  `04_vf.md`.
- Added user-facing `docs/spec/02_concrete_syntax.md`, rebuilt from valid old
  grammar material and reconciled with current decisions.
- Concrete syntax now specifies comments/trivia, tokens, ASI, full EBNF, `$`,
  reserved-word labels, v0/vf form ownership, and normalized term shapes using
  `{@}` application marker.


## Layer 4 pass — 2026-07-11

Memory used:
- Searched memory for `load_file`, `parse_term`, module graph, module discovery, and activation-time specialization.
- Relevant hints: old module notes separate module discovery from module semantics; activation-time specialization supersedes user-visible `{specialize}`.

Output: `docs/spec/03_v0.md` extended with L4 Backbone facilities.

Decision draft:
- Keep `load_file` and `parse_term` as v0 backbone facilities.
- `load_file :: path-bytes -> bytes | diagnostic` is the only required host effect in v0.
- `parse_term :: source-bytes -> normalized syntax term | diagnostic` is pure.
- `parse_term` accepts full shared Vetochka syntax and returns ordinary term data with no vf or executable meaning.
- No `specialize`, `execute`, or `eval` intrinsic exists in v0.
- Execution is the semantic event of term entering executable position, which supplies activation context and invokes L3 specialization, then L5 execution.
- v0 does not define module roots, import names, dependency resolution, package discovery, caching, authority policy, or path search rules.
- Programmers build module/code graph policy above L4 using `load_file`, `parse_term`, term transforms, and executable positions.

Rationale:
- Lowest useful module-system substrate is bytes-from-host plus parser-to-term.
- Keeping module policy out of v0 preserves user-defined module systems and vf languages.
- Making execution an intrinsic would create eval-like callable semantics and muddy executable-position activation.

Open:
- Diagnostic term shapes.
- Path byte interpretation and authority boundary for `load_file`.
- Concrete term representation for parsed normalized terms.

## Layer 5 entry point — 2026-07-10/11

**Question**

```text
What is the smallest Layer 5 machine that implements settled Layers 0–4?
```

L0–L4 deliberately stop before executable application semantics. L5 was opened
because it alone must say what happens when an activated `APPLY` reaches delta,
function, opcode, identifier, or other runtime family.

**Initial constraints**

- Machine owns executable application dispatch; L0 triage is only delta case.
- Keep no store/boxes for first machine; preserve proper tail calls.
- Prefer small CESK/CEK-like control with callee-first pending arguments.
- `$` (then provisional `do!`) is source/executable-position protocol, not a
  dedicated runtime syntax node.
- Opcode identity is state, not a per-family cell tag.
- Current `OP_FN*` cells are historical implementation artifacts.

**Initial open cruxes**

Function/opcode runtime representation; partial application; lexical binding;
strictness/forcing; machine registers/continuations; under/exact/overapplication;
effect order; tail calls; and body/block sequencing.

**What changed during this session**

- `TERM(root,E)` replaced close-copy lexical bodies.
- Model B nameful `ID` plus captured persistent E replaced semantic `LOCAL`.
- `$` replaced `do!`; `$f`, `$fn`, `$load_file`, `$parse_term`, and `to:` were
  introduced/settled in part.
- Generic `OPCODE(state, inputs; private)` replaced old `OP_FN0/1/2` design.
- Early "opcode is a stem; saturation is unobservable" projection is no longer
  sufficient: triage lenses now reopen the runtime-to-calculus boundary.

## Working v0 opcode state tree — 2026-07-11

This is the authoritative working shape. `Open` is deliberately unspecified.
IDs are allocated once all states are approved.

- `DOLLAR_SELECT`
    + Id: later
    + Input: `[:label, key, payload]`
    + Policy: `route`
    + Outcome:
        * `key = f`: validate `payload` as one binder identifier; retain syntax;
          `Continue(F_AFTER_PARAM)`
        * `key = fn`: validate `payload` as parameter-list syntax; retain syntax;
          `Continue(FN_AFTER_PARAMS)`
        * `key = load_file`: eagerly evaluate `payload`; `Dispatch({:ok, bytes} |
          {:error, diagnostic})`
        * `key = parse_term`: eagerly evaluate `payload`; `Dispatch({:ok, term} |
          {:error, diagnostic})`
        * otherwise: hard trap

- `F_AFTER_PARAM`
    + Id: later
    + Invariant: input history has validated `[:label, {f}, binder]`
    + Input: `[:label, {do}, body]`
    + Policy: `syntax`
    + Outcome: immediately specialize `body` under current lexical environment plus
      `binder`; `Dispatch` a one-argument eager function with closed executable code
      and opaque capture
    + Otherwise: hard trap

- `FN_AFTER_PARAMS`
    + Id: later
    + Invariant: input history has validated `[:label, {fn}, parameter-list]`
    + Input:
        * `[:pos, ^, [{:block}, S1, S2, ...]]`
        * `[:label, {with}, args]`
    + Policy:
        * block: `syntax`
        * with: Open — depends on exact eager evaluation semantics
    + Outcome:
        * block: Open — multi-argument closure specialization/dispatch details
        * with: `Continue(FN_HAVE_WITH)`
        * otherwise: hard trap

- `FN_HAVE_WITH`
    + Id: later
    + Input: positional block `[:pos, ^, [{:block}, S1, S2, ...]]`
    + Policy: `syntax`
    + Outcome: Open — closure construction and application of stored `with` arguments
    + Otherwise: hard trap

Notes:
- A `with:` following a dispatched `$fn` block applies to the resulting closure, not
  to `FN_AFTER_PARAMS`.
- v0 transition/source order determines host-effect and hard-trap order.
- Hard traps are host-visible halted execution with optional diagnostic term handle;
  user errors are `{:ok, value} | {:error, diagnostic}` data.

## Working v0 evaluation rules — 2026-07-11

This section is the concise authoritative record for the ongoing eagerness/CESK
pass. It supersedes conflicting earlier exploratory wording. Model B nameful TERM
lexical semantics is accepted; see `dialogue/lexical-reference-models.md`.

- A closure retains body plus opaque lexical capture. Function call runs body as
  `TERM(body, Ecall)`; it does not close-copy the whole body.
- `TERM(root, E)` is a real runtime family with opaque `E`. It is transparent to
  structural observation: selected delta branches retain the same `E`.
- Selected lexical-reference model: bodies remain nameful executable code;
  `TERM(ID(name), E) -> lookup(E, name)` when its active lens treats ID as
  executable usage. Raw syntax remains nameful data. Function closure stores raw
  body, binder names, and opaque defining E; call extends E with eager argument
  values.
- Resolved `LOCAL` IR is rejected for v0 semantics. It may later be a private cache
  optimization only if it preserves this nameful TERM behavior.
- `TERM(APPLY(f, x), E)` evaluates `TERM(f, E)` with pending `TERM(x, E)`.
- When delta accepts pending `TERM(arg, Ecaller)`, it stores that TERM unchanged;
  the argument retains caller environment. Delta application does not force it.
- Triage rule 1 discards its applied argument. It returns selected branch `x` under
  callee E unless `x` is already a TERM carrying its own environment.
- Triage rule 2 threads environments: branches `x` and `y` retain callee E; both
  duplicated uses of `z` retain the same immutable caller TERM environment.
- Triage rule 3 obtains argument triage view through TERM. Calee branches retain
  callee E; stem/fork children selected from argument retain argument E.
- Lists are proper L0 structure and do not evaluate their elements. An open list
  returns as TERM/list with its lexical environment retained.
- Semantic TERM association is objective. CESK may thread E without allocating a
  wrapper while descending, but allocates/preserves TERM on return, structural
  storage, pending argument, or continuation/frame crossing.
- Opcode operands are curried one at a time. Each state receives one raw TERM; when
  that state saturates, it may demand accumulated operands and dispatch next curried
  state or final result. This is not multi-field/batch application. Labels remain
  encoded term structure; lenses recognize and direct their traversal. CESK has no
  separate label-metadata state.
- Terms are application-first: `^ ^ ^` means `((^ ^) ^)`, not a ready source fork.
  Rule 0a/0b make it reduction-equivalent to leaf/stem/fork storage. `$f`/`$fn`
  likewise remain application chains until active demand evaluates them.
- Triage view is an internal L5 matching operation only for Rules 0a–3c. Every
  cell/runtime family must declare its root view; it is not a user-visible generic
  reflection API. Root view returns only `Leaf`, `Stem(child)`, or
  `Fork(left,right)`. Rule 3 remains language-level structural observation and does
  not force APPly reduction. Pure delta triage view recognizes application spines
  `APPLY(^,u)` as stem and `APPLY(APPLY(^,u),v)` as fork; an arbitrary APPly chain
  is not automatically a fork.
- REF is implicitly dereferenced before triage and is absent from triage semantics.
  ID resolves through captured TERM environment when the active lens traverses it as
  executable usage, including usage triage. A construction/declaration lens may
  preserve its syntax instead; `$f` binder traversal is the motivating case.
- Labels remain encoded term structure (current encoder uses `:label` tagged trees).
  Lenses recognize and direct their traversal. CESK has no separate label-metadata
  state.
- Postponed: traversal/provenance of triage-view children. An OPCODE public view may
  be list-like, but raw selected children would lose opcode/private-state provenance
  over later reductions; define this before exposing multi-step node-content view.

## Current structural baseline — 2026-07-11

**Decisions**

```text
L2 label AST -> encoded label term -> L5 lens traversal
```

There is no CESK label side channel. In the current encoder a label is a tagged
DELTA tree; a future cell ABI may change its storage but must preserve term-level
label data unless a later decision supersedes this.

```text
APPLY is application syntax/storage.
```

In executable control, `TERM(APPLY(f,x),E)` evaluates the callee first and retains
`TERM(x,E)` pending. Rule 3 does not execute that pending application. Pure triage
recognizes only delta-headed application spines as leaf/stem/fork:
`DELTA0`, `APPLY(DELTA0,u)`, and `APPLY(APPLY(DELTA0,u),v)`. When Rule 3 shape demand sees
top-level `APPLY`, L5 schedules it and retains a Rule-3 continuation; it does not
classify it by generic storage arity. ~~Pure L0 cell representation also has scoped
`≡₀` congruence.~~ Superseded: all semantics, including Rules 0a/0b, are expressed
as explicit transitions; any pure observational equivalence is a later derived
property, not a semantic primitive.

```text
lens is the added CESK traversal state.
```

A lens supplies current triage view and controls syntax-like versus eager-like
traversal. Runtime implementation may use a lens stack, but an escaped child must
retain sufficient provenance to resume its correct view. Exact lens composition and
OPCODE/closure projections remain open.

**Open immediate task**

Define, using strict cell constructors and grouping, the `$`/OPCODE lens for a raw
`APPLY` spine containing encoded `:label` terms. It must explain Rule 3 inspection
without executing the `$` chain, and preserve child provenance over later traversal.

## L5 partial spec record — 2026-07-11

Recorded settled L5 substrate in `docs/spec/03_v0.md` under **L5 Execution machine**
and added it to the contents. The spec uses the agreed hybrid: L0 term rewriting and
L5 small-step CESK-style transitions. Added environment-threaded delta/TERM/ID `RUN`/`RETURN`/`SHAPE` transition catalogue,
including `ARG`, `R3`, `RESHAPE`, and `RESTORE` continuations.
All transitions now use effect-labelled form `⟨C,E,K⟩ -> ⟨C',E',K'⟩ : ε`; current
rules emit `τ` or terminal `trap(diagnostic)`, while future host actions use
`host(operation,input,output)`. TERM installs E in machine state; structural results
reify one root TERM only at restoration boundary, while SHAPE captures selected children. The runtime-family
catalogue remains open; its previous `≡₀` proposal is superseded
by explicit operational transitions. It records callee-first pending application,
TERM/Model B lexical semantics, non-forcing Rule 3, pure delta application-spine
triage, REF transparency, generic OPCODE, labels as term data, and settled `$`/
`to:` surface protocol. It deliberately leaves configuration grammar, lenses,
normal-form stopping, exact opcode traversal, and unfinished function protocols
open.

## Continuation after operational triage pass — 2026-07-11

**Settled today**

- L5 is specified by explicit small-step transitions, not representation equivalence.
  `≡₀` was superseded; Rules 0a/0b remain operational transitions.
- Pure triage machine configuration is now
  `⟨RUN|SHAPE|RETURN|SHAPE-RETURN, E, continuation⟩`.
- Its rule catalogue includes delta Rules 0a/0b/1/2/3, callee-first `RUN-APPLY`,
  Rule-3 shape demand/resumption, TERM entry/restoration, and executable ID lookup.
- `E` is active machine state. Entering TERM installs it. A structural delta result
  that crosses its restore boundary gets one root TERM association; SHAPE attaches
  TERM only to selected structural children.
- Current C reducer regression identified: commit `e0b7b3f` changed explicit
  delta/value type whitelists to generic `cells_node_type_get_arity`, causing Rule 3
  to treat APPLY as fork. A TODO now marks this in `reducer/reducer_reducer.c`.
- Intended Rule 3 behavior for top-level APPLY is schedule application, retain
  `RESHAPE`/Rule-3 continuation, and resume on result; never classify by storage
  arity.

**Resume order**

1. Review the new TERM/restore transition table for semantic mistakes before adding
   more families.
2. Add explicit RUN/SHAPE rules for REF, then VALUEF/V leaf/stem/fork behavior.
3. Add SHAPE-ID plus unbound-ID trap; declaration treatment remains opcode-specific.
4. Specify closure and generic OPCODE execution/shape rules, then strict `$` lens
   over encoded `:label` application terms.
5. Finish `$f`/`$fn`, `to:` ordering/overapplication, `$cons`, code builders, and
   normal-form stopping rule.

## LENS continuation — 2026-07-11

**Decision**

`LENS` is implementation-defined and unforgeable. It has no source constructor and
is not a callable. The only way to materialize a LENS is for Rule 3 shape demand to
inspect a non-delta term.

```text
RUN(LENS(...)) -> strict error
```

**Proposed role**

A LENS carries the state/provenance needed to inspect a runtime term as a calculus
leaf, stem, or fork. Derived lenses must survive structural storage by Rules 0/1,
duplication by Rule 2, and capture/transport through `$f`; this replaces an ambient
or losable inspection stack.

Every non-delta runtime family that can reach Rule 3 requires a corresponding LENS
interpretation or an explicit error. Candidate: bytes/string lens exposes a
Tree-Book-style character-list encoding. Lenses themselves have no recursively
inspectable lens.

**L5 rule progress**

Added generic `SHAPE-MATERIALIZE-LENS`, `SHAPE-NO-LENS`, and
`SHAPE-LENS-LEAF/STEM/FORK` rules to the L5 catalogue. Direct delta, APPLY, and
already-LENS cases are disjoint from materialization. `RUN(LENS(...))` hard-traps.
Family-specific `lens-initial`/`lens-view` definitions remain open.

**Runtime ABI/object-system direction — open**

Runtime terms should use binary fixed-buffer or variable-length cell ABI, rather
than encode their private state in ordinary delta/tag trees. LENS defines their
stable calculus-facing inspection projection. Core v0 uses **one cell node type per
runtime state**, not generic core opcode/state-ID payload. `NS1`–`NS4` reserve all
future extension states, including auxiliary/lens states. None has a current payload
schema or semantics. Allocate cell encoding codes after state names stabilize.

Describe each state uniformly in an ABI registry:
`wire tag | state name | binary layout | lens constructor | CEK rule references`.
The registry must reference, not restate, CEK transitions/effects/traps. Added the
initial registry to `docs/spec/03_v0.md`, including current delta/value/APPLY/REF
node types, planned ID/TERM/LENS and current `$` state names, plus NS1–NS4.
Consolidate primitives, callables, closures, and opcodes into one systematic
runtime-object taxonomy; specifically settle whether a closure is an opcode kind or
distinct runtime family.

## Active settlement TODO — 2026-07-11

1. `to:` application: replace postfix calls for `$f`, `$fn`, and suitable v0
   callables; define normalization, curried saturation, eligible states, argument
   ordering, repeated `to:`, and overapplication.
2. Runtime families: settle ID, TERM, OPCODE, and closure public/private semantics,
   including triage lenses: root demand, root shape, child mapping, application
   interception, and escape/provenance. Define a systematic runtime-object taxonomy
   and ABI: primitives, callables, closures, and opcodes; decide whether closure is
   an opcode kind or separate family.
3. Normal form/demand: define v0 values, active APPLY, and L5 relation to unchanged
   L0 triage/branch-first strategy.
4. Dynamic tree construction: settle strict `$cons: el to: list`, ID/label builders,
   and generated-code routing through `$`.
5. Finish `$f`/`$fn`: parameter binding, body execution, block sequencing, `with:`.

## `to:` application — 2026-07-11

Decision:
- `to:` is ordinary labeled-argument syntax; parser gives it no universal call
  semantics.
- v0 runtime states choose whether it is accepted. `$f`/`$fn` closure values consume
  it as their call operand after constructor dispatch; `$cons` will consume it as its
  second construction operand.
- Postfix parenthesized calls have no v0 function-call semantics. Shared parser may
  retain them for vf.
- `$f`/`$fn` closure values accept only `[:label, {to}, argument]` as call operand;
  another label or positional operand hard-traps.

