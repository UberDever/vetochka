# v0 CESK Evaluation Draft

Temporary design note. This is not yet the language specification.

## Goal

Give `v0` a small lexical functional core while retaining:

- explicit binary application;
- triage-calculus values and reduction;
- byte and integer values;
- native operations;
- inspectable cell encoding;
- strict functions and non-strict forms.

The runtime is named `CESK`. It extends the usual CESK shape with a pending-argument
spine. It uses explicit control, lexical environments, a store, and continuations.
Each callable decides whether a pending argument is forced or retained as a `TERM`.

The store is semantically required by stable mutable boxes shared across closures.
Initial immutable implementation may leave the store unused and behave as CEK.

The closure calling convention is ZINC-derived: argument spine, `GRAB`,
underapplication, overapplication, and tail-call reuse. Initial implementation may
evaluate tree-shaped specialized IR directly. Linear bytecode and call fast paths
can be added later without changing core semantics.

## Layers

### 1. Source and CST

`source_tree_t` is host parser output. It contains concrete syntax structure and
token spans. It has names, punctuation, grouping, labels, and other source forms.
It is neither executable code nor a calculus value.

Example source:

```vetochka
{fn} args: [x] do
    add(x, 1)
end
```

### 2. Bootstrap syntax term

Current source normalization produces nameful data:

```vetochka
[{:block},
    {$} {$} {$} {fn}
        [{:label}, {args}, [[{:id}, {x}]]]
        [{:block},
            {$} {$} [{:id}, {add}] [{:id}, {x}] 1
        ]
]
```

This representation is syntax data. Tags such as `:block`, `:id`, and `:label`
describe source structure. They have no direct CESK behavior.

Bootstrap consumes syntax data when compiling `v*`. The bootstrap program itself
must already be compiled to specialized `v0` IR by a small seed compiler or loaded
from previously generated cells.

### 3. Specialized v0 IR

Specialized IR is executable, lexical, and name-free:

```text
e ::=
    DELTA
  | I64(value)
  | BYTES(value)
  | APPLY(e, e)
  | LOCAL(index)
  | GLOBAL(index)
  | OPCODE(id)
```

Meanings:

- `DELTA` is triage `^`.
- `APPLY(f, x)` is textual `{$} f x`.
- `LOCAL(n)` reads lexical environment slot `n`; index `0` is nearest binder.
- `GLOBAL(n)` reads a statically resolved global slot.
- `OPCODE(id)` identifies a native operation leaf such as `{fn}` or `{form}`.

`{fn}` and `{form}` remain ordinary application chains over primitive leaves. The
specializer recognizes their protocols so it can resolve binders inside their bodies:

```text
{fn}(params)(body)(with)
    -> APPLY(APPLY(APPLY(OPCODE(fn), params), body), with)

{form}(params)(body)(with)
    -> APPLY(APPLY(APPLY(OPCODE(form), params), body), with)
```

Current constructor arity is three: primitive leaf accepts `params`, then `body`,
then `with`; applying resulting opcode fork to third field dispatches and creates a
runtime closure. Exact future meaning of `with` remains open; specializer may supply
`^` when no source protocol needs it.

After resolution:

```vetochka
{fn} args: [x] do
    add(x, 1)
end
```

is conceptually:

```text
APPLY(
    APPLY(
        APPLY(
            OPCODE(fn),
            PARAMS([eager])),
        BODY(APPLY(APPLY(GLOBAL(17), LOCAL(0)), I64(1)))),
    DELTA)
```

where global slot `17` denotes `add`.

`{fn}` and `{form}` are compiler-recognized constructors in executable position.
They become opcode leaves during specialization. The byte strings `{fn}` and
`{form}` remain ordinary inert `BYTES` values when used as data.

### 4. Cells

Cells are compact storage for specialized IR and ordinary term data. They are not
the argument stack, lexical environment, continuation stack, or runtime closure.

