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
