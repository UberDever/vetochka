# 11.06.2026

## ✅ Temporary implementation guidance

This section is not part of the language specification. It describes one intended implementation strategy for lexing, layout, and structured comments. Remove it after the lexer/parser behavior is implemented and tested.

### Pipeline

Implement parsing as three conceptual phases:

1. **Raw lexer**

   * Produces ordinary lexical tokens.
   * Preserves physical newlines as newline tokens.
   * Skips horizontal whitespace and ordinary comments.
   * Recognizes string literals, integer literals, identifiers, operators, delimiters, `do`, `end`, `@[`, `]`, and `...`.

2. **Layout filter**

   * Consumes raw tokens.
   * Inserts virtual semicolon tokens on eligible physical newlines.
   * Uses only lexical state: delimiter stack, previous significant token, and line-continuation state.
   * Must not depend on parser productions.

3. **Parser / reader**

   * Parses explicit and virtual semicolons identically.
   * Handles `#;` structured comments by parsing and discarding the next expression.
   * Does not perform automatic semicolon insertion.

### Newline handling

Split whitespace into horizontal whitespace and newline:

```ebnf
t_hws ::= 09 | 0B | 0C | 20
t_nl  ::= 0A | 0D 0A | 0D
```

A physical newline may produce a virtual semicolon only in layout-active contexts.

Layout-active contexts:

```txt
source
do ... end block
```

Layout-inactive contexts:

```txt
(...)
[...]
@[...]
```

Do not insert virtual semicolons inside parenthesized expressions, list/index brackets, or annotation brackets.

### Line continuation

`...` is a lexical line-continuation marker.

```ebnf
line_continue ::= "..." t_hws* line_comment? t_nl
```

When `...` appears before a newline, it consumes that newline and prevents virtual semicolon insertion.

Example:

```txt
x +
y
```

does not require `...`, because `+` cannot end an expression.

```txt
x ...
    + y
```

is also one expression, because `...` explicitly suppresses the newline.

### Virtual semicolon insertion

When the layout filter sees a physical newline, insert a virtual `;` iff all of the following are true:

1. The current lexical context is layout-active.
2. The newline was not consumed by `...`.
3. The previous significant token can end an expression.

Tokens that can end an expression:

```txt
t_literal
t_identifier
")"
"]"
"end"
```

Tokens that cannot end an expression include:

```txt
t_operator
prefix_operator
"."
","
":"
"("
"["
"@["
"do"
```

The layout filter should treat explicit `;` and virtual `;` the same after insertion.

### Annotation close

Annotation opening must be lexed as a distinct token:

```ebnf
t_annot_open ::= "@["
```

The lexer/layout filter should push an annotation delimiter context after `@[`.

The closing `]` of an annotation should be distinguishable internally from a normal bracket close. A newline immediately after an annotation close must not produce a virtual semicolon.

Example:

```txt
@[doc {entry point}]
def fn : main do
    ...
end
```

must be interpreted as an annotation attached to the following expression, not as a completed standalone expression.

### Labels and newline restriction

Whitespace around `:` in labeled arguments is allowed:

```txt
label: value
label : value
label   :   value
```

However, the colon must appear on the same physical line as the label identifier.

This is valid:

```txt
fn : main
```

This is invalid or split by ASI:

```txt
fn
    : main
```

Reason: after `fn`, the newline is eligible for virtual semicolon insertion. The layout filter must not look ahead to discover a later `:`.

The parser should therefore define labeled arguments structurally:

```ebnf
labeled_argument ::= t_identifier t_colon argument_expression
```

Do not encode labels as a single lexical token if whitespace around `:` is allowed.

### Operators containing colon

A single `:` is always `t_colon`.

Colon-containing operator runs are allowed only when the token is not exactly `:`.

Examples:

```txt
:    t_colon
::   t_operator
:=   t_operator
+:   t_operator
```

The raw lexer should prefer the longest operator run, then reject/special-case the exact single-character `:` as `t_colon`.

### Structured comments

`#;` is not ordinary trivia.

Implement `#;` at parser/reader level:

```txt
#; expression
```

The parser must parse exactly one following expression and discard it.

Examples:

```txt
#; foo(a, b)
bar
```

is equivalent to:

```txt
bar
```

```txt
#; def fn : debug do
    print({debug})
end

def fn : main do
    print({main})
end
```

discards the whole `def ... do ... end` expression, not merely one line or one lexical block.

Structured comments may appear anywhere an expression may appear, but they do not produce an AST node.

### Required implementation invariant

After raw lexing and layout filtering, the parser must be able to parse the program without knowing which semicolons were explicit and which were inserted.

Therefore, ASI behavior must be fully determined before parsing proper.

# 13.06.2026

#### Implemented now

- The C parser produces a host-side `source_tree_t`. It preserves concrete
    syntax structure and token spans; it performs no name resolution or semantic
    specialization.
- The cells library stores compact triage terms and references. The reducer implements
    calculus application and native-value currying by arity.
