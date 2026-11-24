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

## Levels

During the development I found out that it would be very convenient to describe the language in
some "leveled" or "layered" sense. There are certain approaches to this that I know of (object algebras or nanopass style).
But for know I will use simplistic onthological view that won't provide strong formal guarantees, but will help me
to reason about the design more clearly.

Every consecutive language includes the previous one, if not stated otherwise.

### Vetochka 0

#### AST

```EBNF
Source ::= t_Tree | ( t_Tree t_Tree )
```

#### Bytecode

```asm
a:  NODE_TREE lhs rhs
b:  NODE_TREE lhs rhs
c:  NODE_APP $a $b
```

#### Evaluation

Three original rules from the `tree-calculus`.
Branch-first evaluation.

### Vetochka 1

Aka `Vetochka I or Vetochka with intrinsics`

#### AST

```EBNF
Operand ::= 
        | t_Tree
        | ListExpression
        | t_String
        | t_Number
        | t_Intrinsic
```

#### Bytecode

```asm
    NODE_DATA number
    NODE_EVAL number
```

#### Evaluation

The evalcall is assumed to be a form of

```
($ NODE_EVAL[1234] [arg1, arg2, arg3...])
```

This adds a new reduction rule, making it 4 in total

### Vetochka 2

Aka `Vetochka λ` or `Vetochka with lambdas`

#### AST

```EBNF
Abstraction ::=
    t_LambdaParameter Expression

Expression ::= ...
              | Abstraction
```

Bytecode and reduction is without changes.

### Vetochka 3

Aka `Vetochka M` or `Vetochka with module system`

#### AST

AST in its final form

#### Bytecode

No changes

#### Evaluation

No changes


