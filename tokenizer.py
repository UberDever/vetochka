# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

from dataclasses import dataclass


@dataclass
class Tree:
    s: str = '^'


@dataclass
class Symbol:
    s: str


@dataclass
class Delim:
    s: str


@dataclass
class String:
    s: str


@dataclass
class Delimeters:
    rsquare = Delim('[')
    lsquare = Delim(']')
    rparen = Delim('(')
    lparen = Delim(')')
    rcurly = Delim('{')
    lcurly = Delim('}')
    comma = Delim(',')


Token = Tree | Delim | String | Symbol


# pylint: disable-next=too-many-branches
def tokenize(byte_array: bytearray) -> list[Token] | str:
    in_string = False
    tokens = []
    word = []
    block_level = -1

    for _, b in enumerate(byte_array):
        if not in_string:
            match b:
                case '^':
                    if word:
                        tokens.append(Symbol(''.join(word)))
                        word = []
                    tokens.append(Tree())
                case ')' | '(' | '[' | ']' | ',':
                    if word:
                        tokens.append(Symbol(''.join(word)))
                        word = []
                    tokens.append(Delim(b))
                case '{':
                    in_string = True
                    continue
                case '}':
                    return ("Encountered closing extra closing curly, "
                            "check if curlies are balanced")
                case _ if b.isspace():
                    if word:
                        tokens.append(Symbol(''.join(word)))
                        word = []
                case _:
                    word.append(b)
        else:
            match b:
                case '{':
                    block_level += 1
                    word.append(b)
                case '}':
                    if block_level >= 0:
                        block_level -= 1
                        word.append(b)
                    else:
                        in_string = False
                        if not word:
                            return ("Encountered closing curly too soon, "
                                    "check if curlies are balanced")
                        tokens.append(String(''.join(word)))
                        word = []
                case _:
                    word.append(b)

    if in_string:
        if word:
            return ("Tokenization ended with unclosed string literal, "
                    "check if curlies are balanced")

    if word:
        tokens.append(Symbol(''.join(word)))
        word = []

    if block_level != -1:
        raise RuntimeError(
            "Can't tokenize, encountered unbalanced expression along the way")
    return tokens
