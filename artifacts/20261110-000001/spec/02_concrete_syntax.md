# Vetochka shared syntax and cells

## Source text and trivia

Source text is UTF-8.

```ebnf
t_hws ::= 09 | 0B | 0C | 20
t_nl  ::= 0A | 0D 0A | 0D

line_comment       ::= ";;" <until newline or eof>
block_comment      ::= "#|" <nestable contents> "|#"
line_continue      ::= "..." t_hws* line_comment? t_nl
structured_comment ::= "#;" trivia* expression

trivia_no_nl ::= t_hws | block_comment | structured_comment
trivia       ::= trivia_no_nl | t_nl | line_comment | line_continue
```

`#;` discards its following parsed expression.

## Tokens

```ebnf
string_literal  ::= "{" balanced_utf8_bytes "}"
integer_literal ::= [1-9][0-9]* | "0"
delta_literal   ::= "^"
literal         ::= string_literal | integer_literal | delta_literal

identifier ::= identifier_start identifier_continue*
identifier_start ::= [a-zA-Z_]
identifier_continue ::= [a-zA-Z0-9_]
  | "?" | "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
  | "!" | "&" | "|"

special_dollar ::= "$"

operator ::= operator_char+
operator_char ::= "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
                | "!" | "&" | "|" | ":"
```

`do` and `end` are reserved block words. `$` is a distinct special token, not an
identifier and not an operator. `:` by itself is punctuation, not an operator.
Operators must be whitespace-separated; otherwise operator characters continue an
identifier.

`balanced_utf8_bytes` permits balanced braces without escape syntax. The exact
scanner algorithm is implementation detail.

## Automatic semicolon insertion

A physical newline becomes virtual `;` iff:

1. lexical context is layout-active;
2. `...` does not suppress it;
3. previous significant token can end an expression.

Layout-active contexts: source and bare `do ... end` blocks.

Layout-inactive contexts: `(...)`, `[...]`, `@[...]`.

Expression-ending tokens: literal, identifier, `$`, `)`, `]`, `end`.

After insertion, virtual and written semicolons parse identically.

## Grammar

```ebnf
source ::= block_list? eof

expression ::= annotation? infix_expression

annotation ::= "@[" comma_list "]"

infix_expression ::= prefix_expression (operator prefix_expression)*
prefix_expression ::= prefix_operator* postfix_expression

postfix_expression ::= primary tight_postfix* loose_postfix*

primary ::= literal
          | identifier
          | special_dollar
          | "[" comma_list? "]"
          | "(" expression ")"

tight_postfix ::= "." identifier
                | "(" comma_list? ")"
                | "[" comma_list? "]"
                | string_literal

loose_postfix ::= block_argument | labeled_argument

block_argument ::= "do" block_list? "end"

labeled_argument ::= label ":" argument_expression
label ::= identifier | "do" | "end"

argument_expression ::= annotation? infix_expression_tight
infix_expression_tight ::= prefix_expression_tight
                           (operator prefix_expression_tight)*
prefix_expression_tight ::= prefix_operator* postfix_expression_tight
postfix_expression_tight ::= primary tight_postfix*

prefix_operator ::= "!" | "-" | "~" | "*" | "&"

block_list ::= expression (";" expression)* ";"?
comma_list ::= expression ("," expression)* ","?
```

`f()` is equivalent to `f(^)`.

A bare `do ... end` is block argument syntax. `do:` and `end:` are also allowed as labels.
`$` participates in postfix application exactly as an identifier callee would; it is only lexically special.

## v0 admission

All shared forms parse into inert cell data. `03_v0.md` assigns v0 execution meaning
only to its stated forms; other forms remain data for vf or user protocols. Parsing
does not execute, resolve names, or create runtime bindings. Spans/locations are
diagnostic metadata, not syntax meaning.

## Parsed cells

`{@}` marks application cell data. It does not conflict with source annotation
punctuation `@[`.

```text
x                   -> [{:id}, {x}]
$                   -> [{:id}, {$}]
[x, y]              -> ^ [{:id}, {x}] (^ [{:id}, {y}] ^)
(expr)              -> [{:group}, expr]
f(x, y)             -> {@} ({@} f x) y
f label: expr       -> {@} f [{:label}, {label}, expr]
f do a; b end       -> {@} f [{:block}, a, b]
@[a, b] expr        -> [{:annot}, ^ a (^ b ^), expr]
f[x, y]             -> {@} f (^ x (^ y ^))
f{bytes}            -> {@} f {bytes}
prefix-op expr      -> [{:prefix}, {op}, expr]
x op y op z         -> [{:infix}, {op}, x, y, z]
base.name           -> [{:selector}, base, {name}]
```

List syntax encodes proper lists; it has no list tag. `{@}` encodes Layer 1 `APPLY`.
Source tags remain inert cell data.

## Open

- Exact parse-diagnostic term shape.
- Future literal extensions require explicit cross-layer specification.

## Cells

Cells store compact tree-shaped data.

A cell root denotes a stored node graph. Nodes are typed and have storage arity:

```text
arity 0: node with no children
arity 1: node with one child
arity 2: node with two children
```

Leaf/stem/fork are Layer 0 triage shapes. Layer 1 arity is storage structure.
Only `DELTA0/1/2` directly represent those triage shapes.

Cells are inert. Storing a node never evaluates it, dispatches it, performs an
effect, or resolves a name. Evaluation belongs to later layers.

Cells use a portable binary ABI. Registry: [Runtime cell node type
registry](03_v0.md#runtime-cell-node-type-registry).

### Rationale

v0 needs a concrete representation that preserves tree shape and remains
inspectable. Separating cells from evaluation prevents old designs from smuggling
machine behavior into storage.

`APPLY` is a separate node because application structure must survive as data.
Encoding application only by adjacency or stack scheduling loses homoiconic
structure needed by runtime work.

### Open

- Exact direct node families and wire tags for concrete opcode states, TERM, and
  other runtime nodes. They are real node families, not payload encodings hidden
  inside `VALUE*`.
- Exact stable public cell structure and Rule 3 shape projection for each opcode
  state. Private opcode state must stay unforgeable.
- Canonical text serialization for cells. Current code can format parsed source
  trees canonically and can encode source trees into cells; it does not yet define
  a canonical cell-to-text format.

### Notes

- Current C code exposes `CELLS_NODE_TYPE_ITEMS` in `reducer/cells_api.h` with
  `DELTA*`, `VALUEF*`, `VALUEV*`, `APPLY`, old `OP_FN*`, and `REF`. The matching
  wire ABI lives in `reducer/cells_impl.h` / `reducer/cells_cells.c`.
- Current C code still contains `OP_FN0/1/2`; they are historical implementation
  artifacts for an older function opcode experiment, not part of this spec.
- Historical hint, 2026-02-04: earlier notes tried to make every node uniformly
  leaf/stem/fork-shaped and inspectable via `get_type` / `get_payload`. Current
  spec keeps the useful storage idea but does not decide runtime inspection here.
- Historical hint, 2026-02-09: earlier notes explored compact byte tags and
  variable-width refs. Current spec leaves exact wire encoding open.
