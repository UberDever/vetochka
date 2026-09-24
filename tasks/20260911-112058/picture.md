# Picture: Vetochka as a whole

Run: "form a complete picture for the language, given its framing", 2026-09-03.
Mode: dialogue (verbose allowed, terse preferred). Not a spec; the spec is
hand-authored in `docs/new_spec/`.

Purpose: one consolidated, current description of every stage from the host rim
to emitted C, under the 2026-05-28 framing, so that (a) nothing stale is carried
silently, (b) every blank spot is named rather than papered over, and (c) the v0
CESK section can be written next against a settled surrounding picture.

## How to read this

Source precedence used throughout:

```text
.memsearch/memory/2026-08-30.md + docs/new_spec/   (current)
  > dialogue/                                       (settled directions)
  > .memsearch/memory/2026-07-11.md + docs/spec/    (July, partly dead)
  > project/                                        (obsolete banner carried)
  > reducer/ code                                   (behind the design)
  > project_history.md                              (framing origin)
```

Provenance: `(user)` ruled or stated it; `(agent)` finding or proposal, not
ruled; `(user+agent)` jointly developed and confirmed. Kind: `[fact]`,
`[assumption]`, `[preference]`, `[direction]`.

Status of a stage: **ruled** (a decision exists and nothing contradicts it),
**direction** (a lean exists, not a hard ruling), **open** (no shape yet).

Blank spots are named where they sit and collected again in Part III. No blank spot
is resolved here; Part IV sharpens them for ruling. Part II pictures vmeta and vsystem
with imagined programs — added on request, nothing in it is ruled.

## 0. Framing the picture respects

1. **Still C.** (user) [fact] Vetochka is a metalanguage and a new frontend for
   C, not a new runtime for C programs. No semantics leaks into emitted C; all
   semantics serves codegen. We are not inventing zig. `vsystem` has no runtime
   reflection. Origin: project_history 28.05.2026.
2. **Phase-separated porosity.** (user) [fact] The stack below. Internal porosity
   (cells, Rule 3 observation) exists at build time only; external porosity is the
   emitted C artifact. C enters the reflective world as data — ABI/layout/header
   models in cells — never as a competing runtime. Kell's dual-porosity question is
   answered by phase, not by mixing.
3. **v0 = triage + one construct.** (user) [fact] Triage is the foundation; the v0
   layer adds exactly one construct: effectful curried functions (`$fn`), the
   universal program-construction form. Opcodes are effect machinery kept close to
   triage. Binders are the sole nonlocality; `E` lives on binder nodes alone;
   closure pith shows syntax, never env. `$` marks every deviation from uniform
   application; not every `$` form is executable.
4. **Appliance = language + mandated passes.** (user) [fact] Substrate permissive,
   appliances restrict at their own compile step (Smalltalk-style deferral).
   Guarantees live in the appliance contract, not in the substrate.
5. **Reflection via pith duality.** (user) [fact] An observable pith of a node
   rebuilds that node. This is what makes intensionality reach past the calculus
   into runtime entities.

The stack, in the order the rest of this document walks it:

```text
host rim      load / parse / print / trap / exec, WASI-like
lexer         adjacency-classified operators, policy-free
grammar       arbitrary inert syntax, five application shapes
lowering      pure tree -> tree, inert cells, {@} always, metadata in-tree
v0 machine    triage rules 0-3 verbatim + opcodes; CESK; PTC; B-sequencing
vmeta         functional, compiler-oriented; v0 is its substrate
vsystem       compiler written in vmeta; emits ordinary C, classic ABI
C             ordinary, boring, inspectable
```

---

# Part I — Stages

## 1. Host rim

**Claim.** (user) [direction] The host owns the outermost ring: load, parse,
print, trap, exec. It is to be modeled on WASI — print/load/trap analogues.
`parse_term` stays native by choice (the host owns the lexer), though it is
technically implementable in-language. Bootstrap phases are host phases: load ->
parse -> transform on-machine -> feed root term. There is no bootstrap cycle and
no second handwritten evaluator: two interpreters drift.

**Status:** direction. The WASI modeling is a stated direction, not a surface.

**Open.**
- Host API surface: exact operation set, naming, and how it is selected
  (namespaced `$ns:`/`op:` opcodes vs a separate host table).
- `load_file` path-byte interpretation and authority boundary. (July open,
  never closed.)
- Diagnostic term shapes for host failures. (July open, never closed.)
- Parsed-cell ownership and lifetime across the host boundary.
- Whether `print` is one operation or a family.

**Notes.** (agent) [fact] July settled `load_file :: path-bytes -> {:ok,bytes} |
{:error,diagnostic}` and `parse_term :: source-bytes -> {:ok,cell-data} |
{:error,diagnostic}`, with expected failure as ordinary result data and
protocol/runtime failure as an uncatchable trap. Nothing since contradicts the
shapes; the WASI direction reframes their selection, not their contracts.
(agent) [fact] June's `initial_term` capability idea prefigures the WASI
direction.

## 2. Lexer

**Claim.** (user) ruled, ADR-1 (2026-08-30). Operator runs are classified by
adjacency; trivia breaks gluing:

1. glued to a preceding identifier — identifier continuation (`x-y` is one name);
2. glued to a preceding literal, `)`, or `]` — syntax error (`1-2`, `f(x)-y`);
3. else glued to its right neighbour — `op_prefix`;
4. else — `op_infix`.

Delimiters split the same way: `g_lparen` / `g_lbracket` / `g_string` / `g_dot`
when glued to an expression-ending token, primaries when free; a free `.` and a
free lone `:` are errors; a lone `:` glued to a label word is `g_colon`.

`identifier_continue` carries `? ' + - * / % < > ! &`; `=` and `|` were removed,
so `x = y` needs spaces and `|` stays free. A lone `:` is punctuation, never an
operator; multi-char operators may contain `:` (`::`). `$` and `~` are special
tokens, each heading exactly one grammar form; whitespace after them is
immaterial.

**Status:** ruled. Void if juxtaposition application is ever added — the ADR
names that as its revisit trigger.

**Open.** None semantic.

**Notes.** (agent) [fact] The lexer never consults an operator table and never
will; fixity is adjacency, meaning is a vf concern. Accepted costs: spaced prefix
`! x` is illegal, `!!x` is one operator.

## 3. Grammar

**Claim.** (user) ruled (2026-08-30, several passes). The grammar is
policy-free and admits arbitrary inert syntax. Load-bearing decisions:

- **No juxtaposition, anywhere.** Five application shapes carry everything:
  `f(args)`, `f[list]`, `f{bytes}`, `f x: v`, `f do end`.
- **Attachment principle (R1).** Every postfix has an owner. Tight postfixes
  (`.name`, `()`, `[]`, `{}`) must be glued — `f (a)` is not a call, and
  `[a (b)]` is a caught missing-comma. Spacing before loose postfixes (labels,
  `do...end`) is immaterial. Parens name the owner: `(f x: 1)(a)`.
- **Labeled expressions are first class.** `label: expr` is a standalone entry in
  comma lists, block lists, and groups; k-v lists are just `[x: 2, y: 4]`. No `|`
  sigil was spent. Payload is tight; nesting needs parens (`x: (y: 1)`).
- **Block promotion.** Bare `do...end` is a standalone entry;
  `entry ::= labeled_expression | block_argument | expression`.
- **Group erasure.** `(entry) -> entry`. Parens are purely syntactic — otherwise
  Rule 3 could observe parenthesization.
- **Opcode is one production.** `opcode ::= special_dollar labeled_expression`.
  The sigil grabs its selection, the rest is ordinary postfix machinery; bare `$`
  is unparseable by construction. The `$[ns, op]` bracket form is dead; namespaced
  natives spell `$ns: {core} op: {add} ...`.
- **Nyad.** `~[]` leaf, `~[x]` stem, `~[x, y]` fork; `~[x, y, z]` is a syntax
  error. Arity is structural, no numbers. `~` and `$` are analogous sigil heads:
  entry points into restricted syntactic families.
- **Mixed infix chains are allowed** and parse flat; vf analyzes them. There is no
  precedence and no operator sections.
- **ASI**: a newline becomes a virtual `;` in layout-active contexts (source and
  bare `do...end`) when `...` does not suppress it and the previous significant
  token can end an expression.

**Status:** ruled.

**Open.**
- Paren-statements `(a; b)` as a replacement for `do...end`: deferred to
  bootstrap-corpus evidence, explicitly not ruled by speculation. (user)