- There is not yet a VM environment, literal intrinsic dispatch, bootstrap evaluator,
    closure conversion, module resolver, or raw-tree-to-cells bridge.
- Therefore current parser tests demonstrate accepted syntax and formatting, while
    reducer tests demonstrate the calculus. They are not yet one execution pipeline.

#### Execution and compilation pipeline

1. The executable reads command-line options and source files, then parses source into
   ordinary host trees. Raw trees do not need cell encoding.
2. The executable constructs one `initial_term` containing argv/options, project facts,
   source identities, and handles to parsed trees.
3. The executable applies its embedded unary bootstrap function to that term:
   `bootstrap(initial_term)`.
4. Bootstrap Vetochka code traverses raw trees, normalizes them into tagged syntax IR,
   expands source protocols such as `def`, resolves modules and bindings, performs
   closure conversion, and constructs specialized executable terms.
5. Bootstrap activates the selected entry term and the VM executes it.

This is full compilation, but semantic analysis belongs to embedded Vetochka code, not
the C parser. There is no required code/data boundary or separate host lowerer:
specialized code is an ordinary inspectable term made from cells, including explicit
application nodes. The C boundary is parsing, primitive host operations, cell storage,
and execution.
The bootstrap program is embedded in the executable as cells. Its source uses only
literals, triage terms, application, and intrinsic calls. Exact CLI spelling and the
schemas of `initial_term`, diagnostics, and the final result remain open.

#### Lexical closures

- Static resolution gives every binding a unique identity and determines each function's
    free bindings.
- Closure conversion must explicitly carry the values of free bindings at function
    creation. Renaming free identifiers is insufficient because the low-level call frame
    is restored before a returned function may run.
- Specialized executable terms therefore represent a lexical function as a closed
    low-level function plus explicit capture data, carried through callable arguments or
    a closure environment. Exact representation remains open.
- Immutable captures can be values. Recursive or mutable captures require stable
    references/boxes shared by all relevant closures.
- Names deliberately marked dynamic may remain call-time environment lookups.

Thus lexical scope is a bootstrap/compiler guarantee built over the smaller dynamically
scoped runtime, not another evaluator rule.

Bootstrap itself does not require lexical closures. It is deliberately first-order:
helpers take one explicit state tree, remain within one bootstrap invocation, and never
escape. Nested immediate `{fn}` applications keep all helper bindings dynamically active
until the entry helper returns. Recursion works within that dynamic extent. Any state
that would otherwise be captured is passed explicitly.

Generated lexical functions use an explicit closure convention. A closure is ordinary
tree data containing closed code and capture data; its application supplies both the
capture tree and the actual argument to that code. The exact record and application
encoding remain open, but no VM lexical environment is required.

#### Primitive runtime

- A literal is inert when stored, passed, or inspected. Intrinsic dispatch happens only
    when the literal is the active callee of an application. Unknown names are errors.
- Dispatch resolves the operation before deciding whether its argument is raw or eager.
    Partial application produces an ordinary callable carrying the resolved operation
    identity and immutable supplied arguments.
- `do!` is removed rather than retained as a second intrinsic-selection mechanism.
- Application and triage inspection remain calculus operations, not separate opcodes.
- Native literal payload operations are exposed only where structural calculus cannot
    provide them, initially byte comparison and required integer arithmetic.
- Raw-tree host operations minimally provide node kind, token/payload, child count, and
    child access. Exact names and ownership rules remain open.
- Build-host operations such as file access and process execution form a separate API.
    Initial compilation needs none when all project inputs are already in `initial_term`.

#### Modules and build

- Module discovery and module semantics are separate. The host/build layer supplies
    candidate sources; bootstrap code decides what those candidates mean.
- A checked module candidate has a name, explicit dependencies, declared public names,
    and a body/generator. Public names are known before dependent modules use them.
- Requiring a dependency does not implicitly open its names. Dependency edges and local
    name introduction are separate operations.
- Bootstrap code checks the graph, constructs module environments, resolves references,
    and exposes only declared exports. Raw dynamic evaluation may bypass this library
    protocol.
- Initial signatures need only names. Types and stronger interface ascriptions can be
    added without changing discovery.
- The same checked contract should drive Vetochka resolution and generated C API
    boundaries.
- `initial.tree` should contain concrete project/manifest facts. Generic build logic belongs
    in ordinary `boot.tree` code consuming that manifest and the host build API.

#### Reconciled history

Retained:

- Compact contiguous cells, explicit application information, triage-calculus reduction,
    inspectable terms, currying, per-parameter eager/raw modes, generic syntax, tagged
    grouping/operator distinctions, VM state, and the C-frontend/project-tool framing.
- The principle that optimized/native behavior must still compose as ordinary terms.
- The distinction between source discovery, static name resolution, and runtime lookup.

Superseded as the primary design:

- A large fixed set of evaluator opcodes such as `.lambda`, `.set`, and `.seq`.
- Magic-position tree encodings for many independent opcodes.
- Direct parser-to-executable-cells specialization.
- Implementing `def`, modules, lexical scope, or the type system as C parser rules or
    mandatory VM special forms.
- Treating old named `scope` syntax as the module semantics. It remains prior exploration;
    current modules are a checked bootstrap protocol.

# 16.06.2026

## CEK/CESK, ZINC, and Scheme naming scratch

Non-normative reasoning retained here instead of `PROJECT.md` or machine rules.

- Calling whole runtime `ZINC` overclaimed similarity. Stock ZINC assumes uniform
  call-by-value compilation; v0 chooses argument mode after evaluating runtime callee.
- `CEK` was initially chosen for control, lexical environment, continuation, plus
  pending argument spine and `TERM`.
- Intended stable mutable boxes make store semantic, so target name is `CESK`.
  Immutable first implementation is its CEK subset. Cells may implement store, but
  ordinary immutable code cells are not store merely because they occupy memory.
- Keep useful ZINC calling convention only: argument collection, `GRAB`,
  underapplication, overapplication, and tail-call frame reuse.
- Scheme was considered as comparison, not lower-level backend. Scheme specifies
  strict ordinary calls, lexical closures, syntax-level special forms, proper tail
  calls, and memoized promises, but does not require one VM.
- v0 differs from Scheme because `{form}` is runtime callable, evaluation is
  callee-first, and repeated `TERM` forcing is deliberate call-by-name rather than
  memoized promise behavior.

Durable decisions:

- [`project/v0_cesk_draft.md`](project/v0_cesk_draft.md)
- [`project/v0_basis_review.md`](project/v0_basis_review.md)

# 17.06.2026

## CEK start for `{fn}` over current reducer

Goal: start with CEK, not CESK. No store. No WHNF condition. Do not create a second application semantics beside reducer. Reducer executes calculus; CEK/VM supplies environment and opcode actions.

Current facts:

- Cells already have `APPLY`, `OP_FN0`, `OP_FN1`, `OP_FN2`.
- Source encoder already turns `{fn}` in callee position into `OP_FN0`; `{fn}` as data remains bytes.
- `$ f x` means `APPLY(f, x)` in cells. Current reducer API still uses postfix stack input: push `REDUCER_APPLY_TOKEN`, then callee index, then argument index.
- Current reducer rule 0 saturates any arity-0/1 node, so `OP_FN0 @ params -> OP_FN1(params)` and `OP_FN1(params) @ body -> OP_FN2(params, body)` already work.
- Reducer is not fallback. Reducer is primary executor for calculus applications.
- VM must only intercept recognized opcode/function values at dispatch boundary. It must not reimplement rules 0a, 0b, 1, 2, 3a, 3b, 3c.
- Current reducer cannot finish `{fn}/3` by itself: `OP_FN2(params, body) @ with` would fall into generic arity-2 triage rule selection. Needed change is an opcode hook in reducer application, not a parallel CEK apply machine.

Small CEK data, using existing abstractions:

```text
callable = [callable-tag, [cur_arity,max_arity], [curried_args], payload]
fn payload = [params, body, with, Edef]
env = [frame, parent_env]
frame = [[name, value], ...]
term = [root, env]
```

All of this is cell data in `cells_t`. No side table. No `callable_i`. No separate value heap. Use existing cell shapes:

```text
tag       -> VALUEV0(":fn" / ":env" / ":frame" / ":term" / ...)
pair/list -> DELTA2(left, right) lists, same current source data convention
payload   -> normal child cells
```

`Edef` is a cell index pointing to an env tree. `Ecaller` is current env cell index saved in reducer/VM continuation marker. If continuation storage must be inspectable later, encode continuation frames in cells too; first cut may keep them in `reducer->stack`-style arrays because reducer already uses arrays for control.

`OP_FN0/1/2` are constructor stages. `OP_FN2 @ with` allocates a callable tree in `cells_t` and returns its cell index.

Formal step rules:

```text
Machine state:
  <M, c, rho, k>

M    cells_t
c    current cell index
rho  current env cell index
k    control stack

k ::= []
    | Arg(x, rho_x) :: k
    | Bind(fn, args, frame, i, rho_caller) :: k
    | Restore(rho_caller) :: k
```

All values are cell indices in `M`. `k` is control, same kind of thing as reducer stack; payload objects referenced by `k` are cells.

Application cell:

```text
M[c] = APPLY(f, x)
------------------------------------------------
<M, c, rho, k> -> <M, f, rho, Arg(x, rho) :: k>
```

Identifier lookup:

```text
M[c] = ID(name)
lookup(M, rho, name) = v
------------------------------------------------
<M, c, rho, k> -> <M, v, rho, k>
```

For plain calculus apply:

```text
red(M, f, x) = (M', v)

where red is current reducer run with stack:
  REDUCER_APPLY_TOKEN, f, x

no VM hook matches (f, x)
------------------------------------------------
<M, f, rho, Arg(x, rho_x) :: k> -> <M', v, rho, k>
```

