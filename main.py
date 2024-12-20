#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import argparse
from dataclasses import dataclass


@dataclass
class TreeToken:
    pass


@dataclass
class SymbolToken:
    s: str


@dataclass
class DelimToken:
    s: str


@dataclass
class StringToken:
    s: str


Token = TreeToken | DelimToken | StringToken | SymbolToken


def tokenize(byte_array: bytearray) -> [Token]:
    in_string = False
    tokens = []
    word = []
    block_level = -1

    for _, b in enumerate(byte_array):
        if not in_string:
            match b:
                case '^':
                    if word:
                        tokens.append(SymbolToken(''.join(word)))
                        word = []
                    tokens.append(TreeToken())
                case ')' | '(' | '[' | ']' | ',':
                    if word:
                        tokens.append(SymbolToken(''.join(word)))
                        word = []
                    tokens.append(DelimToken(b))
                case '{':
                    in_string = True
                    continue
                case '}':
                    assert False
                case _ if b.isspace():
                    if word:
                        tokens.append(SymbolToken(''.join(word)))
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
                        assert word
                        tokens.append(StringToken(''.join(word)))
                        word = []
                case _:
                    word.append(b)

    if in_string:
        assert not word

    if word:
        tokens.append(SymbolToken(''.join(word)))
        word = []

    if block_level != -1:
        raise RuntimeError(
            "Can't tokenize, encountered unbalanced expression along the way")
    return tokens


#
# def encode(tokens: [Token]) -> None:
#     pass
#


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p',
                        '--path',
                        help='Path to file to translate',
                        required=True)
    args = parser.parse_args()

    with open(args.path, 'r', encoding='utf-8') as file:
        text = file.read()
        print(tokenize(text))


if __name__ == "__main__":
    main()