- Whether a corpus later justifies revisiting R1's spacious-DSL cost.

**Notes.** (agent) [fact] `f()` is equivalent to `f(~[])`. `do:`/`end:` are
ordinary labels. (user) [preference] Grammar assessments should weigh both
`vmeta` and `vsystem` as consumers.

## 4. Lowering

**Claim.** (user) ruled. Lowering is a **pure tree-to-tree transformation** that
emits inert data. It executes nothing: building `[{@}, f, x]` runs only the list
constructors, never the staged application. The current rule set:

```text
1. x                -> [{:id}, {x}]
2. $                -> [{:id}, {$}]      ;; opcode head, never alone
3. [a, b]           -> ~[a, ~[b, ~[]]]
4. (entry)          -> entry             ;; parens erased
5. f(x, y)          -> {@} ({@} f x) y
6. label: expr      -> [{:label}, {label}, expr]
7. do a; b end      -> [{:block}, a, b]
8. @[a, b] expr     -> [{:annot}, ~[a, ~[b, ~[]]], expr]
9. f[x, y]          -> {@} f ~[x, ~[y, ~[]]]
10. f{bytes}        -> {@} f {bytes}
11. prefix-op expr  -> [{:prefix}, {op}, expr]
12. x op y op z     -> [{:infix}, {op}, x, y, z]
13. base.name       -> [{:selector}, base, {name}]
```

A loose postfix lowers as plain application of its datum:
`f x: 1 -> {@} f [{:label}, {x}, 1]`. The former 6a/7a rules were removed as
derivable from that one line.

**Applications are markers always.** (user) [fact] Even bootstrap code is inert
and must be forced. The chain that forces this: Jay's programs are all values;
binders break that (unevaluated chunks); impurity breaks thunk-gating ergonomics
— unprotected effects would fire immediately, and forcing vsystem users to
`$fn: [_]`-abstract their code is unacceptable, because there an application
means *intent to apply*, a marker for analysis. Therefore `{@}` in data, always,
and a host-side apply-once `exec` as the only door.

**Status:** ruled, except metadata.

**Open.**
- **Metadata list shape.** (user) TODO in `02_syntax.md`: lowered nodes carry a
  metadata list after the type tag — `[{node}, [version, line, file, ...],
  ...payload]`. Spans/provenance live in-tree, not in side tables. Shape unchosen.
  This touches July's open span-ownership question.
- **How piths elide metadata.** Every lowered shape above grows a slot; the pith
  of each must not show it, or duality breaks against hand-written syntax.
- **Pith of an un-exec'd `{@}` datum** — see §9.

**Superseded, do not carry.** (agent) [fact] `{@}`'s old status line — "isn't part
of the syntax, only notation for the cells to come", "the single exception" — is
dissolved: `{@}` is an ordinary inert node type, observable like any other, still
not writable in source. `[{:group}]` is gone with rule 4. `:list` preservation was
rejected in July and stays rejected.

## 5. v0 machine — triage core

**Claim.** (user) ruled (bite-list, 2026-08-30). Adhere to the treecalc.us rules
0-3 exactly as written; evaluation **order** is pinned by the spec; core
evaluation is those rules plus identifier-related adjustments; the strategy is
**branch-first**, from the triage calculus. Nothing here is "eager".

```text
0a. ~[](x)                 -> ~[x]
0b. ~[x](y)                -> ~[x, y]
1.  ~[~[], x](y)           -> x
2.  ~[~[x], y](z)          -> (x(z))(y(z))
3a. ~[~[w, x], y](~[])     -> w
3b. ~[~[w, x], y](~[u])    -> x u
3c. ~[~[w, x], y](~[u, v]) -> y u v
```

The calculus itself has no literals, names, cells, opcodes, machine state,
effects, environments, modules, or execution policy beyond its reduction
relation. Confluence holds there; order becomes observable only with effects, and
must be pinned by the machine.

**Rejected here.** (user) The agent's proposal to make plain application
operand-eager with `$form` as the sole non-strict entry. Rejected: the rules
decide, not a strategy layered on top.

**Status:** ruled.

**Open.**
- (agent) [fact] The spec states the rules but does not yet *write down* the
  pinned order. "Pinned by the spec" is currently a ruling without a sentence.
  Naming this so the CESK pass writes it rather than assumes it.
- Rule 2 duplicates its operand raw; duplicated side effects are accepted by
  design (user). The interaction of that duplication with effect **order** needs a
  sentence in the CESK section, not just an acceptance.

## 6. v0 machine — CESK

**Claim.** (user) [direction] v0's executable semantics is given as a CESK
machine, small-step, effect-labelled:

```text
⟨control, E, kontinuation⟩ -> ⟨control', E', kontinuation'⟩ : effect
```

Pre-rulings that constrain it:

- **Forcing.** (user) Evaluation timing is defined by CESK control position only.
  There is no implicit forcing anywhere.
- **Sequencing.** (user) Candidate B: `do...end` statement execution is
  implemented in the opcode machinery itself, BLOCK-frame style — not desugared.
- **PTC.** (user) Proper tail calls are an obligation to enforce *while writing*
  the rules: frames only where a sub-result is needed; every other transition
  replaces control.
- **Machine-TERMs.** (user+agent) Because there is no substitution, every deferred
  evaluation pairs a term with an env: pending operands (callee-first retains the
  operand with `E-caller`), opcode raw inputs. These TERMs are machine-internal —
  K and opcode state — and are never observable data. "TERM everywhere in the
  machine, TERM only at binders in the data."

**What survives from the July L5 catalogue** (agent) [assumption], to be checked
line by line during the CESK pass:

- effect labels `τ` / `trap(diagnostic)` / `host(op, in, out)`; terminal
  `HALT(term)` / `TRAP(diagnostic)`;
- callee-first `RUN-APPLY`, `ARG` retaining exactly one pending operand;
- the `R3` continuation and shape-return protocol for rules 3a/3b/3c;
- `SHAPE-APPLY` **at machine level only**: a pending machine application in the
  operand is evaluated to expose its root shape, because 3a-c dispatch on root
  shape. (user) This is a semantic requirement, not a machine artifact;
- TERM entry/threading (`ENTER-TERM`, `RESTORE` as machine-internal env
  restoration).

**What is dead there** (agent) [fact]:

- `SHAPE-APPLY` over `{@}` **data**: markers never auto-execute;
- `RESTORE-DELTA1/2` promoting escaping structural results into TERM **values**:
  under the binder axiom the only env-carrying value is the closure;
- the `LENS` naming (renamed pith) and its "materialize a lens for every non-delta
  family" framing, insofar as it implied observation could force.

**Status:** open — this is the section the whole run exists to unblock.

**Open.**
- Control-position grammar: what forms of `control` exist now that `{@}` data,
  opcode states and closures are all values.
- Machine-TERM representation and lifetime; where exactly one is constructed and
  where one is discarded.
- Opcode state machines as CESK transitions (see §8).
- Frame inventory, and the PTC proof obligation for each.
- Normal form / stopping condition: what "no applys remain" means operationally.
- Effect and trap ordering statement, including Rule 2 duplication.
- Where `exec` enters (see §10).

## 7. v0 machine — binders, identifiers, environments

**Claim.** (user) ruled (binder-boundary axiom, 2026-08-30).

- **Axiom.** Binders are the sole nonlocality, side effects aside. `E` is a
  property of binder nodes alone; fragments are pure trees.
- **Identifiers are written references.** An identifier is nameful executable
  code; lookup happens on executable demand through the captured env (Model B,
  accepted 2026-07-11; `LOCAL`/resolved-slot IR rejected as semantics, permitted
  only as an observationally equivalent private cache).
- **Matrix** (`tasks/20241225-140425/identifier-context.md`):

  | | unbound | declared, no value | bound with value |
  |---|---|---|---|
  | undemanded | datum | datum | datum |
  | Rule 3 observes | raw datum | raw datum | observes the value |
  | executed | trap (ruled) | unreachable (invariant) | dereference |

  Position is dynamic: demand comes from the consumer. (user) There is no "data
  position" concept — everything in the language is data, executable constructs
  included.
- **Invariant.** Declared-no-value bindings exist only inside closure datums
  awaiting saturation: declarators force under the outer context, bindings install
  at saturation, the body runs only saturated. Assertable in the machine.
- **Scoping is lexical.** (user) `$fn`'s body runs under the closure's
  *declaration* environment extended by parameters — not `E-caller`. The old step-5
  "E-fn extends E-caller" wording was dynamic scoping and is fixed.