If `x.root` contains identifiers, this plain reducer path is only valid after those identifiers are already lowered/resolved. If not, VM must evaluate/lookup before reducing. Reducer never does env lookup.

Reducer opcode hook:

```text
OP_FN0 @ params
  -> normal reducer rule 0a allocates OP_FN1(params)

OP_FN1(params) @ body
  -> normal reducer rule 0b allocates OP_FN2(params, body)

OP_FN2(params, body) @ with
  -> opcode hook allocates fn callable cell:
     [":callable", [0, param_count(params)], [], [":fn", params, body, with, E]]

fn callable cell @ actual
  -> function hook invokes or curries
```

Hook is checked before reducer arity-2 triage rule. If no hook matches, reducer continues with triage rule 1/2/3 exactly as today.

Function construction:

```text
M[f] = OP_FN2(params, body)
alloc(M, [":callable", [0, n], [], [":fn", params, body, with, rho]]) = (M', fn)
n = param_count(M, params)
------------------------------------------------
<M, f, rho, Arg(with, rho_w) :: k> -> <M', fn, rho, k>
```

Curried function apply:

```text
M[f] = [":callable", [cur, max], args, payload]
cur + 1 < max
alloc(M, [":callable", [cur + 1, max], args ++ [[x, rho_x]], payload]) = (M', f')
------------------------------------------------
<M, f, rho, Arg(x, rho_x) :: k> -> <M', f', rho, k>
```

Saturated function apply, strict args:

```text
M[f] = [":callable", [cur, max], args, payload]
cur + 1 = max
alloc(M, []) = (M', frame)
args' = args ++ [[x, rho_x]]
args'[0] = [x0, rho0]
------------------------------------------------
<M, f, rho, Arg(x, rho_x) :: k>
  -> <M', x0, rho0, Bind(f, args', frame, 0, rho) :: k>
```

No recursion. Strict arg evaluation is normal stepping with `Bind` on `k`.

Bind one evaluated argument:

```text
M[f] payload params = params
param_name(M, params, i) = name
frame_extend(M, frame, name, v) = (M', frame')
i + 1 < len(args)
args[i + 1] = [x_next, rho_next]
------------------------------------------------
<M, v, rho, Bind(f, args, frame, i, rho_caller) :: k>
  -> <M', x_next, rho_next, Bind(f, args, frame', i + 1, rho_caller) :: k>
```

Enter body after last arg:

```text
M[f] payload [":fn", params, body, with, rho_def]
param_name(M, params, i) = name
frame_extend(M, frame, name, v) = (M', frame')
i + 1 = len(args)
alloc(M', [frame', rho_def]) = (M'', rho_body)
------------------------------------------------
<M, v, rho, Bind(f, args, frame, i, rho_caller) :: k>
  -> <M'', body, rho_body, Restore(rho_caller) :: k>
```

Restore caller env:

```text
------------------------------------------------
<M, v, rho, Restore(rho_caller) :: k>
  -> <M, v, rho_caller, k>
```

Final state:

```text
M[c] is not APPLY
k = []
------------------------------------------------
<M, c, rho, []> halts with result c
```

`{fn}` is strictly eager. No WHNF: each arg steps to final result cell before body env is allocated.

Reducer boundary:

- Reducer remains triage engine over `cells`.
- VM owns environment and opcode actions.
- `APPLY` cells are syntax/storage for application; activation steps them one at a time and calls reducer only for plain calculus apply.
- Reducer owns plain triage rules from `PROJECT.md`: 0a, 0b, 1, 2, 3a, 3b, 3c.
- Critical invariant: opcode hook before generic arity-2 rule. `OP_FN2 @ with` must not enter current reducer rule 1/2/3.

Small example:

```text
$ ($ ($ ($ {fn} params) body) with) 42
```

Flow:

```text
<M, $ ($ ($ ($ OP_FN0 params) body) with) 42, rho0, []>
  ->* <M1, OP_FN2(params, body), rho0, Arg(with,rho0) :: Arg(42,rho0) :: []>
  ->  <M2, fn_cell, rho0, Arg(42,rho0) :: []>
  ->  <M3, 42, rho0, Bind(fn_cell, [[42,rho0]], frame, 0, rho0) :: []>
  ->  <M4, body, rho_body, Restore(rho0) :: []>
  ->* <M5, value, rho_body, Restore(rho0) :: []>
  ->  <M5, value, rho0, []>
```

Start-small cut:

- One positional param.
- One body expression.
- Eager only.
- Lexical closure by capturing `Edef`.
- No store, no `{form}`, no modules, no mutation.
- After this works, add multi-param currying and block sequencing.

## CEK rewrite with cell syntax

Step 1: notation.

Use cell syntax directly. Superscript is arity. Bracket prefix is byte index.