Specialized IR is a semantic node vocabulary, not necessarily a second permanent
arena. A specializer may build it transiently and immediately encode it into cells.
Conversely, cells may also contain inert bootstrap syntax lists. Being stored in
cells does not by itself make a root executable; only a specialized root may be
activated by CESK.

Existing mappings remain:

```text
DELTA       -> DELTA0
I64(n)      -> VALUEF0(n)
BYTES(bs)   -> VALUEV0(bs)
APPLY(f, x) -> APPLY(left=f, right=x)
```

Proposed semantic cell nodes:

```text
LOCAL(n)      -> LOCAL(payload=n)
GLOBAL(n)     -> GLOBAL(payload=n)
OPCODE(id)    -> OPCODE0(id)
```

Application saturates opcode families through their ordinary cell shapes:

```text
OPCODE0(id) @ x    -> OPCODE1(id, x)
OPCODE1(id, x) @ y -> OPCODE2(id, x, y)
OPCODE2(id, x, y) @ z
    -> family-specific machine/native dispatch
```

`{fn}` and `{form}` dispatch after receiving their third construction field and
create runtime closures containing resolved body code and current lexical
environment.

`LOCAL` and `GLOBAL` payloads are resolved indices. Source identifier bytes do not
remain on the normal executable path.

Applicable cell families retain current homogeneous saturation shape:

```text
TYPE0        leaf
TYPE1(x)     stem
TYPE2(x, y)  fork
```

Source and specialized IR name only `TYPE0` primitive leaves. `TYPE1` and `TYPE2`
are produced by application; there is no separate source syntax for constructing a
partially saturated opcode.

This does not limit user functions to three parameters. `TYPE2 @ z` completes one
opcode dispatch stage. Dispatch may create a closure with an arbitrary parameter
descriptor or return another callable stage.

Triage inspection may branch on this outer shape for delta, value, opcode, closure,
or runtime-wrapper families. Family tag and payload remain distinct from shape.
`APPLY` is the exception: its two children record pending application syntax; it is
evaluated by CESK and is not itself a saturated fork value.

## Runtime Values

Runtime values are not all serialized cell nodes:

```text
value ::=
    CELL(root)
  | NODE0(family, payload)
  | NODE1(family, payload, value)
  | NODE2(family, payload, value, value)
  | CLOSURE(mode, params, body, with, environment)
  | TERM(code, environment)
  | BOX(location)
  | PRIMITIVE(id, supplied_arguments)
```

Where:

- `CELL(root)` references inert cell data such as integer or bytes.
- `NODE0`, `NODE1`, and `NODE2` preserve generic leaf/stem/fork shape.
- Delta values are `NODE0(delta)`, `NODE1(delta, x)`, and
    `NODE2(delta, x, y)`.
- `CLOSURE` is lexical code plus captured environment.
- `TERM` is suspended executable code plus its lexical environment.
- `BOX(location)` is a stable reference to a mutable store slot.
- `PRIMITIVE` is semantic shorthand for an opcode family plus supplied arguments:
    `OPCODE0`, `OPCODE1(x)`, or `OPCODE2(x, y)`. Implementation may use the cell
    nodes directly rather than allocate another object.

Exact allocation is deferred. Cells may back both immutable code/data and mutable
store slots, but these are different semantic roles. A box location must remain
stable while its store entry changes. Initial implementation may keep runtime
objects in a separate heap while cells hold code and ground data.

Runtime heap allocation does not imply invisibility to triage. A closure or `TERM`
may be represented by a shape-preserving cell wrapper containing a runtime handle.
Rule 3 can inspect wrapper leaf/stem/fork shape. Explicit operations are required to
inspect family tags or payloads such as code, lexical environment, or host
capabilities.

### TERM Is Not Syntax

These are different:

```text
SYNTAX(source_tree_handle)
TERM(code_root, lexical_environment)
```

`SYNTAX` preserves source names and token structure. Bootstrap traverses it through
raw-tree host operations.