- **Escaping terms.** The three intents each have their own spelling, and none
  needs a new mechanism: embed values (ordinary application-built construction —
  `list(v, 1)` derefs `v` and yields a closed pure tree), keep behavior (return a
  thunk), keep open syntax (return the literal tree; it rebinds at its
  destination — a template feature). Ambient late binding on escaped terms is
  deliberately lost. The "splice/template opcode" idea was raised and withdrawn.

**Status:** ruled.

**Open.**
- Env representation in cells (implementation, not semantics): frames, sharing,
  lifetime.
- Recursive binding: `<~` clauses route through `$form`/fix (see §11), and
  `$form`'s shape is itself open (§8). Recursion is intended to be derived, not
  primitive.
- Whether any future env-touching opcode can preserve the declared-no-value
  invariant, and how that is asserted.

## 8. v0 machine — opcodes

**Claim.** (user) ruled in outline.

- Opcodes are machine-native operations extending triage: abstraction, numerics,
  bytes, structures, and host actions. They are **effect machinery kept close to
  triage**, not a second language.
- Selection is by label: `opcode ::= special_dollar labeled_expression`.
  Namespaced natives spell `$ns: {core} op: {add} ...`; partial selection is the
  curried initial state.
- Every opcode is **curried by construction** — the shape is just continuous
  application — and each state declares the policy for its next input. July's
  policies were `syntax` (retain the normalized term), `eager` (evaluate in caller
  context), and `route` (dispatch on a label). Common states:
  `Continue(next-state, inputs)`, `Dispatch(result)`, hard trap.
- **Code closures.** (user) An in-flight, partially applied opcode value *is* a
  closure: an env-carrying value whose pith shows syntax only — the selected
  opcode plus its inputs as written, envs hidden. Precision: declarator inputs
  like `$fn: [x]` *declare* `x` and reference no outer binder, so they need no env
  pairing; env pairing is needed only for retained inputs that can use outer
  bindings (raw `$form` payloads, computed declarators). Eager inputs are stored
  as values, no env.
- **Inert `$` forms.** (user) Not every `$` form is executable. Some are
  opcode-shaped data consumed at specialization time to build code; executing them
  traps. `$` marks specialness syntactically; executability is per-form.
- **`$fn` is the single universal program-construction form.** (user) [direction,
  debatable] Multi-arg, statements now, more later. `$f` is folded away — its
  July semantics (one arg, `do:` body, capture at construction) survives only as
  the *code-closure capture* reading, and its spelling is dead. `with:` is dead.
  Zero-parameter `$fn` executes immediately on receiving its body.
- **Callee-dependent raw arguments are accepted by design.** (user) The standing
  tension — that a first-class curried opcode applied via plain application can
  retain its operand raw, so argument-effect reasoning at an unmarked call site is
  callee-dependent — is closed by decision, not by removing the case. The
  raw-taking opcode is named `$form`, in continuity with the historical `{form}`.
  v0 is a target/substrate language, not a production surface.

**Status:** ruled in outline, open in shape.

**Open.**
- **`$form`'s exact shape.** Named, motivated (raw inputs; `<~` routes through
  it), unspecified. Blocking `let`'s recursive clause kind.
- Whether `$rec` exists as an opcode. "$rec dead" was ruled *for the let form*;
  "separate `$form`/`$rec` allowed where needed" was said of opcodes. Unresolved
  as stated.
- The opcode state table itself: which states exist, their inputs, policies, and
  outcomes, now that `$f`/`with:` are gone and `$fn` is universal.
- Native inventory and namespace layout under `$ns:`/`op:`.
- Under/exact/over-application of a saturated `$fn` (see §12 on ZINC).
- Which opcodes are inert-only, and how a reader tells.
- `$fn` argument order and repeated `to:` semantics. `to:` is settled as
  ergonomics — `f to: x` is really `f(to: x)`, a label datum applied — but the
  saturation questions July left open are still open.

## 9. Observation — Rule 3 and piths

**Claim.** (user) ruled in principle.

- **Duality.** A pith is the observable view of a node, generated on demand when
  the node enters Rule 3; if a pith is observable, a pith built the same way from
  scratch *is* such a node. This is what carries intensionality past the calculus
  into runtime entities.
- **Rule 3 does not force.** (user) It observes the operand as-is. Identifier
  dereference (bound-with-value) is the only indirection. Unreduced non-nyad
  operands are observed through their pith, with no evaluation.
- **But a pending machine application is evaluated.** (user) Rules 3a-c dispatch
  on root shape, so a pending application in control must be reduced far enough to
  expose its root shape ("корешок"). This is a semantic requirement. `{@}` **data**
  is never evaluated by it — markers are data with their own pith.
- **Only values are inspectable.** (agent, accepted) A live application is not a
  value. Final axiom form: APPLY exists only in machine control; in data,
  application is the `{@}` marker; `exec` is the only door, host-side.
- **Closure pith** shows the structural half — declarators and body syntax — and
  no env slot. Observation across a binder boundary is **re-quotation**: rebuilt
  syntax rebinds where it lands. Duality holds everywhere else. This is what
  dissolved the "pith duality vs E-privacy vs closure observability — pick two"
  tension.
- Observation never errors.

**Status:** ruled in principle, **open in table** — and this is the largest
concrete hole after CESK.

**Open — the pith table for non-nyad nodes.** `02_syntax.md` defines piths for
`nyad0/1/2` and then says "TODO: add more". Every one of these needs a pith:

- `{@}` — an un-exec'd application datum. List-shaped; must be defined *with*
  piths, i.e. its pith must rebuild an equal `{@}` node.
- `[{:id}, {name}]` — interacts with the identifier matrix: the *node* is a
  datum, the *reference* dereferences. Which of the two the pith shows must be
  said out loud.
- `[{:label}, {name}, payload]`, `[{:block}, ...]` — and the rest of the lowered
  tags (`:annot`, `:prefix`, `:infix`, `:selector`), which are lists and so may
  need nothing beyond list piths; that "may" is the open part.
- Literals: integer values and byte/string values. July floated a Tree-Book
  character-list encoding for bytes; never ruled.
- Code closures (§8) — declarators + body syntax, no env; the exact shape is
  unwritten.
- Opcode states — the "selected opcode + inputs as written" projection, per state.
- Metadata elision (§4): every pith above must skip the metadata slot.

## 10. exec — the host door

**Claim.** (user) ruled in direction, open in semantics.

- There is **no specialize stage**. July's "specialization compiles executable
  cells; names die there" holds no longer at all. Execution is just opcodes
  applying, apply after apply, values coming back until no applys remain. Names
  survive to runtime and die by env lookup.
- The machine does **not** ambiently walk `{@}`. It runs `exec` on a supplied
  **value** term containing `{@}`. Machine internals are otherwise unchanged by
  the marker decision; the only new thing is explicit `exec`.
- It is named `exec`, not `eval`, because of effects. It is apply-once and
  top-level. `parse_term` returns a value, never a reducible term.
- (user) [direction, not hard truth] `exec` is likely **not** user-accessible:
  bootstrapped code can interpret further code by ordinary application — applying
  one value onto another is exec-inside-the-language. `exec` stays thin host
  forcing machinery at the rim. A future in-language `$exec` would be an explicit
  supersession, not a discovery.

**Status:** direction; TODO already sits in `03_v0.md`'s CESK section.

**Open.**
- What `exec` does operationally: which configuration it constructs, what it
  accepts (any value? a value whose root is `{@}`?), what it returns, and what it
  traps on.
- Whether "apply-once" means one rule application, one saturation, or run-to-value.
- Its interaction with PTC and with the effect order statement.

**Note.** (user) [fact] Data dependency cannot stage impure eager code: an unbound
`_` is still a value (an id datum), an executed unbound id traps, and there is no
reduce-around-the-hole. Only control-based delay is sound. This is why `exec` is a
door and not a convention.

## 11. Bootstrap `let`

**Claim.** (user) ruled, final shape (OCaml-flavoured).

```text
let[x = 2, y := 10, z <- foo()] do ... end
```

- One form only. The loose block terminates it; `in:` is unnecessary and
  `in: do...end` would not even parse, since label payloads are tight.
- Clause kinds are infix operators: `=` plain bind, `<~` recursive (routed through
  `$form`/fix), extensible (`:=`, `<-`, ...). Clauses lower to
  `[{:infix}, op, id, expr]` list entries, so their shape is grammar-checkable.
- Scoping is sequential: each RHS sees outer bindings plus preceding clauses;
  later clauses shadow, so the OCaml `let x = x + 1` idiom works.