```text
^0                    delta leaf
^1 a                  delta stem
^2 a b                delta fork
ref(N)                reference node; N is byte index target
value0f(n)            fixed native value
value0v(bytes)        byte payload value
op_fn0                {fn} constructor leaf
op_fn1 params         {fn} constructor stem
op_fn2 params body    {fn} constructor fork
$ f x                 APPLY(f, x), executable application cell
```

Indexed cells:

```text
[0]^0
[1]value0f(42)
[10]^2 ref(0) ref(1)
```

References are explicit `ref(N)` child nodes. `N` is byte index into `cells_t`. Example above means cell at byte `10` is fork with child refs to byte `0` and byte `1`.

Runtime state uses same indices:

```text
C = current cell index
E = current env cell index
K = control stack of indices/markers
```

No side heap. If something is runtime data, it is cell data. If something is control, it is `K` like reducer stack.

CEK metadata outside `cells_t`:

```c
typedef enum {
  CEK_K_ARG,
  CEK_K_BIND,
  CEK_K_DO,
  CEK_K_RESTORE,
} cek_k_kind_t;

typedef struct {
  size_t name;
  size_t value;
} cek_binding_t;

typedef struct {
  cek_binding_t* bindings;
} cek_frame_builder_t;

typedef struct {
  size_t fn;
  size_t args;
  size_t arg_i;
  size_t env_caller;
  cek_frame_builder_t frame;
} cek_bind_t;

typedef struct {
  size_t exprs;
  size_t next_i;
} cek_do_t;

typedef struct {
  cek_k_kind_t kind;
  union {
    size_t payload; // CEK_K_ARG/RESTORE payload cell in cells_t
    cek_bind_t bind;
    cek_do_t do_;
  } as;
} cek_k_entry_t;

typedef struct {
  struct cells_t* cells;
  struct reducer_t* reducer;
  size_t control;       // C
  size_t env;           // E
  cek_k_entry_t* stack; // K
} cek_t;
```

Only `kind`, `control`, `env`, stack shape, `BIND` builder state, and `DO` cursor are external CEK metadata. Durable runtime data lives in `cells_t`.

External CEK functions:

```c
void cek_push_arg(cek_t* cek, size_t arg_root, size_t env);
void cek_push_bind(cek_t* cek, size_t fn, size_t args, size_t arg_i, size_t env_caller);
void cek_push_do(cek_t* cek, size_t exprs, size_t next_i);
void cek_push_restore(cek_t* cek, size_t env_caller);
cek_k_entry_t cek_pop(cek_t* cek);
```

`cek_push_arg` and `cek_push_restore` allocate payload cells in `cells_t`. `cek_push_bind` creates C-side frame builder. `cek_push_do` stores sequencing cursor in CEK metadata.

Payload shapes:

```text
ARG payload:
  [p]^2 ref(arg_root) ref(env)

BIND payload:
  none in cells_t; CEK metadata holds cek_bind_t, including cek_frame_builder_t

DO payload:
  none in cells_t; CEK metadata holds exprs,next_i

RESTORE payload:
  [p]ref(env_caller)
```

Step 2: input term layout.

Start with one strict function call:

```text
{fn}(x)([ [{:id}, {x}] ])([{:do}, <body_expr>])([{:with}, 5])(42)
```

Same thing as application:

```text
$ ($ ($ ($ op_fn0 params_list) do_list) with_list) arg
```

List encoding used here:

```text
[head0, head1, ...] = ^2 head0 (^2 head1 (... ^0))
```

Tagged field/list item:

```text
[{:tag}, payload...] = ^2 value0v(":tag") payload_list
```

One possible `cells_t` layout. First atoms:

```text
[0]op_fn0

[1]value0v(":id")
[10]value0v("x")
[20]value0v(":do")
[30]value0v(":with")
[40]value0f(5)
[50]value0f(42)               ; actual call arg
```

`[{:id}, {x}]`:

```text
[60]^2 ref(10) ref(61)        ; payload list: x :: nil
[61]^0
[70]^2 ref(1) ref(60)         ; tagged id item
```

Params list `[ [{:id}, {x}] ]`:

```text
[80]^2 ref(70) ref(81)
[81]^0
```

Do list `[{:do}, <body_expr>]`, with body expr `{:id} x`:

```text
[90]^2 ref(70) ref(91)        ; payload list: id(x) :: nil
[91]^0
[100]^2 ref(20) ref(90)       ; tagged do item
```

With list `[{:with}, 5]`:

```text
[110]^2 ref(40) ref(111)      ; payload list: 5 :: nil
[111]^0
[120]^2 ref(30) ref(110)      ; tagged with item
```

Application cells:

```text
[130]$ ref(0) ref(80)         ; $ op_fn0 params_list
[140]$ ref(130) ref(100)      ; $ ($ op_fn0 params_list) do_list
[150]$ ref(140) ref(120)      ; $ ($ ($ op_fn0 params_list) do_list) with_list
[160]$ ref(150) ref(50)       ; whole call
```

Initial runtime state:

```text
C = 160
E = 170
K = []

[170]^0                       ; empty env for first step
```