`TERM` preserves executable meaning. Its code may contain `LOCAL(0)`, so removing
its environment would make it invalid. A `{form}` receives a `TERM`, not source
syntax.

This distinction does not remove `TERM` from calculus inspection. It only says that
the visible runtime object is a wrapper around `(code, environment)`, not identical
to environment-free source syntax. Structural reification of captured code and
environment is not implicit. A future operation may close or quote one explicitly,
but this draft defines only shape inspection, passing, and forcing suspended terms.

First bootstrap boundary:

```text
parse
  -> source_tree_t
  -> optional source-tree transforms
  -> specialize_v0(source_tree_t, lexical_scope)
  -> executable cells
  -> CESK
```

`{form}` exists only on executable side of this boundary. It cannot recover source
names, punctuation, grouping, or token spans from its `TERM`.

No general macro protocol or `TERM -> source_tree_t` reification is needed for v0.
No calculus decision remains here. Later bootstrap wiring needs ordinary host API
details:

- ownership and lifetime of `source_tree_t` handles;
- minimal inspect/build operations exposed to bootstrap code;
- specializer input, lexical-scope input, output root, and error contract.

These are implementation contracts, not blockers for evaluation semantics.

## Machine State

Use this abstract state:

```text
<control, environment, store, arguments, returns>
```

Components:

```text
control     = EVAL(code) | RETURN(value)
environment = lexical vector/frame chain
store       = stable location -> runtime value
arguments   = stack of TERM values
returns     = stack of continuation frames
```

This is CESK with a ZINC-derived argument spine. A later bytecode interpreter may
split `control` into code pointer and accumulator.

Environment entries may be direct immutable values or `BOX(location)` values.
Direct values avoid needless store allocation. Mutable or recursive bindings use
stable locations. `FN` parameters receive forced values; `FORM` parameters receive
`TERM` values.

Store operations are explicit:

```text
box(v, S)          -> BOX(a), S[a := v]       where a is fresh
box_get(BOX(a), S) -> S[a]
box_set(BOX(a), v, S)
                   -> UNIT, S[a := v]
```

If later syntax provides transparent mutable variables, specialization may compile
their reads and writes to these operations. Immutable `LOCAL(n)` remains direct.

### Weak-Head Evaluation

`eager` means evaluate until control returns a runtime value:

```text
whnf(TERM(code, E)) -> run code under E until RETURN(value)
whnf(value)         -> value
```

`APPLY` is executable code, not a runtime value. Reaching weak-head form may
therefore execute several head `APPLY` nodes or follow several `TERM` indirections.
It stops as soon as it obtains a literal, closure, primitive, or leaf/stem/fork
value. It does not normalize children, closure bodies, or inert cell payloads.

Executing exactly one `APPLY` is a separate `step` operation, not weak-head
evaluation. v0 does not need to expose `step` initially.

## Core Evaluation Rules

Notation:

```text
t @ u
```

means application of runtime value or computation `t` to suspended argument `u`.

### Constants

```text
<EVAL(DELTA), E, S, R>
    -> <RETURN(NODE0(delta, none)), E, S, R>

<EVAL(I64(n)), E, S, R>
    -> <RETURN(CELL(i64(n))), E, S, R>

<EVAL(BYTES(bs)), E, S, R>
    -> <RETURN(CELL(bytes(bs))), E, S, R>
```

### Lexical References

```text
<EVAL(LOCAL(n)), E, S, R>
    -> <RETURN(E[n]), E, S, R>

<EVAL(GLOBAL(n)), E, S, R>
    -> <RETURN(G[n]), E, S, R>

<EVAL(OPCODE(p)), E, S, R>
    -> <RETURN(PRIMITIVE(p, [])), E, S, R>
```

Reading a local does not automatically force it. A form parameter therefore remains
a first-class `TERM` until passed to a strict callable or explicitly forced.

### Application Spine

Application evaluates its callee first and suspends its argument:

```text
<EVAL(APPLY(f, x)), E, S, R>
    -> <EVAL(f), E, TERM(x, E) :: S, R>
```

For:

```vetochka
f(a, b)
```

normalized as:

```vetochka
{$} {$} f a b
```

the stack reaches:

```text
TERM(a, E) :: TERM(b, E) :: S
```

Thus arguments are consumed left-to-right.

Each `APPLY` contributes exactly one pending `TERM`. Peeling a left-associated
application spine collects all currently visible argument terms before callable
execution:

```text
APPLY(APPLY(APPLY(f, a), b), c)
    -> EVAL(f), arguments = [TERM(a), TERM(b), TERM(c)]
```

This collection is eager only about finding the spine. It does not evaluate `a`,
`b`, or `c`.

### Callable Dispatch

`APPLY` decomposition and callable dispatch are separate:

```text
eval APPLY(f, x)
    -> push TERM(x, E)
    -> eval f

return callee with pending argument
    -> dispatch by callee kind, family, and saturation
```

Conceptually:

```text
apply(callee, pending_TERM):
    TERM    -> evaluate captured callee code; preserve pending_TERM
    CLOSURE -> choose closure parameter mode
    NODE    -> choose next_mode(family, payload, saturation)
    other   -> non-callable error

prepare(term, pending_TERM)  -> pending_TERM
prepare(eager, pending_TERM) -> force pending_TERM to WHNF

accept(NODE0(F), prepared)   -> return NODE1(F, prepared)
accept(NODE1(F,x), prepared) -> return NODE2(F, x, prepared)
accept(NODE2(F,x,y), prepared)
                              -> dispatch F with x, y, prepared
```

Delta and `{fn}`/`{form}` constructor stages use `term`; eager native stages use
`eager`. Saturation never changes family `F`. Only family dispatch may return a
value from another family. Examples:

```text
DELTA0 @ x -> DELTA1(x) -> DELTA2(x, y)
DELTA2(x, y) @ z -> triage rule -> selected/generated value

OPCODE0(fn) @ params -> OPCODE1(fn, params)
OPCODE1(fn, params) @ body -> OPCODE2(fn, params, body)
OPCODE2(fn, params, body) @ with -> CLOSURE(...)
```

These arrows are successive applications, not one application changing three
states. For delta:

```text
first argument:  DELTA0       @ a -> DELTA1(a)
second argument: DELTA1(a)    @ b -> DELTA2(a, b)
third argument:  DELTA2(a, b) @ c -> triage dispatch
```

If dispatch returns a callable while argument stack still contains terms, same
process immediately continues. Thus:

```vetochka
{$} {$} {$} {$} {fn} params body with arg
```

flows as:

```text
OPCODE0(fn)
  @ params -> OPCODE1
  @ body   -> OPCODE2
  @ with   -> CLOSURE
  @ arg    -> force according to eager mode, then enter body
```

Current reducer dispatches only on arity. That works while every fork is delta, but
fails once opcode and runtime families exist. CESK application dispatch must first
inspect family, then use arity inside that family.

If a suspended term appears in callee position, evaluate it while preserving pending
arguments:

```text
<RETURN(TERM(code, Et)), E, t :: S, R>
    -> <EVAL(code), Et, t :: S, R>
```

If argument stack is nonempty, returned value must be one of:

```text
CLOSURE
PRIMITIVE
NODE0 | NODE1 | NODE2
TERM
```

Any other value in callee position is a runtime non-callable error.

### Abstraction Construction

`{fn}` and `{form}` use normal opcode saturation. Their three construction fields
are received as terms because parameter descriptors and body are code/data, not
expressions to execute during closure creation:

```text
next_mode(fn, 0..2)   = term
next_mode(form, 0..2) = term
```

When saturated constructor fork receives third field:

```text
dispatch(fn, [params, body, with], E)
    -> CLOSURE(eager, params, body, with, E)

dispatch(form, [params, body, with], E)
    -> CLOSURE(term, params, body, with, E)
```

