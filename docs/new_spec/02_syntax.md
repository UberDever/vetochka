# Core syntax

## Motivation and examples

The purpose of the core syntax is to express fundamental terms of the language without
sugar and shorthands. Conceptually, it can represent every feature `vetochka` has
while still residing in the `triage-calculus` domain.

This syntax can express only nyads and applications.

Nyad is a trivial value, it could be compared to a type-theoretical `unit` in the sense that
it doesn't bear any information by itself. Rather, the usefulness 
of nyads comes from their composition -- they form a homogenous tree that is used to encode
all programming language constructs you would imagine. The interaction of these trees is described
in later chapters.

We would call a nyad a `leaf` when it has no children, a `stem` when it has one and a `fork` when it has two.
A nyad can't hold more than two children.

We've introduced only proper-list shorthand to this syntax, since it is core enough to
get its own notation.

```vf
[]     == ~
[x]    == ~[x, ~]
[x, y] == ~[x, ~[y, ~]]
...
```

Below are the examples of nyads and structures they form.

```vf
~; ;; a leaf
~[~, ~]; ;; a fork
~(~) ;; an application
~[~, ~[~, ~]] ;; a proper list of two leafs, ending by a nil-like leaf nyad
~[~, ~[~, ~[~, ~[~, ~]]]] ;; a number 4 using Peano-style encoding
[[], [], [], []] ;; the same number, but using proper list notation explicitly
```

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

## Grammar

We would use `NO_WS` marker to represent "gluing" of tokens, i.e. no trivia could be contained between the two.
On the opposite, `WS` requires at least one trivia item. We use these markers to separate some productions,
for example an infix and prefix operators (see later sections).

```ebnf
source ::= block_list? eof
block_list ::= expression (";" expression)* ";"?
comma_list ::= expression ("," expression)* ","?

expression ::= postfix_expression
postfix_expression ::= primary (NO_WS "(" comma_list? ")")*

primary ::= 
    nyad
    | "[" comma_list? "]"
    | "(" expression ")"

leaf ::= "~"
nyad ::= leaf | "~" NO_WS "[" expression ( "," expression )? "]"
```

A `~` immediately followed by `[` starts a stem or fork, never a leaf.

# General syntax

## Motivation and examples

This syntax unifies different `vetochka` branches (called `vf`) under a single convenience umbrella.
It mainly consists of shorthands and sugar to simplify writing programs. However,
it is also used to represent intent of the programmer more clearly. An integer
or a key-value pair is clearer than a tree of a specific corresponding structure.

Please note: this syntax mostly is deliberately **not** executable. Some native
operations and objects have their semantics (see [v0](TODO:)), but, aside from
that, every construction is just a pure syntax sugar, the meaning of which
assigned by the interpreter you write yourself.

A bunch of examples:

```vf
;; literals
1234
{outer {inner} text}        ;; balanced braces inside a string
{веточка}                   ;; UTF-8 string content

;; different forms in one list, note that the latter is an identifier also
[1234, {text}, stuff1234, empty?, x-y']

;; some applications
(f + g)(x)                  ;; an application of a grouped expression
f[x, y,]                    ;; equivalent to f([x, y])
f{bytes}                    ;; equivalent to f({bytes})
(~)[x]                      ;; list application to a leaf, since ~[x] is a stem

;; on operators
x :: y                      ;; an infix form
x . y                       ;; dot is an ordinary infix operator
::x                         ;; a prefix form
x#|left|#+#|right|#y        ;; comments satisfy WS on both sides of +
-123                        ;; prefix application, not an integer literal with a sign
x + -y                      ;; this parses as you expect it would

;; labeled expressions
name: {Ada}
result: x + y
:x                         ;; omitted label name
;; a nested label requires grouping, because labels associate loosely
outer: (inner: x)
;; labels are first-class, you can use them as argument/list entries
f([name: {Ada}, age: 22], count: 3)

;; you can also use labels and blocks for applications
f x: 1 y: 2                 ;; smalltalk-esque
f :x                        ;; equivalent to f(:x)
f(x) option: y              ;; tight postfixes precede loose postfixes
do x; y end                 ;; blocks are first-class and contain expressions
f do x; y end               ;; you can also apply to them
value at:field do:x         ;; grouped as (value at:field) do:x, use instead of `value.field(x)`
;; combining stuff together => rich DSL
f(x) with: stuff do print(g) end

;; closures, your general lambda calculus
\[x] x
\[a, b, c] (a(c))(b(c))
\[x] \[y] x(y)

;; there are tags in the language, tags could be of any term
@[{note}] x
@[[a: b, c: d]] foo            ;; if you need a list of kv values, you could have it
@[[]] foo                      ;; empty metadata
@[dangerous] 42 / 0            ;; could also assign to expressions
(\[foo] @(foo))(@[{stuff}] 42) ;; decompose into [{stuff}, 42]

;; some syntax niceties
top-level                   ;; usually don't need a semicolon, see ASI
f do
    inner1
    inner2
end
;; a continuation suppresses semicolon insertion
x ...
    + y

;; comments
x ;; a line comment
f(#| a block comment |# x)
f(#| outer #| nested |# comment |# x)
[a, #;discarded(x) b]       ;; the structured comment discards discarded(x)
```

