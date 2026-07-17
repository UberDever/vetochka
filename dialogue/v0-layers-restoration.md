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
pass. It supersedes conflicting earlier exploratory wording. The lexical-reference
model remains open; see `dialogue/lexical-reference-models.md`.

- A closure retains body plus opaque lexical capture. Function call runs body as
  `TERM(body, Ecall)`; it does not close-copy the whole body.
- `TERM(root, E)` is a real runtime family with opaque `E`. It is transparent to
  structural observation: selected delta branches retain the same `E`.
- Selected lexical-reference model: bodies remain nameful executable code;
  `TERM([{:id}, {name}], E) -> lookup(E, name)` on executable demand. Raw syntax
  remains nameful data. Function closure stores raw body, binder names, and opaque
  defining E; call extends E with eager argument values.
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