Returned closure retains lexical environment `E`. Parameter descriptors and body
have already been specialized, so body references use `LOCAL` and `GLOBAL` rather
than runtime name lookup.

### Closure GRAB

Closures do not use `TYPE0 -> TYPE1 -> TYPE2`. That shape protocol belongs to
delta/opcode node families. A closure has arbitrary remaining parameter count and
uses a ZINC-style `GRAB` loop over argument stack.

Conceptually:

```text
grab(CLOSURE(mode, remaining_params, body, with, E0), S):
    remaining_params empty
        -> enter body under E0, preserving extra S

    S empty
        -> return partial CLOSURE(mode, remaining_params, body, with, E0)

    otherwise
        -> pop next argument from S
        -> force to WHNF only when mode = eager
        -> bind it in E0
        -> continue grab with next parameter
```

For eager parameter and one available argument:

```text
<RETURN(CLOSURE(eager, ps, body, with, E0)), E, TERM(code, Et) :: S, R>
    -> <EVAL(code), Et, empty, GRAB_FN(ps, body, with, E0, S) :: R>
```

When forcing finishes:

```text
<RETURN(TERM(code, Et)), E, empty, GRAB_FN(ps, body, with, E0, S) :: R>
    -> <EVAL(code), Et, empty, GRAB_FN(ps, body, with, E0, S) :: R>

<RETURN(v), E, empty, GRAB_FN(ps, body, with, E0, S) :: R>
    where v is not TERM
    -> bind v, then continue grab(ps, body, with, v :: E0, S, R)
```

Thus forcing follows suspended-term chains until a non-`TERM` value appears, then
`GRAB` immediately consumes another available argument.

Underapplication:

```text
needs 3, stack has 0
    -> return original closure

needs 3, stack has 1
    -> bind 1, allocate one partial closure needing 2
```

Exact application:

```text
needs 3, stack has 3
    -> bind all 3, enter body, allocate no intermediate partial closure
```

Overapplication:

```text
needs 2, stack has 3
    -> bind 2, enter body with 1 argument still pending
    -> apply body result to remaining argument
```

If body result is not callable while arguments remain, execution fails with
non-callable error.

This is main ZINC-derived calling-convention benefit for v0: exact and overapplied calls do
not allocate one closure per curried argument. Underapplication after consuming
some arguments creates one closure carrying supplied bindings and resume point;
with no consumed arguments, existing closure can be returned unchanged.

### Form Invocation

Forms use same `GRAB` loop but bind each suspended argument without evaluating it:

```text
<RETURN(CLOSURE(term, ps, body, with, E0)), E, t :: S, R>
    -> bind t, then continue grab(ps, body, with, t :: E0, S, R)
```

Example:

```vetochka
{form} args: [x] do
    x
end
```

returns the `TERM` bound to `x`; it does not execute `x`.

An explicit primitive can force it:

```vetochka
{force}(x)
```

`{force}` has one eager argument. Passing `x` to it forces the stored `TERM`.

### Native Operations

Each primitive declares the mode of its next argument:

```text
next_mode(primitive, supplied_count) = eager | term
```

Term argument:

```text
<RETURN(PRIMITIVE(p, xs)), E, t :: S, R>
    where next_mode(p, len(xs)) = term
    -> dispatch_or_curry(p, xs ++ [t], S, R)
```

Eager argument:

```text
<RETURN(PRIMITIVE(p, xs)), E, TERM(code, Et) :: S, R>
    where next_mode(p, len(xs)) = eager
    -> <EVAL(code), Et, empty, FORCE_PRIMITIVE(p, xs, S) :: R>

<RETURN(TERM(code, Et)), E, empty, FORCE_PRIMITIVE(p, xs, S) :: R>
    -> <EVAL(code), Et, empty, FORCE_PRIMITIVE(p, xs, S) :: R>

<RETURN(v), E, empty, FORCE_PRIMITIVE(p, xs, S) :: R>
    where v is not TERM
    -> dispatch_or_curry(p, xs ++ [v], S, R)
```

