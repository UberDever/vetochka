⬜ ✅ ❌

# Grammar

## Rationale

The main advantage of this grammar is that it allows for relatively readable common
imperative code while being fully homoiconic and unbound in terms of keywords. The only keywords
are (`end` and `do`) and only other reserved syntax objects are operators and literals, the rest is
just structure, as triage-calculus bequeathed.

## Base language

This language is a form of sugar that represents 4 main entities of a VM+reducer:
1. Application
2. Triage nodes `^`
3. Integer `i64` literals
4. String literals (aka byte utf-8 sequences)

I would call this language `vetochka0` or `v0`. In contrast, language with
extended syntax and semantics is `vetochka*` or `v*`, since I don't know the
exact semantical level beneath such language: this is user's right.

The pipeline for `v0` is as follows: it parsed into a single expression
with different tagged nodes (ir) and later lowered into VM+reducer cells.
Executable surface of the language is very small: currently we have triage nodes `^`
and string literals `{fn}` carrying the semantics. "Compilation" of this language to cells
is straightforward.

On the other hand, `v*` is compiled in `v0` itself, thanks to `vetochka` reflectivity.
An extended syntax allows for great expressibility of source-level terms, that is, module-system,
datatypes, rich functional features, sugary constructs -- all built from `v*` terms by `v0` bootstrap/compile step to
`v0` language itself.

Encoding is utf-8 text.

Comments: 
1. `;;` till the end of the line or eof.
2. Nestable: `#|` and `|#`
3. Structured: `#;` (comments out next expression in its entirety, doin it on parser level)

### Tokens

```ebnf
t_hws ::= 09 | 0B | 0C | 20
t_nl  ::= 0A | 0D 0A | 0D

line_comment ::= ";;" <until newline or eof>

block_comment ::= "#|" <nestable contents> "|#"

line_continue ::=
    "..." t_hws* line_comment? t_nl

structured_comment ::=
    "#;" trivia* expression

trivia_no_nl ::=
    t_hws
  | block_comment
  | structured_comment

trivia ::=
    trivia_no_nl
  | t_nl
  | line_comment
  | line_continue

(* 
    t_literal_chars can include any utf-8 character;
    It also can include any number of "}" as long as they are balanced;
    With conjunction with identifiers, they can encode different literals like
    i64{1234} or f32{-5.4e9}
*)
t_string_literal ::= "{" t_literal_chars "}"

t_integer_literal ::= [1-9][0-9]* | [0-9]

t_literal ::= t_string_literal | t_integer_literal | "^" | "Δ"

(*excluding "do", "end"*)
t_identifier ::=
    identifier_start identifier_continue*

identifier_start ::=
    [a-zA-Z_]

identifier_continue ::=
    [a-zA-Z0-9_]
  | "?" | "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
  | "!" | "&" | "|"

t_colon ::= ":"

(* except exact ":" *)
(*operators must be separated by whitespace, in other case they
are considered as part of identifier*)
(*An infix chain is valid only if all operator tokens in the chain are byte-identical*)
t_operator ::= operator_run
             

operator_run ::= operator_char+
operator_char ::= "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
                | "!" | "&" | "|" | ":"
```

### Automatic semicolon insertion

Newlines may be converted into virtual semicolon tokens before parsing.

A physical newline is converted to `;` iff:

1. the current lexical context is layout-active;
2. the newline is not suppressed by `...`;
3. the previous significant token can end an expression.

Layout-active contexts are:

```txt
source
do ... end block
```

Layout-inactive contexts are:

```txt
(...)
[...]
@[...]
```

Tokens that can end an expression:

```txt
t_literal
t_identifier
")"
"]"
"end"
```

The closing `]` of an annotation `@[ ... ]` does not trigger semicolon insertion.

`...` before a newline suppresses semicolon insertion and continues the expression on the next line.

After ASI, virtual semicolons are parsed exactly like explicit `;`.


### Syntax