No evaluation yet. This step only fixes concrete input shape in `cells_t`.

Step 3: peel application cells.

Rule:

```text
if C points to:
  [C]$ ref(F) ref(X)

then:
  cek_push_arg(cek, X, E)
  C = F
```

Starting state:

```text
C = 160
E = 170
K = []
```

Step:

```text
[160]$ ref(150) ref(50)

[180]^2 ref(50) ref(170)       ; ARG payload allocated by cek_push_arg
K = [CEK_K_ARG ref(180)]
C = 150
E = 170
```

Step:

```text
[150]$ ref(140) ref(120)

[190]^2 ref(120) ref(170)
K = [CEK_K_ARG ref(190), CEK_K_ARG ref(180)]
C = 140
E = 170
```

Step:

```text
[140]$ ref(130) ref(100)

[200]^2 ref(100) ref(170)
K = [CEK_K_ARG ref(200), CEK_K_ARG ref(190), CEK_K_ARG ref(180)]
C = 130
E = 170
```

Step:

```text
[130]$ ref(0) ref(80)

[210]^2 ref(80) ref(170)
K = [CEK_K_ARG ref(210), CEK_K_ARG ref(200), CEK_K_ARG ref(190), CEK_K_ARG ref(180)]
C = 0
E = 170
```

Now `C` points to:

```text
[0]op_fn0
```

Stop peeling. Next step is applying `op_fn0` to first pending arg.

Step 4: apply constructor stages `op_fn0` and `op_fn1`.

Current state from Step 3:

```text
C = 0
E = 170
K = [CEK_K_ARG ref(210), CEK_K_ARG ref(200), CEK_K_ARG ref(190), CEK_K_ARG ref(180)]

[0]op_fn0
[210]^2 ref(80) ref(170)      ; argument term = (params_list, env)
```

Rule:

```text
if C points to arity-0 or arity-1 cell
and top K entry is CEK_K_ARG ref(P)
and [P]^2 ref(X) ref(EX)

then:
  pop K
  run reducer on C @ X
  C = reducer result
```

`EX` is ignored here because `{fn}` constructor fields are raw cell data. No name lookup, no eager eval.

Important: constructor field order is not semantic. These are all same kind of `{fn}` construction:

```text
{fn}(x) do ... end with: stuff
{fn} with: stuff (x) do ... end
{fn} do ... end (x) with: stuff
```

Therefore `op_fn0/op_fn1/op_fn2` must accumulate raw constructor fields, not assign meanings by position. Meaning is recovered after all three fields are present:

```text
params field = list whose items are param descriptors, e.g. [ [{:id}, {x}] ]
body field   = tagged list [{:do}, ...]
with field   = tagged list [{:with}, ...]
```

Apply `op_fn0 @ first_field`:

```text
reducer stack = [REDUCER_APPLY_TOKEN, 0, 80]
reducer result allocated:

[220]op_fn1 ref(80)

C = 220
E = 170
K = [CEK_K_ARG ref(200), CEK_K_ARG ref(190), CEK_K_ARG ref(180)]
```

Apply `op_fn1(first_field) @ second_field`:

```text
[200]^2 ref(100) ref(170)     ; do_list term

reducer stack = [REDUCER_APPLY_TOKEN, 220, 100]
reducer result allocated:

[230]op_fn2 ref(80) ref(100)

C = 230
E = 170
K = [CEK_K_ARG ref(190), CEK_K_ARG ref(180)]
```

Now `C` points to:

```text
[230]op_fn2 ref(80) ref(100)
```

This means only:

```text
op_fn2(first_field = [80], second_field = [100])
```

Do not read it as:

```text
op_fn2(params, body)
```

Next pending arg is third constructor field, at payload:

```text
[190]^2 ref(120) ref(170)     ; with_list term
```

Stop here. Next step is special hook: `op_fn2 @ third_field` classifies all three fields, then creates function cell. It must accept any order.

Step 5: finish `{fn}` construction.

Current state:

```text
C = 230
E = 170
K = [CEK_K_ARG ref(190), CEK_K_ARG ref(180)]

[230]op_fn2 ref(80) ref(100)
[190]^2 ref(120) ref(170)     ; third field term
```

Dereference accumulated fields:

```text
[80]^2 ref(70) ref(81)        ; [ [{:id}, {x}] ]
[100]^2 ref(20) ref(90)       ; [{:do}, x]
[120]^2 ref(30) ref(110)      ; [{:with}, 5]
```

Hook condition:

```text
C is op_fn2 ref(A) ref(B)
top K is CEK_K_ARG ref(P)
[P]^2 ref(F) ref(EF)

fields = [A, B, F]
fields contain exactly:
  one params field
  one {:do} field
  one {:with} field
```

`EF` is ignored. Constructor fields are raw syntax/data. Closure environment is current `E`, not field env.

Classify:

```text
params = 80
body   = 100
with   = 120
Edef   = E = 170
```