`dispatch_or_curry` returns a new `PRIMITIVE` when unsaturated and invokes native
code when saturated.

## Triage Application

Applicable families share structural saturation:

```text
NODE0(F, P) @ x
    -> NODE1(F, P, x)

NODE1(F, P, x) @ y
    -> NODE2(F, P, x, y)
```

Applying `NODE2` selects behavior by family:

```text
NODE2(delta, none, x, y) @ z
    -> triage reduction selected by shape(x)

NODE2(opcode, id, x, y) @ z
    -> machine/native dispatch(id, x, y, z)
```

Other families may define their own fork-application behavior or reject application.
`APPLY(f, x)` does not follow this rule because it is pending application code, not
a `NODE2` value.

Triage application is non-strict. Suspended arguments remain runtime values.
Define:

```text
shape(NODE0(...))       = leaf
shape(NODE1(..., u))    = stem(u)
shape(NODE2(..., u, v)) = fork(u, v)
```

Runtime wrappers such as closures and terms must expose an assigned outer shape if
they are admitted as calculus values. Their family tag and hidden payload do not
affect rule selection.

Reduction of delta fork:

```text
NODE2(delta, none, leaf(_), y) @ z
    -> y

NODE2(delta, none, stem(x), y) @ z
    -> (x @ z) @ (y @ z)

NODE2(delta, none, fork(w, x), y) @ leaf(_)
    -> w

NODE2(delta, none, fork(w, x), y) @ stem(u)
    -> x @ u

NODE2(delta, none, fork(w, x), y) @ fork(u, v)
    -> (y @ u) @ v
```

Before selecting a rule, only demanded positions are forced to runtime weak-head
form:

- left child of delta fork must expose leaf, stem, or fork;
- rule 3 forces `z` only enough to expose leaf, stem, or fork;
- other children remain suspended.

The implementation uses continuation frames for these demands. It must not scan or
normalize whole terms.

## Returning and Forcing

If argument stack is empty, `RETURN(v)` resumes top return frame. If both stacks are
empty, `v` is final result.

Forcing is:

```text
force(TERM(code, E)) -> evaluate code under E to WHNF, with isolated argument stack
force(v)             -> v
```

Isolating argument stack is ZINC's stack-mark idea expressed as an explicit return
frame. It prevents evaluation of a strict argument from consuming arguments meant
for its caller.

This draft deliberately defines call-by-name for form arguments: forcing same
`TERM` twice evaluates it twice, including effects. Triage rule 2 and `{form}` may
therefore duplicate effectful computation. This is intended calculus behavior, not
an accidental optimization gap. Effect order follows ordinary callee-first demand;
an undemanded duplicate does not execute.

## Evaluation Order

Generic application follows:

1. Evaluate callee.
2. Inspect callable kind or next argument mode.
3. Force argument only for `eager`.
4. Enter callable body or dispatch primitive.

This order is required because callable mode may be known only at runtime.

It differs from stock ZINC call-by-value compilation, which evaluates arguments
before application. Static specialization may recover that fast path when callee and
argument modes are known:

```text
known eager closure/primitive call -> evaluate argument directly, then Apply
known form/term call               -> push TERM directly
unknown callable                   -> generic mode-aware rule
```

These are optimizations, not different semantics.

## Seed Compilation and Bootstrap

There is no bootstrap cycle:

1. Host parser reads `boot.tree` into `source_tree_t`.
2. Small seed specializer understands only `v0` core protocols.
3. It resolves lexical names to `LOCAL`, globals to `GLOBAL`/`OPCODE`, and emits
   specialized cells.
4. Result is embedded or cached.
5. Runtime loads compiled bootstrap cells.
6. Bootstrap compiles richer `v*` source using raw syntax handles.

