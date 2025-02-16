# Tree-lang grammar

## Tokens

Comments: simplest line comments, start with `#` and contain a mandatory whitespace
character after.

```EBNF

t_Tree ::= "^"

t_Delimeter ::= "(" | ")" | "[" | "]" | "," | ";"

t_Intrinsic ::= "#{" t_Symbol | t_Number "}"

t_Number ::= 0 | [1-9][0-9]*

t_String ::= "{" any kind of character, except for unbalanced } or { "}"
            | "{{{" any kind of character, except for unbalanced }}} or {{{ "}}}"

t_Operator ::= # TODO

t_LambdaParameter ::= (`\` | `λ`) t_Symbol

t_Symbol ::= any other character

```

Reserved symbols:

  * `scope`
  * `module`
  * `use`
  * `in`
  * `do`
  * `end`
  * `=`

## Parsing


```EBNF

Source ::=
    Module* Expression

Module ::=
    t_Symbol('module') t_String t_Symbol('do') ScopeEntry* t_Symbol('end') 

ScopeEntry ::=
    ScopeBinding
    Module
    Use-statement

Expression ::=
    Scope
    Use-clause
    Abstraction
    Application
    Operand

Scope ::=
    t_Symbol('scope') t_String? t_Symbol('do') ScopeEntry* Expression t_Symbol('end')

ScopeBinding ::=
    t_Symbol t_Symbol('=') Expression t_Delimeter(';')

Use-statement ::=
    t_Symbol('use') t_String t_Symbol*

Use-clause ::=
    t_Symbol('use') t_String t_Symbol* t_Symbol('in') Expression
    
Operand ::=
    t_Tree
    t_Intrinsic
    t_Number
    t_String
    t_Symbol
    t_Delimeter('[') (Expression t_Delimeter(','))* t_Delimeter(']')
    t_Delimeter('(') Expression t_Delimeter(')')

Abstraction ::=
    t_LambdaParameter Expression

Application ::=
    t_Delimeter('(')? Expression Expression t_Delimeter(')')?
    # t_Delimeter('(') Expression t_Operator Expression t_Delimeter(')')

```
