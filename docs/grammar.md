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

As such, this language is parsed into a single expression
with different tagged nodes (ir), that is later analyzed
(specialized ir) and lowered (VM+reducer cells).

Encoding is utf-8 text.

Comments: 
1. `;;` till the end of the line or eof.
2. Nestable: `#|` and `|#`
3. Structured: `#;` (comments out next expression in its entirety)

### Tokens

```ebnf
(* whitespace in hexadecimal notation *)
t_ws ::= 09 | 0A | 0B | 0C | 0D | 20

line_comment ::= ";;" <until newline or eof>

block_comment ::= "#|" <nestable contents> "|#"

structured_comment ::= "#;" trivia* expression

trivia ::= t_ws | line_comment | block_comment | structured_comment

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

t_label ::= t_identifier ":"

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
    t_label argument_expression

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

### Rewrite rules into a unstructured tree

```elixir
# Rules are recursive

t_literal => t_literal
t_identifier => [:id, t_identifier]

"[" comma_list? "]" => [comma_list...]

"(" expression ")" => [:group, expression]

block_list => [:block, expression1, ..., expressionN]

@[ann] expr => [:annot, ann, expr]

primary "." t_identifier =>
    ($ ($ "." primary) t_identifier)

primary "(" comma_list? ")" =>
    left_apply(primary, comma_list...)

primary "[" comma_list? "]" =>
    ($ primary [comma_list...])

primary t_string_literal =>
    ($ primary t_string_literal)

primary labeled_argument =>
    ($ primary [:label, t_label, argument_expression])

primary block_argument =>
    ($ primary [:block, expression1, ..., expressionN])

prefix_operator postfix_expression =>
    ($ [:prefix, prefix_operator] postfix_expression)

prefix_expression t_operator prefix_expression =>
    ($ ($ [:infix, t_operator] lhs) rhs)
```
