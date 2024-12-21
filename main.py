#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import argparse
import tokenizer
import parser  # pylint: disable=wrong-import-order,deprecated-module

# def encode(root: int) -> int:
#     pass


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-p',
                            '--path',
                            help='Path to file to translate',
                            required=True)
    args = arg_parser.parse_args()

    with open(args.path, 'r', encoding='utf-8') as file:
        text = file.read()
        tokens = tokenizer.tokenize(text)
        p = parser.Parser()
        tree = p.parse(tokens)
        print(tree)


if __name__ == "__main__":
    main()
