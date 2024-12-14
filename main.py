#!/usr/bin/env python3
import argparse
from dataclasses import dataclass


@dataclass
class TreeToken:
    pass


@dataclass
class LParenToken:
    pass


@dataclass
class RParenToken:
    pass


@dataclass
class StringToken:
    s: str


Token = TreeToken | LParenToken | RParenToken | StringToken


def tokenize(bytes: bytearray) -> [Token]:
    in_string = False
    tokens = []
    word = []
    block_level = -1

    for i, b in enumerate(bytes):
        if not in_string:
            match b:
                case '^': tokens.append(TreeToken())
                case ')': tokens.append(RParenToken())
                case '(': tokens.append(LParenToken())
                case '{': in_string = True; continue
                case '}': assert false
                case _ if b.isspace(): 
                    if word:
                        tokens.append(StringToken(''.join(word)))
                        word = []
                case _: word.append(b)
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
                        tokens.append(StringToken(''.join(word)))
                        word = []
                case _: word.append(b)

    if word:
        tokens.append(StringToken(''.join(word)))
        word = []

    if block_level != -1:
        raise RuntimeError("Can't tokenize, encountered unbalanced expression along the way")
    return tokens


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-p', '--path', help='Path to file to translate', required=True)
    args = parser.parse_args()

    with open(args.path, 'r') as file:
        bytes = file.read()
        print(tokenize(bytes))


if __name__ == "__main__":
    main()