- A list-shaped clause is a **knot**: its members' RHSs see only pre-knot scope
  (plain — OCaml `and`, so the swap idiom works), or additionally the knot's own
  names when `<~` (OCaml `let rec ... and ...`, mutual recursion via a function
  list through one rec).
- `rec` and `and` stay unreserved; knots are structural, with no adjacency
  merging. `_ = expr` is the effect clause, `_` being an ordinary identifier.
  Blocks nested in clauses need parens. OCaml's `let f x y = body` sugar is
  deliberately not imported — `$fn` covers it, and that is vmeta taste.

**Status:** ruled.

**Open.**
- Blocked on `$form` (§8): `<~` names a routing that has no shape yet.
- Where `let` lives. It is spelled as bootstrap surface, not as a `$` opcode; the
  stale `$let: x is: 42` sketch still sits in `03_v0.md` as a core opcode. Whether
  `let` is a vmeta form, an inert-`$` form consumed at build time, or a core
  opcode is **not** ruled.

## 12. ZINC application edges

**Claim.** (agent) [assumption] The July basis review selected an extended CESK
for semantics and borrowed ZINC's `GRAB` calling convention as the closure-call
optimization: an argument spine peeled without evaluation, closures consuming all
available arguments, forcing only eager parameters. Its payoff is that exact and
over-applied calls allocate no intermediate partial closure; under-application
allocates one closure carrying the supplied bindings and a resume point.

**Status:** open, and its standing has changed under today's framing.

**Open — this is the "ZINC application edges" blank spot.**
- ZINC's benefit is stated for *closures with a known remaining arity*. Under
  today's picture there is one construct (`$fn`) but also opcode states that are
  themselves closures (§8) with per-input policies. Whether `GRAB` applies to
  both, one, or neither is unaddressed.
- Over-application: July said bind what fits, enter the body, then apply the body
  result to the rest, failing if it is not callable. That interacts with the
  ruling that plain application follows rules 0-3 branch-first; the seam is
  unexamined.
- Under-application: allocating a partial closure is exactly "construct a code
  closure", which §8 already has a name for. Whether these are the same object is
  unasked.
- The ZINC stack-mark / isolated argument stack during forcing (so a strict
  argument's evaluation cannot consume its caller's arguments) — needed or
  subsumed by "no implicit forcing"?
- July's caveat that stock ZINC assumes uniform call-by-value and argument-first
  compilation, while v0 must decide mode at runtime, still stands and still costs
  the fast path.

**Note.** (agent) [assumption] The honest reading is that ZINC is currently an
*optimization hypothesis inherited from July*, not part of the ruled picture. It
should be re-derived after the CESK rules exist, not before.

## 13. vmeta

**Claim.** (user) [direction] `vmeta` is a functional, compiler-oriented language.
v0 is its executable substrate and internal implementation language — and the only
place v0 arises. Nothing below is ruled; this section is wishful thinking with
each wish tied to the v0 mechanism that would back it.

| Wish | v0 mechanism that backs it | Status |
|---|---|---|
| Write compilers over program terms as ordinary data | lowering emits inert cells; `{@}` markers always; nothing fires while you build | ruled substrate, wish unruled |
| Inspect any term structurally | Rule 3 + pith duality (§9) | substrate ruled, pith table open |
| Construct terms without quotation | application-built construction: `list(v, 1)` derefs and yields a closed tree; literals keep names open | ruled |
| Quasiquote-ish ergonomics | at most vmeta sugar over constructor applications — explicitly *not* a v0 opcode | ruled as a non-mechanism |
| Bind and recurse readably | `let[...] do ... end` (§11), `$fn` (§8) | let ruled, `$form` open |
| Analysis libraries as discipline-libraries (`vrust`-style ownership, etc.) | appliance = language + mandated passes; conventions + passes, no new grammar | direction |
| Stage code deliberately | opaque staging via closure capture (`$fn: [g, y] do $fn: [_] do g(y) end end`); transparent staging via `{@}`-tagged trees built by ordinary constructors | verified, no new machinery |
| Read source and other files | host rim `load`/`parse` (§1) | direction |
| Run built code | `exec` at the rim, or ordinary application of one value onto another inside the language (§10) | direction |

**Status:** direction only. No shape yet.

**Open.**
- Everything about vmeta's own surface: it has none.
- Whether vmeta is an appliance in the §0.4 sense (language + mandated passes) or
  the bare substrate language.
- What vmeta's mandated passes would be, if any.
- Its error-message story. (user) [fact] Load-bearing build-time analysis makes
  error messages and analysis time *the product* — the other side of the C++
  template failure. Nothing addresses this yet.

## 14. vsystem

**Claim.** (user) [direction] `vsystem` is "buffed C": a more expressive frontend
to C that compiles directly to C and does more semantic analysis than a C compiler
does. Its compiler is written in vmeta and integrated into vsystem. It emits
separate, ordinary C.

Hard constraints already ruled:

- **No semantics leaks into emitted C.** All semantics serves codegen.
- **No runtime reflection**, by design. Internal porosity is build-time only.
- **Classic ABI**, same compiler, same debugger, same mental model.
- C enters the reflective world as data — ABI/layout/header models in cells.
- Appliances restrict at their own compile step: vsystem rejects, say, `-` in
  names semantically, leaving the grammar untouched.

| Wish | v0 mechanism that backs it | Status |
|---|---|---|
| Checked modules, explicit dependencies | host `load` + `parse` + programmer-written graph policy; v0 defines no module system on purpose | direction |
| Analyzable declarations | declarations are inert lowered trees; passes read them via Rule 3 | direction |
| Compile-time execution with guarantees | v0 machine at build time; `exec` at the rim | open (§10) |
| Code generation | term construction by application (§13) | ruled substrate |
| Project conventions enforced mechanically | appliance contract: mandated passes | direction |
| Imperative skin | five application shapes; `do...end` blocks; labels as the spacious DSL surface | ruled grammar |
| Programs observing line numbers etc. | metadata list in-tree (§4) | ruled intent, shape open |

**Status:** direction only. No shape yet.

**Open.**
- The entire vsystem surface, type story, and module story.
- What "more semantic analysis than your usual C compiler" concretely means as a
  mandated-pass list.
- How C ABI/layout/header models are represented as cells.
- The paren-statements question (§3) was deferred partly on vsystem's imperative
  skin; that evidence does not exist yet.

## 15. Emitted C

**Claim.** (user) [fact] Ordinary, boring, inspectable C, using the ordinary C
compiler, ABI, debugger, libraries, and mental model. The language exists to add
structure *around* C, not to replace it. The research question is whether a small
reflective term system can serve as both the compiler's internal representation
and the user-facing metaprogramming substrate while still producing that boring C.

**Status:** ruled as framing.

**Open.** Everything operational — there is no codegen work yet.

## 16. reducer/ — the current code

**Claim.** (agent) [fact] The C+Zig implementation is behind the design and is
partly stale, but it holds important bits. Newer records override it.

**What is there, verified:**

- `cells_*`: compact typed cell storage. Wire table in `cells_impl.h` publishes
  `DELTA0/1/2` (0x80-0x82), `VALUEF0/1/2` (0x83-0x85, i64), `VALUEV0/1/2`
  (0x86-0x88, bytes), `APPLY` (0x89), `OP_FN0/1/2` (0x8A-0x8C), and `REF14`/`REF62`
  as two wire layouts of one semantic `REF`. The header states that published
  MASK/CODE/layout combinations form a stable bytecode ABI.
- `source_tokenize.c` + `source_ast.c`: text -> `source_tree_t`. Node types are
  the pre-2026-08-30 grammar: `LABELED_ARGUMENT` (not labeled *expression*),
  `TIGHT_POSTFIX`/`LOOSE_POSTFIX`, `IMPLICIT_DELTA`, and no nyad.
- `bytecode_source.c`: `source_tree_t` -> cells. Emits `:id`, `:block`, `:label`,
  `:annot`, `:prefix`, `:infix`, `:selector`, `:group`, proper lists, and real
  `APPLY` cells via `new_apply`.
- `reducer_reducer.c`: a flat stack loop implementing rules 0a-3c, dispatching by
  node arity.
- Zig tests (`test_reducer.zig`, `test_source.zig`, `test_bytecode.zig`), CI on
  zig 0.15.1, plus a sanitized run.

**What is stale, and how:**

- **Nyad rename not applied.** The lexer takes `^` as the delta token and `~` as a
  prefix operator — exactly inverted from the ruled `~[...]` family.