```ebnf
source ::= block_list? eof

expression ::=
    annotation? infix_expression

annotation ::=
    "@[" expression "]"

infix_expression ::=
    prefix_expression (t_operator prefix_expression)*

prefix_expression ::=
    prefix_operator* postfix_expression

postfix_expression ::=
    primary tight_postfix* loose_postfix*

tight_postfix ::=
    "." t_identifier
  | "(" comma_list? ")"
  | "[" comma_list? "]"
  | t_string_literal

loose_postfix ::=
    block_argument
  | labeled_argument

block_argument ::=
    "do" block_list? "end"

labeled_argument ::=
    t_identifier t_colon argument_expression

argument_expression ::=
    annotation? infix_expression_tight

infix_expression_tight ::=
    prefix_expression_tight (t_operator prefix_expression_tight)*

prefix_expression_tight ::=
    prefix_operator* postfix_expression_tight

postfix_expression_tight ::=
    primary tight_postfix*

primary ::=
    t_literal
  | t_identifier
  | "[" comma_list? "]"
  | "(" expression ")"

prefix_operator ::= "!" | "-" | "~" | "*" | "&"

block_list ::=
    expression (";" expression)* ";"?

comma_list ::=
    expression ("," expression)* ","?
```

Special rule: `f()` is equivalent to `f(^)`

### CST normalization

The parser CST is normalized into one bootstrap syntax term before cell encoding.
Punctuation and parser-only wrapper nodes are discarded. Every non-core syntax record is
a list whose first element is a colon-prefixed string tag. Applications use the explicit
binary `{$}` form.

| CST form                         | Bootstrap syntax term                            |
|----------------------------------|--------------------------------------------------|
| `source`                         | normalized top-level `block_list`                |
| `^`                              | `^`                                              |
| integer literal `n`              | `n`                                              |
| string literal `{bytes}`         | `{bytes}`                                        |
| identifier `name`                | `[{:id}, {name}]`                                |
| `[item1, ..., itemN]`            | `[{:list}, item1, ..., itemN]`                   |
| `block_list`                     | `[{:block}, expr1, ..., exprN]`                  |
| `(expr)`                         | `[{:group}, expr]`                               |
| `@[ann] expr`                    | `[{:annot}, ann, expr]`                          |
| `f(arg1, ..., argN)`             | left fold: `{$} ... ({$} f arg1) ... argN`       |
| `f()`                            | `{$} f ^`                                        |
| `f[arg1, ..., argN]`             | `{$} f [{:list}, arg1, ..., argN]`               |
| `f{bytes}`                       | `{$} f {bytes}`                                  |
| `base.name`                      | `[{:selector}, base, {name}]`                    |
| `f name: expr`                   | `{$} f [{:label}, {name}, expr]`                 |
| `f do expr1; ...; exprN end`     | `{$} f [{:block}, expr1, ..., exprN]`            |
| prefix `op expr`                 | `[{:prefix}, {op}, expr]`                        |
| infix chain `a op b op c`        | `[{:infix}, {op}, a, b, c]`                      |

Rules are recursive. Infix chains remain flat; normalization does not choose
associativity.

### Cell encoding

Cell encoding is a mechanical implementation step. It recursively
encodes the bootstrap syntax term:

| Bootstrap syntax            | Cell encoding                                                     |
|-----------------------------|-------------------------------------------------------------------|
| `^`                         | `DELTA0`                                                          |
| integer `n`                 | `VALUEF0(n)`                                                      |
| string `{bytes}`            | `VALUEV0(bytes)`                                                  |
| `{$} f x`                   | `APPLY` node referencing encoded `f` and `x`                      |
| `[x1, ..., xN]`             | `^ x1 ^ x2 ... ^ xN ^`                                            |
| `{$} {some_opcode} x`       | `APPLY` referencing the `SOME_OPCODE` stem and encoded `x`        |

Opcode protocol:

```elixir
SOME_OPCODE [
    {:call},
    [cur_arity, max_arity],
    [curried_args],
    stuff_to_apply
]

APPLY(
    SOME_OPCODE(payload),
    argument
)
    -> VM(payload, argument)
    -> value | SOME_OPCODE(updated_payload)
```
