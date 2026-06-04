⬜ ✅ ❌

## Base language

This language is also a canonical bytecode representation in utf-8 text.

Comments: 
1. `;;` till the end of the line or eof.
2. ⬜ structural comments

### Tokens

```ebnf

(* whitespace in hexadecimal notation *)
t_ws ::= 09 | 0A | 0B | 0C | 0D | 20

(* 
    t_literal_chars can include any utf-8 character;
    It also can include any number of "}" as long as they are balanced;
    With conjunction with identifiers, they can encode different literals like
    i64{1234} or f32{-5.4e9}
*)
t_string_literal ::= "{" t_literal_chars "}"
(*integer literal is always i64*)
t_integer_literal ::= [1-9][0-9]* | [0-9]
t_literal ::= t_string_literal | t_integer_literal | "^" | "Δ"

TODO: properly extend this and extract "identifier label, identifier marker"
(*excluding "do", "end"*)
t_identifier ::= [a-zA-Z_][a-zA-Z0-9_=+-*/%<>!&|:]* |
                 ":"[a-zA-Z_][a-zA-Z0-9_=+-*/%<>!&|]*

(*operators must be separated by whitespace, in other case they
are considered as part of identifier*)
t_operator       ::= operator_char+
operator_char    ::= "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">" | "!" | "&" | "|" | ":"

```

### Syntax

```ebnf
source ::= expression

expression ::= annotation? infix_expression

annotation ::= "@" expression ";"

(*
    mixed infix is rejected by the parser, i.e.
    x + y + z   => fine
    x + y * z   => error, do explicit
    x + (y * z) => fine
*)
infix_expression ::=
    prefix_expression (t_operator prefix_expression)*

prefix_expression ::=
    prefix_operator* postfix_expression

postfix_expression ::=
    primary postfix*

postfix ::=
    "." t_identifier
  | "(" comma_list ")"
  | "[" comma_list? "]"
  | t_string_literal
  | block_argument
  | named_argument

block_argument ::=
    "do" block_list? "end"

named_argument ::=
    t_identifier <nows> ":" expression

primary ::=
    t_literal
  | t_identifier
  | "[" comma_list? "]"
  | "(" expression ")"

prefix_operator ::= "!" | "-" | "~" | "*" | "&"

block_list ::=
    expression (";" expression)* ";"

comma_list ::=
    expression ("," expression)* ","?
```

The main advantage of this grammar is that it allows for relatively readable common
imperative code while being fully homoiconic and unbound in terms of keywords. The only keywords
are (`end` and `do`) and only other reserved syntax objects are operators and literals, the rest is
just structure, as triage-calculus bequeathed.

Following "rules" describe the main idea of a parser rewrite: in the end, tree must contain
only literals, identifiers+operators, lists (encoded via `^`) and applications. It
must also contain tagged expressions/values/other stuff to provide enough information
after desugaring, i.e. infix/prefix tag or groupping tag, since all parsing is left-associative and we lose distinction between intentional/unintentional grouping.

```elixir
t_literal => t_literal
t_identifier => t_identifier
"[" comma_list? "]" => [comma_list...]
"(" expression ")" => [:group, expression]
block_list => [expression1, ..., expressionN]
primary "." t_identifier => ($($ "." primary) t_identifier)
primary "(" comma_list? ")" => ($...($($ primary comma_list[0]) comma_list[1])...)
primary "[" comma_list? "]" => ($ primary [comma_list...])
primary t_string_literal => ($ primary t_string_literal)
primary named_argument => ($ primary [t_identifier:, expression])
primary block_argument => ($ primary [expression1, ..., expressionN])
prefix_operator postfix_expression => ($ [:prefix, prefix_operator] postfix_expression)
prefix_expression t_operator prefix_expression => ($($ [:infix, t_operator] prefix_expression) prefix_expression)
```