- **ADR-1 not applied.** No adjacency classification, no `op_infix`/`op_prefix`,
  no glued-delimiter token classes.
- **Grammar drift.** Labeled expressions are not first-class entries; blocks are
  not promoted; `:group` is still emitted where rule 4 now erases parens; `$` has
  no opcode production.
- **`OP_FN0/1/2`** are an older experiment, not target semantics. They predate the
  whole opcode-state design and are explicitly not registry states.
- **Known Rule 3 regression.** `reducer_reducer.c:235` carries
  `TODO(v0-l5)`: generic arity dispatch classifies `APPLY` as a fork. Introduced
  by `e0b7b3f`. Correct behavior is to request the argument's triage view and, if
  the top node is a pending machine application, evaluate it and resume Rule 3
  through a continuation — never classify by storage arity. Under today's ruling
  this is *sharper*, not weaker: `{@}` data must not be executed by Rule 3 at all,
  while a pending machine application must be.
- **No machine.** There is no CESK, no environments, no ID/TERM/closure cell
  types, no `$` opcode states — the whole of §§6-8 is unimplemented.
- **Pipeline shape.** Normalization and cell encoding are fused in
  `bytecode_source.c`. July proposed splitting them; today's "no specialize stage"
  ruling changes what that split would even be — likely just text -> cells
  encoding plus inert-`$` expansion.

**What is worth keeping:**

- The cell ABI discipline and the wire table's shape.
- `source_formatting.c`'s canonical formatting of parsed trees — a
  cell-to-text serialization is still required and still unspecified.
- The arity/saturation pattern in `OP_FN*` as an existence proof of shared
  leaf/stem/fork saturation, even though the tags die.
- The tests and CI, which are the only executable check on any of this.

**Status:** stale where it contradicts §§2-9; useful as substrate.

**Open.**
- Whether the cell wire table is re-cut before or after the CESK rules exist.
- Canonical cell-to-text serialization.
- Whether to fix the Rule 3 regression now (it is a correctness bug against both
  the July and the current reading) or as part of the machine rewrite.

---

# Part II — Illustrated appliances (imagined)

Added on request after the first pass: §§13-14 gave wish tables and no use cases.
This part pictures vmeta and vsystem with full-blown illustrative programs.

Rules of this part, stated once:

- **Everything here is (agent) [assumption].** Nothing is ruled. It is imagination
  rooted in the ruled v0 design, pulled toward zig, Odin, Jai, Rust, C++ (and their
  cousins C3, D, Nim) where they fit the 2026-05-28 framing.
- **The grammar is untouched.** Every keyword-looking word — `module`, `struct`,
  `fn`, `if`, `match`, `case`, `defer`, `try`, `comptime`, `build`, `rule` — is an
  ordinary identifier head. Every form is one of the five application shapes plus
  labels, blocks, annotations, infix, selectors, nyads. The appliance gives them
  meaning; the parser knows none of them.
- **Every snippet was hand-checked against `docs/new_spec/02_syntax.md`.** Where the
  grammar bit, I kept the bite and say so in §II.15 — that friction is exactly the
  bootstrap-corpus evidence the paren-statements deferral (§3) asked for.
- **Each use case names** its inspiration, the v0 mechanism that backs it, the pass
  that consumes it, and the C that comes out.
- Conventions I chose for readability, none ruled: `head name: [params] label:
  payload do ... end` as the declaration shape; `::` for ascription; `:=` for
  declare-and-bind in vsystem statements (Odin/Jai), `let[...] do ... end` in
  vmeta; `T[n]` arrays and `ptr(T)` pointers as ordinary applications; typed
  literals through the byte-payload shape (`f64{0.5}`, `u32{0xEDB88320}`), since
  the grammar has only decimal integers and byte strings; `@[pub]`, `@[test]`,
  `@[layout: soa]` as modifiers, since `pub fn` would be juxtaposition.

## II.1 vsystem — a vector module

Inspiration: zig (explicit allocators, `comptime T: type`), Odin (`$T`), the user's
own 28.05 sketch. Not C++ templates: instantiation is a term function run at build
time with readable names out, and the failure line is "analysis time and error
messages are the product", not deferred meaning.

```vetochka
module(vec) do
    use[std.mem, std.assert]

    struct Vec: [T] do
        data :: ptr(T)
        len :: usize
        cap :: usize
        alloc :: ptr(Allocator)
    end

    @[pub] fn init: [T, alloc :: ptr(Allocator)] ret: Vec(T) do
        Vec(T)[data = null, len = 0, cap = 0, alloc = alloc]
    end

    @[pub] fn free: [T, v :: ptr(Vec(T))] do
        if(v.data != null) do mem.free(v.alloc, v.data) end
        v.data = null
        v.len = 0
        v.cap = 0
    end

    fn reserve: [T, v :: ptr(Vec(T)), wanted :: usize] ret: bool do
        if(wanted <= v.cap) do return(true) end
        doubled := v.cap * 2
        new_cap := max(wanted, max(doubled, 8))
        new_data := mem.alloc_array(v.alloc, T, new_cap)
        if(new_data == null) do return(false) end
        if(v.data != null) do
            mem.copy(new_data, v.data, v.len)
            mem.free(v.alloc, v.data)
        end
        v.data = new_data
        v.cap = new_cap
        true
    end

    @[pub] fn push: [T, v :: ptr(Vec(T)), value :: T] ret: bool do
        if(v.len == v.cap) do
            if(!reserve(T, v, v.len + 1)) do return(false) end
        end
        v.data[v.len] = value
        v.len = v.len + 1
        true
    end

    @[pub] fn get: [T, v :: ptr(Vec(T)), i :: usize] ret: ptr(T) do
        assert.that(i < v.len)
        v.data[i]
    end
end
```

A bare parameter without ascription (`T`) is a build-time parameter — zig's
`comptime T: type` without the keyword: the appliance sees a declarator with no
`::` and treats it as a term-level input. A call site `vec.init(i32, alloc)`
instantiates. The last expression of a `fn` body is its value, as in `$fn`.

Reading the lowered shape: `fn init: [...] ret: Vec(T) do ... end` is just
`{@} ({@} ({@} fn [{:label},{init},params]) [{:label},{ret},type]) [{:block},...]`.
The fields are `[{:infix}, {::}, data, ptr(T)]` entries in a block. The struct
literal `Vec(T)[data = null, ...]` is a bracket application of a list of `=`
infix entries. Nothing here is special to the parser.

What comes out, for `Vec(i32)`:

```c
typedef struct Vec_i32 { int32_t *data; size_t len; size_t cap; Allocator *alloc; } Vec_i32;
Vec_i32 vec_init_i32(Allocator *alloc) { return (Vec_i32){ .data = NULL, .len = 0, .cap = 0, .alloc = alloc }; }
void    vec_free_i32(Vec_i32 *v);
bool    vec_push_i32(Vec_i32 *v, int32_t value);
int32_t *vec_get_i32(Vec_i32 *v, size_t i);
```

Only the instantiations the program uses are emitted; a memo pass dedups them.
Names are boring on purpose — C++ mangling is the anti-example.

Backing mechanism: lowering (§4) gives inert declarations; the vsystem
monomorphization pass is a vmeta `$fn` over terms (II.10) that constructs the C
declarations by application (§7, embed-values intent); `@[pub]` is in-tree data
by lowering rule 8 and drives what lands in the header.

## II.2 vsystem — tagged unions and exhaustive match

Inspiration: Rust enums-with-data and exhaustive `match`; Odin's tagged `union`.

```vetochka
union Shape: [] do
    circle: [r :: f64]
    rect: [w :: f64, h :: f64]
    empty: []
end

fn area: [s :: Shape] ret: f64 do
    match(s) do
        case circle: [r] do f64{3.14159} * r * r end
        case rect: [w, h] do w * h end
        case empty: [] do f64{0} end
    end
end
```

Arms are `case` heads with a loose label and a loose block. I first wrote
`circle: [r] -> expr` and it parses, but as one labeled expression whose payload is
the whole infix chain `[r] -> f64{...} * r * r` — a mixed operator chain that the
matcher would then have to re-split. `case pattern do body end` is what the
grammar actually hands you; see §II.15.

What comes out:

```c
typedef struct Shape {
    enum { Shape_circle, Shape_rect, Shape_empty } tag;
    union { struct { double r; } circle; struct { double w, h; } rect; } u;
} Shape;

double area(Shape s) {
    switch (s.tag) {
    case Shape_circle: { double r = s.u.circle.r; return 3.14159 * r * r; }
    case Shape_rect:   { double w = s.u.rect.w, h = s.u.rect.h; return w * h; }
    case Shape_empty:  { return 0; }
    }
}
```