Allocate function cell in `cells_t`:

```text
[240]value0v(":callable")
[250]value0f(0)               ; cur_arity
[260]value0f(1)               ; max_arity, from one param x
[270]^0                       ; curried args = empty list
[280]value0v(":fn")

[290]^2 ref(80) ref(291)      ; fn payload list: params
[291]^2 ref(100) ref(292)     ; body
[292]^2 ref(120) ref(293)     ; with
[293]^2 ref(170) ref(294)     ; Edef
[294]^0

[300]^2 ref(280) ref(290)     ; [:fn, params, body, with, Edef]

[310]^2 ref(250) ref(311)     ; arity list [cur,max]
[311]^2 ref(260) ref(312)
[312]^0

[320]^2 ref(240) ref(321)     ; callable record list
[321]^2 ref(310) ref(322)     ; arity
[322]^2 ref(270) ref(323)     ; curried args
[323]^2 ref(300) ref(324)     ; payload
[324]^0
```

Pop third-field arg and set result:

```text
C = 320
E = 170
K = [CEK_K_ARG ref(180)]
```

Dereferenced result:

```text
[":callable",
  [0, 1],
  [],
  [":fn", params, body, with, Edef]]
```

Next pending arg is actual call arg:

```text
[180]^2 ref(50) ref(170)
[50]value0f(42)
[170]^0
```

Stop here. Next step applies function cell to actual arg.

Step 6: apply function cell to actual arg.

Current state:

```text
C = 320
E = 170
K = [CEK_K_ARG ref(180)]

[320]^2 ref(240) ref(321)     ; callable cell
[180]^2 ref(50) ref(170)      ; actual arg term
[50]value0f(42)
```

Hook condition:

```text
C dereferences to callable record:
  [":callable", [cur, max], args, payload]

top K is CEK_K_ARG ref(P)
[P]^2 ref(X) ref(EX)
```

For this example:

```text
cur = 0
max = 1
args = []
payload = [":fn", params, body, with, Edef]
X = 50
EX = 170
```

Since `cur + 1 == max`, call is saturated. `{fn}` is strict, so actual arg must be evaluated before body starts.

Allocate argument list with term `(X, EX)`:

```text
[330]^2 ref(50) ref(170)      ; term pair [root, env]
[340]^2 ref(330) ref(341)     ; args = [term(50,170)]
[341]^0
```

Update external CEK metadata:

```text
pop CEK_K_ARG ref(180)
push CEK_K_BIND {
  fn = 320,
  args = 340,
  arg_i = 0,
  env_caller = 170,
  frame.bindings = []
}
```

Set control to first arg term root/env:

```text
C = 50
E = 170
K = [CEK_K_BIND {...}]
```

Now current cell is:

```text
[50]value0f(42)
```

Since it is already a value and top `K` is `BIND`, next step binds it to parameter `x`.

Step 7: bind evaluated arg and enter body.

Current state:

```text
C = 50
E = 170
K = [CEK_K_BIND {
  fn = 320,
  args = 340,
  arg_i = 0,
  env_caller = 170,
  frame.bindings = []
}]

[50]value0f(42)
```

Dereference needed parts:

```text
fn_cell = 320
args = 340
i = 0
env_caller = 170
frame.bindings = []
```

Function payload in `fn_cell`:

```text
[320]^2 ref(240) ref(321)
[321]^2 ref(310) ref(322)
[322]^2 ref(270) ref(323)
[323]^2 ref(300) ref(324)

[300]^2 ref(280) ref(290)     ; [":fn", params, body, with, Edef]
[290]^2 ref(80) ref(291)      ; params
[291]^2 ref(100) ref(292)     ; body
[292]^2 ref(120) ref(293)     ; with
[293]^2 ref(170) ref(294)     ; Edef
```

Parameter `i = 0`:

```text
params = [80]^2 ref(70) ref(81)
first param descriptor = [70]^2 ref(1) ref(60)
[1]value0v(":id")
[60]^2 ref(10) ref(61)
[10]value0v("x")

param name cell = 10
```

Extend C-side frame builder with `x = 42`, then finalize frame into `cells_t` because body env will capture it:

```text
frame.bindings = [(10, 50)]

[370]^2 ref(10) ref(50)       ; binding pair [name,value]
[380]^2 ref(370) ref(381)     ; frame = [binding]
[381]^0
```

Allocate body env:

```text
[390]^2 ref(380) ref(170)     ; env = [frame, Edef]
```

Extract body expressions from body field:

```text
body = [100]^2 ref(20) ref(90)
[20]value0v(":do")
[90]^2 ref(70) ref(91)        ; exprs = [id(x)]
[70]^2 ref(1) ref(60)         ; [{:id}, x]
```

Allocate `RESTORE` payload:

```text
[400]ref(170)                 ; env_caller
```

Update external CEK metadata:

```text
pop CEK_K_BIND {...}
push CEK_K_RESTORE ref(400)
push CEK_K_DO {
  exprs = 90,
  next_i = 1
}
```