Seed specializer performs lexical resolution, but does not implement modules, rich
types, C generation, or user-level language policy.

## Consequences for Current Code

Current implementation fuses syntax normalization and cell encoding in
`bytecode_source.c`. Proposed split:

```text
source_tree_t
    -> nameful bootstrap syntax data
    -> v0 specialization/resolution
    -> specialized IR cells
```

For precompiled bootstrap, first two arrows run at build time.

Current `OP_FN0/1/2` already demonstrates shared leaf/stem/fork saturation. It can
become `{fn}` constructor family; `{form}` gets an equivalent family. Constructor
dispatch creates a separate runtime closure:

```text
OP_FN2(params, body) @ with
    -> CLOSURE(eager, params, body, with, environment)

OP_FORM2(params, body) @ with
    -> CLOSURE(term, params, body, with, environment)
```

Current reducer dispatches mostly by generic node arity. New invariant:

```text
applicable family shape = TYPE0 | TYPE1 | TYPE2
TYPE0 @ x              -> TYPE1(x)
TYPE1(x) @ y           -> TYPE2(x, y)
TYPE2(x, y) @ z        -> family-specific behavior
```

Delta, opcode, and runtime-wrapper families may expose leaf/stem/fork shape to
triage inspection. Closures use arbitrary-arity `GRAB`, even if their wrapper has
an inspectable outer shape. `APPLY` remains a separate pending-application code
node and requires explicit machine dispatch.

Likely implementation boundary:

```text
cells_*    compact node storage and references
reducer_*  triage runtime rules over runtime values
cesk_*     control, environments, store, arguments, returns, closures, primitives
```

## Immediate Use-Case Caveats

Settled for first CESK implementation:

- `eager` evaluates to weak-head form, not full normalization and not exactly one
  `APPLY`. Integers, bytes, closures, primitives, and leaf/stem/fork values are
  results; native operations explicitly demand deeper payloads when needed.
- Non-strict terms are call-by-name. Repeated forcing, including through triage rule
  2, deliberately repeats effects.
- CESK owns application dispatch. It unfolds `APPLY`, inspects callable family, then
  saturates or dispatches that family.
- Machine peels argument spines into pending `TERM`s without evaluating them. Closure
  `GRAB` then consumes all available arguments, forcing only eager parameters.
- Exact and overapplied closure calls allocate no intermediate partial closures.
  Underapplication returns one closure for remaining parameters.
- Runtime `TERM` is not source syntax. Source transforms use
  `source_tree_t`/syntax handles before specialization.
- Seed specializer stays small: recognize v0 constructors, resolve locals/globals,
  and emit specialized cells. Module discovery and language policy belong in
  bootstrap code.

Host API work still needed before source bootstrap, but not a semantic decision:

- define `source_tree_t` handle ownership and lifetime;
- define minimal source-tree traversal and construction operations;
- define exact `specialize_v0` input, output, and diagnostic contract.

Can defer for first bootstrap:

- Garbage collection. A process-lifetime arena is acceptable for one-shot bootstrap
    execution if limits are measured and explicit.
- Call-by-need memoization and black-hole detection.
- Recursive bindings and mutable boxes may be deferred operationally, but store and
  stable-location semantics are reserved now.
- Serialization, equality, or source reconstruction for closures and `TERM`.
- General inspection of lexical environments. Keep environment/capability payloads
    opaque; expose only deliberate operations.
- Linear bytecode optimizations. Tree-shaped specialized cells are sufficient
    for first execution path.

## Deferred

- Exact byte tags for `OP_FORM0/1/2`, `LOCAL`, and `GLOBAL`.
- Exact runtime heap and garbage collector.
- Runtime object encoding in cells versus separate runtime allocations.
- Exact box cell layout and transparent mutable-binding lowering.
- Call-by-need memoization.
- Reification of `TERM` into closed inspectable cells.
- Linear bytecode and direct-threaded dispatch.
- Final surface spelling and arity of `{fn}` and `{form}`.