Drop the `empty` arm and the mandated exhaustiveness pass refuses:

```text
src/shape.vt:8:5: match on Shape is not exhaustive: missing `empty`
```

Backing mechanism: `union` body entries are labeled expressions (C6), so the
variant list is grammar-checked k-v data; the `match` pass walks the block via
Rule 3 (§9); exhaustiveness is a set difference over two inert lists. The C
`switch` has no default — the guarantee lives in the appliance contract (§0.4),
and the emitted C is checkable by any C compiler's `-Wswitch` too.

## II.3 vsystem — `defer`, error unions, `try`

Inspiration: zig (`!T`, `try`, `defer`, `errdefer`). The C++ lesson taken is RAII's
*ordering*, not destructors: scope-exit code is a syntax rewrite, no runtime.

```vetochka
fn read_all: [alloc :: ptr(Allocator), path :: str] ret: !bytes do
    f := try(fs.open(path))
    defer do fs.close(f) end
    size := try(fs.size(f))
    buf := try(mem.alloc_bytes(alloc, size))
    errdefer do mem.free(alloc, buf) end
    try(fs.read_exact(f, buf))
    ok(buf)
end
```

zig's exact `!bytes` spelling falls out of the grammar for free: `!` glued right is
`op_prefix`, so `ret: !bytes` lowers to `[{:label}, {ret}, [{:prefix}, {!}, bytes]]`
and the type pass reads "error union of bytes". zig's `try expr` does not — it is
juxtaposition — so it is `try(expr)`. Rust's postfix `?` is impossible (an operator
glued after `)` is a lexical error), which is fine: `try(...)` is the same thing
with the parens the language wants anyway.

What comes out (defers expanded at every exit; nothing hidden):

```c
typedef struct { int err; bytes val; } err_bytes;

err_bytes read_all(Allocator *alloc, str path) {
    err_file _t0 = fs_open(path);
    if (_t0.err) return (err_bytes){ .err = _t0.err };
    File f = _t0.val;
    err_size _t1 = fs_size(f);
    if (_t1.err) { fs_close(f); return (err_bytes){ .err = _t1.err }; }
    size_t size = _t1.val;
    err_bytes _t2 = mem_alloc_bytes(alloc, size);
    if (_t2.err) { fs_close(f); return _t2; }
    bytes buf = _t2.val;
    err_void _t3 = fs_read_exact(f, buf);
    if (_t3.err) { mem_free(alloc, buf); fs_close(f); return (err_bytes){ .err = _t3.err }; }
    fs_close(f);
    return (err_bytes){ .err = 0, .val = buf };
}
```

Backing mechanism: purely §4-level — the `defer` pass rewrites
`[{:block}, ...]` to `[{:block}, ...]` with the deferred blocks inlined before each
`return`/fall-off (II.12 shows that pass). This is the 28.05 constraint verbatim:
"new features that boil down to simple syntax rewrites". `B`-sequencing (§6) is
what makes a block a first-class unit to rewrite.

## II.4 vsystem — layout as data (SOA)

Inspiration: Jai's and Odin's `#soa`. The framing's "C enters the reflective world
as data — layout models in cells" made concrete.

```vetochka
@[layout: soa] struct Particles: [] do
    pos :: vec3
    vel :: vec3
    life :: f32
end

fn tick: [ps :: ptr(Particles), dt :: f32] do
    i := 0
    while(i < ps.len) do
        ps.pos[i] = vec3.add(ps.pos[i], vec3.scale(ps.vel[i], dt))
        ps.life[i] = ps.life[i] - dt
        i = i + 1
    end
end
```

What comes out:

```c
typedef struct Particles { vec3 *pos; vec3 *vel; float *life; size_t len; } Particles;
static inline vec3  *Particles_pos (Particles *p, size_t i) { return &p->pos[i];  }
static inline float *Particles_life(Particles *p, size_t i) { return &p->life[i]; }
/* ps.pos[i] in the loop body is emitted as p->pos[i]; the AOS spelling never existed in C */
```

Backing mechanism: rule 8 puts the annotation in-tree as
`[{:annot}, ~[[{:label},{layout},soa], ~[]], struct-term]`; the layout pass reads
it via Rule 3 and owns both the type emission and the rewrite of field access on
that type. The annotation is data a program can observe — which is the ruled point
about metadata and annotations being ordinary observable trees (§4).

## II.5 vsystem — build-time execution

Inspiration: Jai `#run`, zig `comptime`, D CTFE. Difference from all three: there
is no second evaluator. The build-time machine *is* the v0 machine the whole
appliance already runs on.

```vetochka
crc_table := comptime(crc32_table(u32{0xEDB88320}))

fn crc32: [data :: bytes] ret: u32 do
    c := u32{0xFFFFFFFF}
    i := 0
    while(i < data.len) do
        c = xor(crc_table[band(xor(c, data[i]), 255)], c >> 8)
        i = i + 1
    end
    xor(c, u32{0xFFFFFFFF})
end
```

with `crc32_table` an ordinary vmeta function (`>>` is an operator run; `^` is
not an operator char, hence `xor`/`band` as heads):

```vetochka
crc32_table = ($fn: [poly] do
    map($fn: [n] do
        fold(range(8), n, $fn: [_, c] do
            if((c & 1) == 1) do xor(c >> 1, poly) end else: (c >> 1)
        end)
    end, range(256))
end)
```

What comes out:

```c
static const uint32_t crc_table[256] = { 0x00000000, 0x77073096, 0xEE0E612C, /* ... */ };
```

The C type comes from the value's shape (a 256-list of `u32`); the appliance can
demand an annotation instead if it prefers explicitness. Hex literals go through
the byte-payload shape because the grammar only has decimal integers — deliberate,
the head interprets.

Backing mechanism: the pass sees head `comptime`, applies the inner *value* term
on the machine — "applying one value onto another is exec-inside-the-language"
(§10, user direction) — and serializes the result as a C initializer by
application-built construction (§7). `exec` at the rim is used once, by the
driver (II.13), never here.

## II.6 vsystem — whole-program passes: tests and reflection tables

Inspiration: Jai's compiler message loop (a metaprogram sees every declaration);
Rust `#[test]`/`#[derive]`. The framing constraint: vsystem has **no runtime
reflection**, so reflection is *emitted into C as tables*, at build time.

```vetochka
@[test] fn push_grows: [] do
    v := vec.init(i32, test_alloc)
    assert.that(vec.push(i32, v, 1))
    assert.that(v.len == 1)
end

@[reflect] struct Event: [] do
    user_id :: i64
    kind :: EventKind
    path :: str
end
```

What comes out:

```c
/* tests_main.c */
static const test_entry tests[] = { { "push_grows", push_grows }, /* every @[test] in the program */ };

/* event_reflect.c */
static const field_info Event_fields[] = {
    { "user_id", FIELD_I64, offsetof(Event, user_id) },
    { "kind",    FIELD_ENUM, offsetof(Event, kind) },
    { "path",    FIELD_STR,  offsetof(Event, path) },
};
```

A C program can iterate `Event_fields` — boring arrays, debugger-visible, no
magic. That is "C-like external porosity" delivered by "reflective internal
porosity" (porosity memory), with the phases kept separate as ruled (§0.2).

Backing mechanism: after all modules are lowered, the driver holds one term; a
registry pass collects every `[{:annot}, ~[[{:id},{test}], ~[]], fn-term]` by Rule
3 walk and constructs the table by application. No new mechanism; the only
requirement is that annotations stay in-tree, which rule 8 guarantees.

## II.7 vsystem — the build is a program; conventions are passes

Inspiration: `build.zig` (the build is code in the language), Odin's explicit
allocators as a *taste* (no implicit `context`), and the project's original itch:
conventions enforced mechanically.

```vetochka
build do
    appliance(vsystem)
    modules[{src/vec.vt}, {src/shape.vt}, {src/main.vt}]
    passes[defer, errors, layout, tests, reflect]
    rule no_raw_malloc: {mem}
    rule alloc_first_param: []
    emit{out/}
end
```

`rule name: payload` registers a mandated check; a rule is a vmeta `$fn` from term
to list of diagnostics. What the user sees when it fires:

```text
src/main.vt:12:5: no_raw_malloc: `malloc` called outside module `mem`
src/vec.vt:31:5: alloc_first_param: `reserve` takes ptr(Allocator) in position 2, expected 1
```