Set control to first body expression:

```text
C = 70
E = 390
K = [CEK_K_DO {...}, CEK_K_RESTORE ref(400)]
```

Current expression:

```text
[70]^2 ref(1) ref(60)         ; [{:id}, x]
```

Stop here. Body itself is not evaluated as opcode. Function-entry logic already unpacked `{:do}` and installed `CEK_K_DO`.

Step 8: evaluate body expression and restore caller env.

Current state:

```text
C = 70
E = 390
K = [CEK_K_DO { exprs = 90, next_i = 1 }, CEK_K_RESTORE ref(400)]

[70]^2 ref(1) ref(60)
```

`C` is identifier:

```text
[70]^2 ref(1) ref(60)
[1]value0v(":id")
[60]^2 ref(10) ref(61)
[10]value0v("x")
```

Identifier lookup uses current env:

```text
E = 390
[390]^2 ref(380) ref(170)     ; env = [frame, parent]
[380]^2 ref(370) ref(381)     ; frame list
[370]^2 ref(10) ref(50)       ; binding [x, 42]
[50]value0f(42)
```

Lookup result:

```text
C = 50
E = 390
K = [CEK_K_DO { exprs = 90, next_i = 1 }, CEK_K_RESTORE ref(400)]
```

Now `C` is value:

```text
[50]value0f(42)
```

Top `K` is `DO`:

```text
[90]^2 ref(70) ref(91)        ; exprs = [id(x)]
next_i = 1
```

Since `next_i == len(exprs)`, body is done. Pop `CEK_K_DO`; keep `C` unchanged:

```text
C = 50
E = 390
K = [CEK_K_RESTORE ref(400)]
```

Top `K` is now restore:

```text
[400]ref(170)
```

Restore caller env:

```text
pop CEK_K_RESTORE ref(400)
E = 170
C = 50
K = []
```

Final result:

```text
[50]value0f(42)
```

Whole call result is `42`.

State placement after this example:

```text
cells_t:
  source terms
  constructed function cell
  argument term list
  finalized frame cell
  body env cell
  ARG/RESTORE payload cells

CEK metadata:
  C
  E
  K entry kinds
  BIND frame builder while args evaluate
  DO cursor while statements run
```

Reason for lifted CEK state:

```text
BIND frame builder:
  transient mutable accumulator; serialized to cells_t only when body env is created

DO cursor:
  transient program counter through already-unpacked body expressions

ARG/RESTORE payload:
  tiny stable pairs; currently cell-backed, can be lifted too if K owns all control payloads
```

## Generic opcode stem

Proposed replacement for dedicated `op_fn0/op_fn1/op_fn2` families.

Invariant:

```text
unsaturated opcode = stem
saturated opcode   = fork
```

Opcode stem contains all state accumulated so far:

```text
[500]^1 ref(510)

[510]list(
  value0v(":opcode"),
  value0v(":fn"),
  value0f(0),              ; collected field count
  value0f(3),              ; required field count
  ^0                       ; collected fields
)
```

Apply one field:

```text
$ ref(500) ref(F)
```

Application dispatch order:

```text
1. inspect callee
2. if callee is opcode stem: run opcode-arity branch
3. otherwise: run ordinary reducer application
```

Reason: current reducer keeps intermediate rule-0 results inside its control stack. If reducer runs first, pending outer args can apply generic triage rules to opcode fork before CEK sees it.

Opcode-arity branch reads `count` and `required` from state at `510`.

If more fields remain after this argument, do not create fork. Allocate next stem directly:

```text
count' = count + 1
fields' = append(fields, F)
count' < required

[700]^1 ref(710)
[710]list(":opcode", ":fn", count', required, fields')
```

If this is last required field, allocate saturated fork:

```text
count + 1 = required

[800]^2 ref(510) ref(F)
```

Set `C = 800`. On next CEK step, saturated-opcode-fork check runs before pending `ARG` handling and before reducer. It dispatches `{fn}`, classifies three fields, and returns function callable cell.

Therefore:

```text
one-arg opcode:
  opcode check -> fork -> opcode dispatch

N-arg opcode:
  opcode check -> next stem
  opcode check -> next stem
  ...
  opcode check -> fork -> opcode dispatch
```

Reducer remains unchanged, but receives only non-opcode application:

```text
if not opcode_stem(callee):
  reducer(callee @ arg)
```

CEK step priority:

```text
1. saturated opcode fork -> opcode dispatch
2. APPLY cell           -> peel into K
3. opcode stem + ARG    -> opcode-arity branch
4. other value + ARG    -> reducer
5. continuation result  -> BIND/DO/RESTORE
6. empty K              -> halt
```

Fork never represents unsaturated opcode. Reducer never receives opcode stem application or opcode fork application.

Consequence for current `{fn}` example: replace `op_fn0/op_fn1/op_fn2` with repeated generic `^1 opcode_state` / `^2 opcode_state field` transitions.
