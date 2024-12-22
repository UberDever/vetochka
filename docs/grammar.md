# Tree-lang grammar

## Tokenization

The following token groups are there:
- Tree: designates the tree-calculus combinator, denoted as `Δ` or `^`
- Delimeter: any token from the list `()[],`
- String: anything enclosed in outermost curly braces `{}`
- TODO(don't need them for now) ~~Operator: any token from the list `+ - * /`~~
- Symbol: any other token

Note that this simple tokenization requires further parsing and encoding.

## Language

The following is the EBNF of the language:

```
Source ::=
    Expression

Expression ::=
    Application
    # InfixExpression
    ListExpression
    Operand
    
Operand ::=
    Tree
    String
    Symbol
    Delimeter('(') Expression Delimeter(')')
    
Application ::=
    Delimeter('(')? Expression Expression Delimeter(')')?

# InfixExpression ::=
#     Delimeter('(') Expression Operator Expression Delimeter(')')

ListExpression ::=
    Delimeter('[') (Expression Delimeter(','))* Delimeter(']')

```