Backing mechanism: this is the appliance contract (§0.4) as a literal file:
language + mandated passes. `passes[...]` and `rule` are just application shapes
over inert data; the driver (II.13) folds them. Line numbers come from the
in-tree metadata list (§4), which the user already accepted programs may observe.

## II.8 vsystem — an ownership discipline as a library (`vrust`)

Inspiration: Rust ownership, deliberately reduced to the *shape* of the idea: a
pass over inert terms, mandated by an appliance, with plain pointers in the C.

```vetochka
fn consume: [b :: own(Buffer)] do drop(b) end
fn peek: [b :: ref(Buffer)] ret: usize do b.len end

fn main: [] ret: i32 do
    b := make_buffer(alloc, 64)
    consume(b)
    peek(b)
    0
end
```

```text
src/main.vt:7:5: ownership: `b` was moved into `consume` at 6:5 and used here
```

What comes out is C with `Buffer *` everywhere; the discipline left no trace,
which is the point. Honest note: Rust's checker is years of work; the claim here
is only that it *fits* — a flow-sensitive walk over `[{:block}, ...]` with
`own`/`ref` as ordinary type heads — and that this is where "error messages and
analysis time are the product" (user, §13) stops being a slogan.

## II.9 vsystem — C interop, two levels

```vetochka
include{stdio.h}

fn main: [] ret: i32 do
    c.printf({hello %d\n}, 42)
    0
end
```

Level 1 (cheap, available on day one): `include{...}` passes through as
`#include <stdio.h>`; `c.printf(...)` is emitted verbatim and unchecked. Byte
strings have no escapes, so `\n` reaches the C literal as two bytes and C
interprets it — a happy accident worth keeping.

Level 2 (the real thing): typed use needs a *model* of the header in cells — the
"C enters the reflective world as data" line. That needs a C declaration parser
written in vmeta, or a host op behind `$ns: {c} op: {parse_header}`. Open, large,
and the single biggest practical dependency vsystem has.

## II.10 vmeta — a pass is a function over terms

Now the other language. vmeta is where II.1-II.9's passes are written. Collecting
every function name in a module:

```vetochka
let[
    fn-names <~ ($fn: [term, acc] do
        match(term) do
            case spine: [[{:id}, {fn}], [{:label}, name, _] | rest] do
                fn-names(rest, cons(name, acc))
            end
            case [head | rest] do fn-names(rest, fn-names(head, acc)) end
            case _ do acc end
        end
    end)
] do
    fn-names(parse(load{src/vec.vt}), [])
end
```

Three things to notice, all of them the ruled design showing through:

- The pattern `[{:id}, {fn}]` is written as a bracket list of two byte strings and
  is *the same tree* that the identifier `fn` lowers to (rule 1). "As above, so
  below": patterns over code are just code, no quotation.
- `spine:` is a matcher convention that flattens an application spine
  `{@} ({@} ({@} f a) b) c` into `[f, a, b, c]` before matching. Writing it needs
  the pith of a `{@}` datum — **O1 is the first thing vmeta trips over.**
- `<~` is the recursive clause kind, routed through `$form`/fix — **which has no
  shape yet (§8).** Two of the run's top blank spots, reached within ten lines.

Backing mechanism: `match`/`case` are Rule 3 dispatch over piths (§9) dressed as
heads; `[head | rest]` is an infix chain the matcher reads as cons; `_` is an
ordinary identifier the matcher treats as wildcard. Nothing was added to v0.

## II.11 vmeta — building C as terms, then printing

```vetochka
let[
    c-field = ($fn: [f] do
        match(f) do
            case [{:infix}, {::}, fname, ty] do cfield(c-type(ty), fname) end
        end
    end),
    c-struct = ($fn: [name, fields] do
        cdecl typedef: [cstruct(name, map(c-field, fields)), name]
    end)
] do
    print(c-render(c-struct({Point}, [x :: f64, y :: f64])))
end
```

Output: `typedef struct Point { double x; double y; } Point;`

The C AST is one more inert term family (`cdecl`, `cstruct`, `cfield` are heads,
nothing else) — same Rule 3, same piths, same construction by application. The
printer `c-render` is the only place bytes are assembled; everything above it is
trees. Inspiration: none needed; this is what "compiler-oriented functional
language" means operationally.

## II.12 vmeta — defining a pass and mandating it

The `defer` rewrite from II.3, as a vmeta pass:

```vetochka
let[
    exits = ($fn: [block] do
        ;; positions of every return(...) plus the fall-off end
        ;; body elided
        ~[]
    end),
    expand-defer = ($fn: [block] do
        ;; [{:block}, ...] -> [{:block}, ...]: collect `defer do ... end` statements,
        ;; drop them, splice their bodies (reverse order) before each exit
        ;; body elided
        block
    end)
] do
    pass name: {defer} stage: {before-emit} run: expand-defer
end
```

Given the lowered input of a two-statement function body

```text
[{:block}, [{@}, defer, [{:block}, close(f)]], [{@}, return, x]]
```

the pass yields

```text
[{:block}, [{@}, [{@}, block-seq, close(f)], [{@}, return, x]]]
```

— or whatever the appliance's sequencing head is; the point is that both sides are
inert trees and the rewrite is a `$fn`. `pass name: ... stage: ... run: ...` is a
head with three labels; `passes[defer, ...]` in the build file (II.7) names it.
That registration *is* the appliance contract line for `defer`.

## II.13 vmeta — the driver: bootstrap on the machine

```vetochka
let[
    load = ($fn: [path] do $ns: {host} op: {load} path: path end),
    parse = ($fn: [src] do $ns: {host} op: {parse_term} src: src end),
    print = ($fn: [bytes] do $ns: {host} op: {print} bytes: bytes end),
    passes = [defer-pass, errors-pass, layout-pass, tests-pass],
    run-all = ($fn: [term] do fold(passes, term, $fn: [p, t] do p.run(t) end) end)
] do
    print(emit-c(run-all(parse(load({src/main.vt})))))
end
```

This is the bootstrap phase list from §1 — load -> parse -> transform on-machine
-> print — written as a program. The host's `exec` (§10) is used exactly once,
by the rim, to put *this* term into control; everything after is ordinary
application. The `$ns: {host} op: ...` spellings are the §1 blank spot made
visible: the host API surface has a shape ruling (`$ns:`/`op:` labels) but no
inventory.

## II.14 vmeta — staging without quasiquote

Generating one accessor per field, using nothing but the three escape intents
from `tasks/20260830-165615/binders.md`:

```vetochka
let[
    accessor = ($fn: [struct-name, field] do
        fn-decl name: concat(struct-name, {_get_}, field.name) ...
                params: [[{:infix}, {::}, [{:id}, {self}], ptr(struct-name)]] ...
                body: [{:block}, [{:selector}, [{:id}, {self}], field.name]]
    end)
] do
    map($fn: [f] do accessor({Point}, f) end, fields-of(point-decl))
end
```

In the `body:` payload, `field.name` is **embedded** (dereferenced at construction
— intent 1), while `[{:id}, {self}]` is a literal id datum that stays **open** and
rebinds where it lands (intent 3). Same line, two intents, no quasiquote, no
unquote — exactly the "no splice opcode needed" correction (2026-08-30). The `...`
at line ends are the grammar's line continuations: a label chain across lines
would otherwise be cut by ASI.

## II.15 What the exercise showed (corpus evidence)

Grammar friction met while writing II.1-II.14, kept rather than smoothed. This is
the evidence the paren-statements deferral (§3) said should decide things.

1. **Juxtaposition is the constant tax.** `pub fn`, `try expr`, `defer expr`,
   `else do` all die. Answers that fell out: `@[pub] fn`, `try(expr)`,
   `defer do ... end`, `else: (do ... end)` — or, for `if`/`else`, pairing two
   adjacent statements inside the block, which an appliance can do because it sees
   the whole `[{:block}, ...]`. Neither is bad; both are worth a ruling on taste.
2. **Attributes on their own line don't work.** `@[test]` NL `fn ...` gets a
   virtual `;` after `]`, orphaning the annotation. Same line, or `...`. Rust/zig
   habit will fight this daily; either accept `@[x] fn` as the house style or
   exempt an annotation's `]` from the ASI expression-ending list.
3. **Multi-line label chains need `...`** (II.14). Same root cause as 2.
4. **Match arms.** `pat -> expr` parses, but as one labeled expression with a mixed
   infix payload. `case pat do ... end` is what the grammar gives; it reads fine.
