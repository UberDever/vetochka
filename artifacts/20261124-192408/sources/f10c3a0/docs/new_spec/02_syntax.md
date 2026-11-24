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
delta_literal   ::= "^0"
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

`do` and `end` are reserved block words. `$` is a distinct special token, a quasiidentifier. It can be considered a single built-in identifier across the whole system. `:` by itself is punctuation, not an operator.
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

Layout-inactive contexts: `(...)`, `[...]`, `@[...]`, `^1[]`, `^2[]`.

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
          | fixed_delta
          | "[" comma_list? "]"
          | "(" expression ")"

fixed_delta ::= "^1[" expression "]"
                | "^2[" expression "," expression "]"

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

`f()` is equivalent to `f(^0)`.

A bare `do ... end` is block argument syntax. `do:` and `end:` are also allowed as labels.

`$` participates in postfix application exactly as an identifier callee would; it is only lexically special.

Fixed arity `^1[x]` and `^2[x, y]` encode exact stem or fork and avoid unnecessary application. `^0(x)` is three nodes: application with lhs `^0` and rhs `x`; `^1[x]` is two nodes with `^1` root and `x` lhs.

# Intensionality

## Rewrite rules

To support intensionality, syntax above is lowered into simpler terms, representable by the same syntax. But there's a single
exception.
`{@}` marks application and isn't part of the syntax. They are shown simply to clarify how these nodes would be translated to cells later.
In the current notation they can be considered strict binary nodes with positional application, i.e. `f(x) -> {@} f x`.

```text
1. x                    -> [{:id}, {x}]
2. $                    -> [{:id}, {$}]
3. [x, y]               -> ^2[ [{:id}, {x}], ^2[ [{:id}, {y}], ^0 ] ]
4. (expr)               -> [{:group}, expr]
5. f(x, y)              -> {@} ({@} f x) y
6. f label: expr        -> {@} f [{:label}, {label}, expr]
7. f do a; b end        -> {@} f [{:block}, a, b]
8. @[a, b] expr         -> [{:annot}, ^2[a, ^2[b, ^0]], expr]
9. f[x, y]              -> {@} f ^2[x, ^2[y, ^0]],
10. f{bytes}            -> {@} f {bytes}
11. prefix-op expr      -> [{:prefix}, {op}, expr]
12. x op y op z         -> [{:infix}, {op}, x, y, z]
13. base.name           -> [{:selector}, base, {name}]
```

List syntax encodes proper lists, using simple lisp translation `[a, b, c] -> ^2[a, ^2[b, ^2[c, ^0]]]`.

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

1. `delta0`
    + Represents `^0` without children, a leaf
    + Pith: `^0`
2. `delta1`
    + Represents `^1` with a child, a stem
    + Pith: `^1[x]`
3. `delta2`
    + Represents `^2` with 2 children, a fork
    + Pith: `^2[x, y]`
TODO: add more
    

`Pith` is a "view" into the node that is generated on demand when the node enters
 [`rule 3` family rule of `v0`](#triage-calculus). On more detail see [pith section](#pith).

## Pith

Pith are essential internals of a node that are introspectable by `rule 3`. 
They are dual, meaning: if there's an observable pith of a certain node, a pith constructed in a same way from scratch
represents such node. This allows for intensionality not only for `tree-calculus` level of `v0`, but also for other runtime
entities, such as opcodes.

\newpage
