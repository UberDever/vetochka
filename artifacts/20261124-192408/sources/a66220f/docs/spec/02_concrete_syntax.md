# Vetochka concrete syntax

Status: draft shared syntax specification, recomposed 2026-07-11.

This document specifies syntax shared by all Vetochka languages. It specifies
parsing and normalization only; it does not assign runtime meaning. `03_v0.md`
says which forms v0 admits. `04_vf.md` says which additional forms vf languages
may interpret.

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

## Form ownership

All forms above are parsed by shared syntax. Language documents assign admission
and meaning:

| Form | v0 | vf |
|---|---|---|
| literals, identifiers, `$`, grouping, lists, `(...)` calls, labels, `do ... end` blocks | admitted | inherited |
| annotations, selectors, bracket calls, byte calls, prefix, infix | not admitted | may be interpreted |

Parser recognition does not itself assign runtime meaning.

## Normalization

Parsing produces inert normalized term data. No language-level quote operation is
involved.

Application uses `{@}` as normalized-term marker. It is byte data in a normalized
term; it does not conflict with source annotation punctuation `@[`.

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

Ordinary list syntax always desugars to proper lists. A language needing a special
source-level list form must introduce an explicit tag for that form.

A labeled argument lowers to generic tagged data: `name: expr` becomes
`[{:label}, {name}, expr]`.

`{@}` normalizes to Layer 1 `APPLY` structure when the term is encoded. Source tags
are inert data until some language protocol interprets them.

## Open

- Exact parse-diagnostic term shape.
- Exact grammar for special source-level list forms, if one is ever needed.
- Future literal extensions require explicit cross-layer specification.
