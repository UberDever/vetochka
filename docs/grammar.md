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
t_literal ::= t_string_literal | "^" | "Δ"

(*excluding "end"*)
t_identifier ::= [a-zA-Z_][a-zA-Z0-9_]*

(*excluding ":"*)
t_operator       ::= operator_char+
operator_char    ::= "=" | "+" | "-" | "*" | "/" | "%" | "<" | ">" | "!" | "&" | "|" | ":"

```

### Syntax

```ebnf
source ::= expression

expression ::= infix_expression

infix_expression ::=
    prefix_expression (t_operator prefix_expression)*

prefix_expression ::=
    prefix_operator* postfix_expression

postfix_expression ::=
    primary postfix*

postfix ::=
    "." t_identifier
  | "(" argument_list? ")"
  | "[" argument_list? "]"
  | t_string_literal
  | block_suffix
  | named_expr_suffix

block_suffix ::=
    block_section+ "end"

block_section ::=
    t_identifier ":" block_list

named_expr_suffix ::=
    "~" t_identifier expression ("," "~" t_identifier expression)* ","?

primary ::=
    t_literal
  | t_identifier
  | "[" argument_list? "]"
  | "(" expression ")"

prefix_operator ::= "!" | "-" | "~" | "*" | "&"

block_list ::=
    expression (";" expression)* ";"?

argument_list ::=
    expression ("," expression)* ","?
```