5. **Mixed infix chains are under-specified in lowering.** Rule 12 shows a single
   `{op}` for `x op y op z`; what `x :: T = v` lowers to is not written. Every
   appliance above wanted to know. **New blank spot**, added to Part III.
6. **ASI inside `do ... end` nested in `(...)`.** The spec's "bare `do...end`" is
   ambiguous about whether a block inside parens re-activates layout. I assumed
   innermost wins and used explicit `;` where it mattered. Needs one sentence.
7. **`!T` is free, `?` is impossible, `^` is not an operator.** Prefix ops give
   zig's error-union spelling for nothing; postfix ops don't exist; xor is a head.
   All consistent with the ruled token classes — just worth knowing.
8. **Typed literals through `f{bytes}`** (`f64{0.5}`, `u32{0xEDB...}`) work and
   read well. Deliberate, and vindicated.
9. **Dependence on blank spots is immediate.** `spine:` matching needs the pith of
   `{@}` (O1); recursive passes need `$form` (§8); the driver needs the host
   inventory (§1) and `exec` (§10). vmeta cannot be sketched further than II.10
   without O1 and `$form`; that orders the next run.

What was deliberately *not* taken from the inspirations, with the framing reason:

- C++ templates as the generic mechanism — deferred meaning is the named failure
  smell; instantiation here is a term function with readable output.
- Odin's implicit `context` — hidden parameters violate explicitness; the
  allocator is a parameter and a *rule* enforces where it goes (II.7).
- Rust traits as runtime dispatch — vtables are semantics leaking into C; only
  build-time static dispatch fits "no semantics in emitted C".
- Jai's "run arbitrary C at compile time" — allowed in spirit (II.5), but at v0
  speed, which is precisely the analysis-time cost the appliance contract must
  own rather than hide.

# Part III — Consolidated blank spots

Ordered by what blocks what. Nothing here is resolved.

1. **Pith table for non-nyad nodes** (§9) — `{@}`, `{:id}`, `{:label}`,
   `{:block}`, the remaining lowered tags, integer and byte literals, code
   closures, opcode states, metadata elision. Blocks: observation semantics in
   CESK, and the duality claim itself.
2. **`exec` semantics** (§10) — the door exists, its shape does not. Blocks: the
   CESK entry point.
3. **`$form` exact shape** (§8) — blocks `let`'s `<~` clause kind (§11) and the
   raw-input policy story.
4. **Metadata list shape, and pith elision of it** (§4) — reshapes every lowered
   node; blocks the pith table.
5. **CESK rules proper** (§6) — control position, machine-TERMs, opcode state
   machines, frames, stopping condition, effect order. The main event.
6. **Host API surface and bootstrap phases** (§1) — load/parse/print/trap
   inventory, WASI shaping, diagnostics, ownership.
7. **ZINC application edges** (§12) — whether the borrowed calling convention
   still fits one-construct v0 with opcode-closures.
8. **vmeta and vsystem** (§§13-14) — direction only, no shape.
9. **reducer/ relation** (§16) — which parts are re-cut, which are repaired, when.
10. **Pinned evaluation order, written down** (§5) — ruled but unwritten.
11. **Where `let` lives** (§11) — vmeta form, inert-`$` form, or core opcode.
12. **`$rec` as an opcode** (§8) — dead for `let`, "allowed where needed" for
    opcodes; unresolved as stated.
13. **Lowering of mixed infix chains** (§II.15) — rule 12 shows one `{op}`; what
    `x :: T = v` lowers to is unwritten, and every appliance sketch wanted it.
14. **ASI in a `do...end` nested in `(...)`** (§II.15) — "bare" is ambiguous; one
    sentence settles it.

# Part IV — Opens sharpened for ruling

The run's working rules forbid silent resolutions, so these are posed, not
decided. Each carries an agent lean where one exists; none is a ruling.

**O1. Pith of `{@}`.** (agent) [direction] Define it list-shaped, so
`[{@}, f, x]` observes as a three-element list whose pith rebuilds an equal
`{@}` node — duality then falls out of the list piths already implied by rule 3
of lowering. Cost to check: it makes application syntactically indistinguishable
from a 3-list under observation, which may be exactly right (it *is* data) or may
lose the marker's analytic value. **Needs ruling.**

**O2. Pith of `[{:id}, {name}]`.** (agent) The identifier matrix says observation
follows the reference (bound-with-value observes the value). The pith question is
different: what does the *node* `[{:id}, {x}]` observe as when it is a subterm of a
value nobody is dereferencing? Lean: as a two-element list, with dereference being
a property of the reference in demand position, not of the pith. **Needs ruling**,
because the two readings differ exactly where duality is tested.

**O3. Metadata elision rule.** (agent) [direction] State it once, generically:
piths skip the metadata slot for every lowered node type, so a pith-rebuilt node
carries fresh (or empty) metadata rather than the original's. Consequence to
accept: a rebuilt node is not byte-identical to its source, only pith-identical.
**Needs ruling**, since it defines what "duality" means precisely.

**O4. `exec`'s contract.** (agent) Three sub-questions that should be ruled
together: (a) what it accepts — any value, or one whose root is `{@}`; (b) what
"apply-once" means — one rule application, one saturation, or run-to-value;
(c) whether a trap inside `exec` halts the host or returns a diagnostic. Lean on
(c): halt, matching the existing uncatchable-trap ruling. **Needs ruling.**

**O5. Where `let` lives.** (agent) [direction] Inert-`$` form consumed at build
time reads most consistent with §8's inert-`$` ruling and keeps `$fn` the only
executable construction form. **Needs ruling**, and it decides whether
`03_v0.md`'s core-opcode section keeps a `let` entry at all.

**O6. ZINC's standing.** (agent) [direction] Demote it explicitly to
"optimization hypothesis, re-derive after CESK". It is currently the only place in
the picture where a July implementation decision is carried as if current.
**Needs ruling** only in the sense of confirming the demotion.

**O7. The Rule 3 regression in `reducer/`.** (agent) It is a real bug under every
reading, old and new. Lean: fix independently of the machine rewrite, since the
test suite is the only executable check the project has. **Needs ruling** because
it spends implementation time on code that may be re-cut.

**O8. `$rec`.** Restate the scope of "dead": dead as a `let` variant, undecided as
an opcode. **Needs ruling** or an explicit "leave open".

# Part V — Targeted spec edits

The user hand-authors `docs/new_spec/`; these are targeted line edits only, each
one a supersession or a named blank spot — no new semantics.

Applied in this run:

1. `docs/new_spec/02_syntax.md` — `{@}`'s status line rewritten: it is an
   ordinary inert node type, observable like any other, not writable in source.
   (Supersedes "isn't part of the syntax / the single exception", per the
   marker-doctrine ruling.)
2. `docs/new_spec/02_syntax.md` — the pith section's `TODO: add more` replaced by
   the named blank list from §9, so the hole is enumerated rather than implied.
3. `docs/new_spec/02_syntax.md` — metadata TODO extended with the pith-elision
   consequence (§4, O3).
4. `docs/new_spec/03_v0.md` — dangling `TODO:` in the reduction-rules line pointed
   at the CESK section; note added that order is pinned by the machine (§5).
5. `docs/new_spec/03_v0.md` — inert-`$` note added to the opcodes section (ruled
   2026-08-30, recorded as applied but absent from the file).
6. `docs/new_spec/03_v0.md` — `$let` sketch marked superseded by the ruled
   bootstrap `let[...] do ... end`, with the "where does `let` live" question
   named (O5) rather than answered.
7. `docs/new_spec/03_v0.md` — `$form` named as an open core form in the opcodes
   section, with its blocking relation to `let`'s `<~` (§8, §11).
8. `docs/spec/02_concrete_syntax.md` — obsolescence banner, matching the ones
   already on `docs/spec/04_vf.md` and `tasks/20260711-131255/v0_layers.md`.

Added after the first pass, on request: Part II (illustrated appliances). It is
dialogue, not spec; no spec file was touched for it. Its §II.15 feeds two new blank
spots (13, 14) into Part III.

Not applied, deliberately:

- No CESK rules were written. That is the next run, and it needs O1-O4 ruled.
- No pith definitions were written, only the list of missing ones.
- `docs/spec/01_introduction.md` and `docs/spec/03_v0.md` were left unbannered:
  01 is superseded wholesale by `docs/new_spec/01_intro.md`, and 03 is still the
  usable CESK seed (§6) rather than a stale claim. Naming this so the omission is
  visible.
