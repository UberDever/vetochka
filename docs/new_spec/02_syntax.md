# Syntax

> As above, so below

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
literal         ::= string_literal | integer_literal

identifier ::= identifier_start identifier_continue*
identifier_start ::= [a-zA-Z_]
identifier_continue ::= [a-zA-Z0-9_]
  | "?" | "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
  | "!" | "&" | "|"

special_dollar ::= "$"
special_tilde  ::= "~"

operator_run ::= operator_char+
operator_char ::= "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
                | "!" | "&" | "|" | ":"
```

`do` and `end` are reserved block words. `$` is a distinct special token, a quasiidentifier. It can be considered a single built-in identifier across the whole system. `~` is a distinct special token, the nyad sigil; it is not an operator and heads only the nyad form below. A lone `:` is punctuation, not an operator; multi-character operators may contain `:` (e.g. `::`).

An operator run is classified by adjacency; trivia breaks gluing:

1. glued to a preceding identifier: continues that identifier;
2. glued to a preceding literal, `)`, or `]`: syntax error;
3. otherwise, glued to its right neighbor: `op_prefix`;
4. otherwise: `op_infix`.

`balanced_utf8_bytes` permits balanced braces without escape syntax. The exact
scanner algorithm is implementation detail.

## Automatic semicolon insertion

A physical newline becomes virtual `;` iff:

1. lexical context is layout-active;
2. `...` does not suppress it;
3. previous significant token can end an expression.

Layout-active contexts: source and bare `do ... end` blocks.

Layout-inactive contexts: `(...)`, `[...]`, `@[...]`, `~[...]`.

Expression-ending tokens: literal, identifier, `$`, `)`, `]`, `end`.

After insertion, virtual and written semicolons parse identically.

## Grammar

```ebnf
source ::= block_list? eof

expression ::= annotation? infix_expression

annotation ::= "@[" comma_list "]"

infix_expression ::= prefix_expression (op_infix prefix_expression)*
prefix_expression ::= op_prefix* postfix_expression

postfix_expression ::= primary tight_postfix* loose_postfix*

primary ::= literal
          | identifier
          | special_dollar
          | nyad
          | "[" comma_list? "]"
          | "(" expression ")"

nyad ::= "~" "[" ( expression ( "," expression )? )? "]"

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
                           (op_infix prefix_expression_tight)*
prefix_expression_tight ::= op_prefix* postfix_expression_tight
postfix_expression_tight ::= primary tight_postfix*

block_list ::= expression (";" expression)* ";"?
comma_list ::= expression ("," expression)* ","?
```

`f()` is equivalent to `f(~[])`.

A bare `do ... end` is block argument syntax. `do:` and `end:` are also allowed as labels.

`$` participates in postfix application exactly as an identifier callee would; it is only lexically special.

`~` sigil is used to encode exact stem or fork and avoid unnecessary application. `~[](x)` is three nodes: application with lhs `~[]` and rhs `x`; `~[x]` is two nodes with `~[]` nyad root and `x` lhs. Hence, `~[]` is a leaf, `~[x]` an exact stem, `~[x, y]` an exact fork; `~[x, y, z]` is a syntax error.

# Intensionality

## Rewrite rules

To support intensionality, syntax above is lowered into simpler terms, representable by the same syntax. But there's a single
exception.
`{@}` marks application and isn't part of the syntax. They are shown simply to clarify how these nodes would be translated to cells later.
In the current notation they can be considered strict binary nodes with positional application, i.e. `f(x) -> {@} f x`.

```text
1. x                    -> [{:id}, {x}]
2. $                    -> [{:id}, {$}]
3. [x, y]               -> ~[ [{:id}, {x}], ~[ [{:id}, {y}], ~[] ] ]
4. (expr)               -> [{:group}, expr]
5. f(x, y)              -> {@} ({@} f x) y
6. f label: expr        -> {@} f [{:label}, {label}, expr]
7. f do a; b end        -> {@} f [{:block}, a, b]
8. @[a, b] expr         -> [{:annot}, ~[a, ~[b, ~[]]], expr]
9. f[x, y]              -> {@} f ~[x, ~[y, ~[]]],
10. f{bytes}            -> {@} f {bytes}
11. prefix-op expr      -> [{:prefix}, {op}, expr]
12. x op y op z         -> [{:infix}, {op}, x, y, z]  ;; mixed operator chains are allowed; analyzed at vf stages
13. base.name           -> [{:selector}, base, {name}]
```

List syntax encodes proper lists, using simple lisp translation `[a, b, c] -> ~[a, ~[b, ~[c, ~[]]]]`.

Note that the resulting tree is fully inert by itself, it isn't executed until it comes into executable position, see [v0
execution rules](#v0-cesk).

Preserving application marker nodes allows to construct and inspect arbitrary application trees without the need for their
execution.

# Representation

## Cells

Cells are used to store compact tree-shaped data and make up a runtime system memory.

Every node is typed (as opposed to `tree-calculus`) and has its arity: `0, 1 or 2`. Note that semantically a node
could have any amount of children if proper list encoding is used.

There are following node types:

1. `nyad0`
    + Represents `~[]` without children, a leaf
    + Pith: `~[]`
2. `nyad1`
    + Represents `~[x]` with a child, a stem
    + Pith: `~[x]`
3. `nyad2`
    + Represents `~[x, y]` with 2 children, a fork
    + Pith: `~[x, y]`
TODO: add more
    

`Pith` is a "view" into the node that is generated on demand when the node enters
 [`rule 3` family rule of `v0`](#triage-calculus). On more detail see [pith section](#pith).

## Pith

Pith are essential internals of a node that are introspectable by `rule 3`. 
They are dual, meaning: if there's an observable pith of a certain node, a pith constructed in a same way from scratch
represents such node. This allows for intensionality not only for `tree-calculus` level of `v0`, but also for other runtime
entities, such as opcodes.

\newpage
