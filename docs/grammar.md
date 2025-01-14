# Tree-lang grammar

## Tokenization

The following token groups are there:

- Tree: designates the tree-calculus combinator, denoted as `Δ` or `^`
- Delimeter: any token from the list `()[],;`
- String (two cases): 
    * Anything enclosed in single curly braces `{...}`
    * Anything enclosed in group of sequential curly braces, group size is 3 `{{{...}}}`.
        Therefore, only data `}}}` is unrepresentable in the language -- fair enough, just use
        explicit concatenation then.
- TODO(don't need them for now) ~~Operator: any token from the list `+ - * /`~~
- Lambda parameters: Symbols starting with a `\` or a `λ`
- Symbol: any other token

Comments: simplest line comments, start with `#`

Reserved symbols:

  * `scope`
  * `module`
  * `use`
  * `in`
  * `do`
  * `end`
  * `=`

Note that this simple tokenization requires further parsing and encoding.

## Language

The following is the EBNF of the language:

```
Source ::=
    Module* Expression

Module ::=
    Symbol('module') String Symbol('do') ScopeEntry* Symbol('end') 

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
    Symbol('scope') String? Symbol('do') ScopeEntry* Expression Symbol('end')

ScopeBinding ::=
    Symbol Symbol('=') Expression Delimeter(';')

Use-statement ::=
    Symbol('use') String

Use-clause ::=
    Symbol('use') String Symbol('in') Expression
    
Operand ::=
    Tree
    String
    Symbol
    ListExpression
    Delimeter('(') Expression Delimeter(')')

Abstraction ::=
    LambdaParameter Expression

Application ::=
    Delimeter('(')? Expression Expression Delimeter(')')?
    # InfixExpression

# InfixExpression ::=
#     Delimeter('(') Expression Operator Expression Delimeter(')')

ListExpression ::=
    Delimeter('[') (Expression Delimeter(','))* Delimeter(']')

```