## Tokens

General syntax strictly extends the core syntax, so every production below explicitly
contains a core language production of the same name.

```ebnf
string_literal  ::= "{" balanced_utf8_bytes "}"
integer_literal ::= [1-9][0-9]* | "0"
(* Floating-point syntax: task 20261020-103514. *)
literal         ::= string_literal | integer_literal

identifier ::= identifier_start identifier_continue*
identifier_start ::= [a-zA-Z_]
identifier_continue ::= [a-zA-Z0-9_]
  | "?" | "'" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
  | "!" | "&"

operator_run ::= operator_char+
operator_char ::= "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">"
                | "!" | "&" | "|" | ":" | "^" | "."
```

Here, `do` and `end` are reserved block words. 

We exclude a lone `:`: 
it is punctuation, not an operator, and operators may contain `:` (e.g. `::`).
We also exclude `...` since it's used as line continuation marker.

Identifiers and operator runs consume the longest matching sequence of characters.
Thus, `x-y` is one identifier, and `!!x` starts with one operator run, `!!`.

`balanced_utf8_bytes` permits balanced braces without escape syntax. The exact
scanner algorithm is an implementation detail.

## Automatic semicolon insertion

A physical newline becomes virtual `;` iff:

1. lexical context is layout-active;
2. `...` does not suppress it;
3. previous significant token can end an expression;
4. previous significant token does not close an annotation header or closure parameter list.

Layout-active contexts: source and bare `do ... end` blocks.

Layout-inactive contexts: `(...)`, `[...]`, `@[...]`, `@(...)`, `~[...]`, `\[...]`.

Expression-ending tokens: literal, identifier, `~`, `)`, `]`, `end`.

After insertion, virtual and written semicolons parse identically.

Note that since an inserted semicolon is a token, it does not satisfy `WS` marker.

## Grammar

```ebnf
source ::= block_list? eof
block_list ::= expression (";" expression)* ";"?
comma_list ::= expression ("," expression)* ","?

expression ::= labeled_expression | block | tagged_expression

block ::= "do" block_list? "end"

labeled_expression ::= (label_word NO_WS)? ":" argument_expression
label_word ::= identifier | "do" | "end"

tagged_expression ::= tag? (closure | infix<postfix_expression>)
tag ::= "@[" tagged_expression "]"

argument_expression ::= tag? (closure_tight | infix<postfix_expression_tight>)

closure ::= closure_head expression
closure_tight ::= closure_head (block | argument_expression)
closure_head ::= "\" NO_WS "[" identifier ("," identifier)* ","? "]"

infix<P> ::= prefix<P> (WS operator_run WS prefix<P>)*
prefix<P> ::= (operator_run NO_WS)* P

postfix_expression ::= primary tight_postfix* (WS loose_postfix)*
postfix_expression_tight ::= primary tight_postfix*

tight_postfix ::= NO_WS (
                    "(" comma_list? ")"
                  | "[" comma_list? "]"
                  | string_literal
                )
loose_postfix ::= block | labeled_expression

primary ::= literal
          | identifier
          | nyad
          | untag
          | "[" comma_list? "]"
          | "(" expression ")"

nyad ::= leaf | "~" NO_WS "[" tagged_expression ( "," tagged_expression )? "]"
untag ::= "@(" tagged_expression ")"
```

Some clarifications:

We've used extended EBNF with parametrization. It simplifies rules and keeps the important destinction:
some productions associate tightly, some loosely.

`f()` is equivalent to `f(~)`.

Tight postfixes associate left to right. Each attaches to the accumulated expression:
`f(x)[y]{z}` groups as `((f(x))[y]){z}`.

`-x` is a prefix expression, and `x - y` is an infix expression.
`- x`, `x -y`, `1-2`, and `f(x)-y` are syntax errors.
Infix operators require trivia on both sides.

A labeled expression consumes the longest
`argument_expression` as its payload. Nesting requires parens: `x: (y: 1)`.

## Lowering

The main purpose of lowering is to allow the programmer to simplify reasoning about program structure,
while keeping syntax somewhat human-readable and concise. This section descibes only
syntax-level transformations. There are also [piths](TODO:), which allow
you to inspect and build runtime-constructed objects beyond syntax. These
primarily address native-side objects, like closures or numbers.

Note that these lowered terms don't necessarily stored as-is during runtime. But
from the standpoint of the language, you could be sure that these would behave exactly as
their core syntax form does.

