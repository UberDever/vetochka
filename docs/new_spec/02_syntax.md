# The core

This section defines the fundamental syntax used to represent every feature
of `vetochka`.

At its core, the language consists of tagged trees. Programs, which are also trees,
interpret, pass, and inspect these trees.
All trees are built from nyads.
A nyad carries no information by itself, much like the type-theoretical `unit`.
However, they are useful together: nyads can form any homogeneous binary tree.
This way, we encode a series of useful features, which build the backbone
of the language. Furthermore, since a programmer can write trees that build other
trees, she can encode her own semantics, at her heart's content.

We call a nyad with no children a `leaf`.
A nyad with one child is a `stem`, and a nyad with two children is a `fork`.
A nyad can't hold more than two children.

Here are some examples:
```
~           ;; a leaf
~[~]        ;; a stem
~[~[~], ~]  ;; a fork with one stem and a leaf
```

Proper lists are fundamental enough to have their own shorthand:

```vf
[]     == ~
[x]    == ~[x, ~]
[x, y] == ~[x, ~[y, ~]]
...
```

We call this subset of syntax the core syntax.

Below are examples of nyads and structures that this syntax can express.

```vf
~; ;; a leaf
~[~, ~]; ;; a fork
~[~, ~[~, ~]] ;; a proper list of two leaves, terminated by a leaf nyad
~[~, ~[~, ~[~, ~[~, ~]]]] ;; a number 4 using Peano-style encoding
[[], [], [], []] ;; the same number, but using proper list notation explicitly
;; an application encoding, with stubs instead of myriad of nodes
;; note, however, that these stubs are also just nyads all the way down
~[
    [<app-tag>, ...],
    ~[
        ~[
            [<func-tag>, ...],
            [
                ~[[<identifier-tag>, ...], <id-x>], ;; binder
                ~[[<body-tag>, ...], ~[[<identifier-tag>, ...], <id-x>]], ;; body
            ]
        ], ;; function
        ~, ;; argument
    ]
]
```

# General syntax

## Motivation and examples

This syntax provides a common notation for different `vetochka` branches (called `vf`).
It mainly consists of shorthands and syntax sugar that simplify writing programs
and express the programmer's intent more clearly.
That is, an integer or a key-value pair is clearer than its corresponding tree structure
both for the viewer and for the program that inspects that.

Most of this syntax is deliberately **not** executable. Some native operations
and objects have defined semantics (see [v0](TODO:)), but overall, other constructs are just sugar, and their meaning is assigned by the interpreter.

Examples:

```vf
;; literals
1234
{outer {inner} text}        ;; balanced braces inside a string
{веточка}                   ;; UTF-8 string content

;; different forms in one list; `empty?` and `x-y'` are also identifiers
[1234, {text}, stuff1234, empty?, x-y']

;; some applications
(f + g)(x)                  ;; an application of a grouped expression
f[x, y,]                    ;; list application
f{bytes}                    ;; string application
(~)[x]                      ;; list application to a leaf, since ~[x] is a stem
~[name: x, do y end,]       ;; mind you, nyad children could be anything

;; on operators
x :: y                      ;; an infix form
x . y                       ;; dot is an ordinary infix operator
::x                         ;; a prefix form
x#|left|#+#|right|#y        ;; infix operators require whitespace, comments suffice too
-123                        ;; this is a prefix application, not an integer literal with a sign
x + -y                      ;; an infix operator followed by a prefix expression

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

;; closures, your general lambda calculus (with builtin sequencing)
\[x] do x end
\[a, b, c] do (a(c))(b(c)) end
\[x] do \[y] do x(y) end end

;; bindings
=[a: 12, b: f()] do use(a, b) end
=[x: f32{1.2}, y: x + f32{1.5}] do plot(data, x, y) end

;; triage
?(tree, lhs, rhs)           ;; bind lhs and rhs to (maybe present) children
do ~ end                    ;; when leaf
do lhs end                  ;; when stem
do rhs end                  ;; when fork

;; tags can be arbitrary terms; these examples use lists
@[{note}] x
@[[a: b, c: d]] foo              ;; a list of labels as the tag
@[[]] foo                        ;; an empty list as the tag
@[dangerous] 42 / 0              ;; tag an expression
@[[f(x)]] y                      ;; a tag list containing an application tree
(\[foo] @(foo))(@[[{stuff}]] 42) ;; use a destructuring syntax

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
We also exclude `...` because it is the line continuation marker.

Identifiers and operator runs consume the longest matching sequence of characters.
Thus, `x-y` is one identifier, and `!!x` starts with one operator run, `!!`.

`balanced_utf8_bytes` permits balanced braces without escape syntax. The exact
scanner algorithm is an implementation detail.

## Automatic semicolon insertion

At source level and inside `do ... end` blocks, a newline acts as `;` once
the current expression is complete. If the expression still requires more
syntax, the newline is whitespace. An example could be an expression
that itself includes `do ... end`, like a closure or a triage.

Inside parentheses or brackets, newlines are whitespace. Generally,
the innermost enclosing expression determines how exactly newlines behave.

`...` keeps the following newline as whitespace. Use it to continue a complete
expression onto the next line. Otherwise, the newline ends that expression even
if the next line could extend it.

## Grammar

The `NO_WS` marker requires tokens to touch, with no trivia (generally a whitespace) between them.
Conversely, `WS` requires at least one trivia item.
These markers help to distinguish infix from prefix, or different application constructs from standalone values.

```ebnf
source ::= block_list? eof
block_list ::= expression (";" expression)* ";"?
comma_list ::= expression ("," expression)* ","?

expression ::= labeled_expression | block | tagged_expression

block ::= "do" block_list? "end"

labeled_expression ::= (label_word NO_WS)? ":" argument_expression
label_word ::= identifier | "do" | "end"

tagged_expression ::= tag? (closure | infix<postfix_expression>)
tag ::= "@[" expression "]"

argument_expression ::= tag? (closure_tight | infix<postfix_expression_tight>)

closure ::= closure_head expression
closure_tight ::= closure_head (block | argument_expression)
closure_head ::= "\" NO_WS "[" identifier ("," identifier)* ","? "]"

binding ::= "=" NO_WS "[" binding_list? "]" WS block
binding_list ::= binding_clause ("," binding_clause)* ","?
binding_clause ::= identifier NO_WS ":" argument_expression

triage ::= "?" NO_WS "(" expression "," identifier "," identifier ","? ")"
               WS block WS block WS block

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
          | binding
          | triage
          | "[" comma_list? "]"
          | "(" expression ")"

nyad ::= "~" | "~" NO_WS "[" expression ( "," expression )? ","? "]"
untag ::= "@(" tagged_expression ")"
```

Some clarifications:

We use parameterized EBNF to simplify the rules while distinguishing
productions that associate tightly from those that associate loosely.

A `~` immediately followed by `[` starts a stem or fork, never a leaf.

`f()` is equivalent to `f(~)`.

Tight postfixes associate left to right. Each attaches to the accumulated expression:
`f(x)[y]{z}` groups as `((f(x))[y]){z}`.

`-x` is a prefix expression, and `x - y` is an infix expression.
`- x`, `x -y`, `1-2`, and `f(x)-y` are syntax errors.
Infix operators require trivia on both sides.

A labeled expression consumes the longest
`argument_expression` as its payload. Nesting requires parens: `x: (y: 1)`.

## Lowering

General syntax allows you to easily build programs, but to simplify their
inspection you probably would need a set of straightforward rules that
describe what exact shape every syntax-expressed tree takes. This is what
this lowering is for. The section describes only
syntax-level transformations. There are also [piths](TODO:), which allow
you to inspect and build runtime-constructed objects beyond syntax. These
primarily address native-side objects, like closures or numbers.

Note that these lowered terms aren't necessarily stored as-is during runtime. But
from the standpoint of the language, you could be sure that these would behave exactly as
their core syntax form does.

### Lowering notation

To describe lowering, we use the core syntax, with certain additions:

- `<term>` for a source-term placeholder on the left and its recursively lowered form on the right
- a set of markers `native-*<...>` for machine-native values
- proper lists instead of raw nyads where applicable

`native-opcode<bind>` and `native-triage<>` denote the binding and triage
opcodes. Their applications carry the source terms as data. Their execution
contracts belong to v0.

In the binding schema, `<bindings>` denotes the clause sequence on the left
and its recursively lowered proper list on the right. `<body>` denotes the body block.

Child terms retain their own tags and source metadata. Native markers denote raw
machine-native values; they do not implicitly add tags.

The placeholder `<meta>` describes source metadata included in the term's tag
during parsing. Since you would work with code directly, you probably would
be interested in the source information, hence meta at least includes this:

- `line`: an `i64` giving the source line. A value of `0` can indicate a synthetic term.
- `col`: the original source column. It follows the same convention as `line`.

### On tagging

TODO: need a full chapter for tags, with runtime and stuff, prob in v0

Every meaningful tree, except a purely structural one such as a list,
is expected to be tagged. Tags form a communication protocol:
they tell interpreters and inspection programs what a tree represents.

The tag in `@[tag] value` can be any term. The examples below use list tags.

The main feature of tagging is that it doesn't affect the application
of terms. The following guarantees hold:

```
@[m] v      == ~[m, v]
@(@[m] v)   => [m, v]
(@[m] v)(x) => v(x)
```

Here, `m` is an arbitrary tag term, and `v` is the payload.

### Lowering rules

```vf
1234                  => @[[native-string<:i64>, <meta>]] native-integer<1234>
{bytes}               => @[[native-string<:str>, <meta>]] native-string<bytes>
x                     => @[[native-string<:id>, <meta>]] native-id<x>

~                     => ~
~[<x>]                => ~[<x>]
~[<x>, <y>]           => ~[<x>, <y>]

[]                    => ~
[<x>, <y>, <z>]       => ~[<x>, ~[<y>, ~[<z>, ~]]]

;; grouping parentheses disappear during lowering

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

;; binding: a clause list and body are operands of the binding opcode
=[<bindings>] <body> =>
    @[[native-string<:app>, <meta>]] ~[
        @[[native-string<:binding>, <meta>]] native-binding<>,
        [<bindings>, <body>]
    ]

;; triage: subject, binders, then leaf, stem, and fork blocks
?(term, lhs, rhs) do leaf end do stem end do fork end =>
    @[[native-string<:app>, <meta>]] ~[
        @[[native-string<:triage>, <meta>]] native-triage<>,
        [<term>, <lhs>, <rhs>, <do leaf end>, <do stem end>, <do fork end>]
    ]
```

The syntax deliberately assigns no precedence or associativity to infix operators.
Some (probably yours) interpreter must assign them.

\newpage