### Tagging

First, we define a lowering for tag/untag operations. Their terms are
straightforward, but opaque to human readers. Instead, you
could be interested in the guarantees, written below the definitions.
In these rewrite schemas, `m`, `v`, and `e` stand for arbitrary terms.

```vf
@[m] v
    => ~[~[~[~[~[[], ~[[]]]], v]], ~[[], m]]

@(e)
    => [
        [[]],
        [[], [[]], [],
            ~[~[[], ~[[[]]]]],
            ~[~[[], ~[[]]]],
            ~[~[[], ~[~[[]]]]],
            ~[~[[], ~[[]]]],
            ~[[]], []]
    ](e)

;; it is guaranteed that
@(@[m] v)   => [m, v]
(@[m] v)(x) => v(x)  ;; tagging doesn't change the application behaviour
```

Note, that `untag` is **unchecked**, which would give you unspecified results in case
of incorrect inputs. You shouldn't rely on it when dealing with arbitrary trees.
A well-formed tagged value always has a tag. No tag is just an empty proper list.

### Rewrites

To describe rewrites, we use the core syntax, with certain additions:
- `<term>` for a source-term placeholder on the left and its recursively lowered form on the right
- set of markers `native-*<...>` for machine-native values
- shorthands for tagging operations, defined above
- proper lists instead of raw nyads where applicable

Child terms retain their own tags and source metadata. Native markers denote raw
machine-native values; they do not implicitly add tags.

Special term `<meta>` is used to describe syntactical metadata that
is spliced into the term's tag during parsing. It consists of the following
key-value pairs:
- a `line` with an `i64` describing term source code location; value of `0` could
be used to indicate that term is synthetic
- a `col` that describes the original source code column, behaves similar to `line` 

Rewrite rules themselves:

```vf
1234                  => @[[native-string<:i64>, <meta>]] native-integer<1234>
{bytes}               => @[[native-string<:str>, <meta>]] native-string<bytes>
x                     => @[[native-string<:id>, <meta>]] native-id<x>

~                     => ~
~[<x>]                => ~[<x>]
~[<x>, <y>]           => ~[<x>, <y>]

[]                    => ~
[<x>, <y>, <z>]       => ~[<x>, ~[<y>, ~[<z>, ~]]]

;; grouping doesn't rewrite into anything and
;; instead is purely syntactical

;; parenthesized, list, and string application
f()                   => @[[native-string<:app>, <meta>]] ~[<f>, ~]
f(x, y)               => 
    @[[native-string<:app>, <meta>]] ~[
        @[[native-string<:app>, <meta>]] ~[<f>, <x>],
        <y>
    ]
f[]                   => @[[native-string<:app-list>, <meta>]] ~[<f>, ~]
f[x, y]               => @[[native-string<:app-list>, <meta>]] ~[<f>, [<x>, <y>]]
f{bytes}              => @[[native-string<:app-str>, <meta>]] ~[<f>, <{bytes}>]

;; labels and labeled application
foo: x                => @[[native-string<:label>, <meta>]] [native-string<foo>, <x>]
:stuff                => @[[native-string<:label>, <meta>]] [~, <stuff>]
f first: x second: y  => 
    @[[native-string<:app>, <meta>]] ~[
        @[[native-string<:app>, <meta>]] ~[
            <f>,
            @[[native-string<:label>, <meta>]] [native-string<first>, <x>]
        ],
        @[[native-string<:label>, <meta>]] [native-string<second>, <y>]
    ]
ptr deref:~           =>
    @[[native-string<:app>, <meta>]] ~[
        <ptr>,
        @[[native-string<:label>, <meta>]] [native-string<deref>, ~]
    ]

;; blocks and block application
do end                => @[[native-string<:block>, <meta>]] []
do x; y; end          => @[[native-string<:block>, <meta>]] [<x>, <y>]
f do x; y end         => 
    @[[native-string<:app-block>, <meta>]] ~[
        <f>,
        @[[native-string<:block>, <meta>]] [<x>, <y>]
    ]

;; operators
!x                    => @[[native-string<:prefix>, <meta>]] [native-string<!>, <x>]
x + y                 => @[[native-string<:infix>, <meta>]] [[native-string<+>], <x>, <y>]
x + y + z             => @[[native-string<:infix>, <meta>]] [[native-string<+>, native-string<+>], <x>, <y>, <z>]
x + y * z             => @[[native-string<:infix>, <meta>]] [[native-string<+>, native-string<*>], <x>, <y>, <z>]

;; closures, all the info in the native object
\[x] body             => @[[native-string<:fn>, <meta>]] native-closure<>
\[x, y] body          => @[[native-string<:fn>, <meta>]] native-closure<>
```

Note that infix operators are deliberately non-associative, the interpreter must
assign precedence and associativity themselves.

\newpage
