#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import pprint

import argparse
import tokenizer
import parser  # pylint: disable=wrong-import-order,deprecated-module
import encoder
from eval import eval  # pylint: disable=redefined-builtin


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-p',
                            '--path',
                            help='Path to file to translate',
                            required=True)
    arg_parser.add_argument('--parse',
                            help='Only parse the provided file and print it')
    args = arg_parser.parse_args()

    with open(args.path, 'r', encoding='utf-8') as file:
        text = file.read()
        tokens = tokenizer.tokenize(text)
        p = parser.Parser()
        tree = p.parse(tokens)
        if args.parse:
            rich = parser.saturate(tree)
            pprint.pprint(parser.strip(rich))
            return
        encoded_root, encoded_nodes = encoder.encode_tree_nodes(tree)
        evaluator = eval.Evaluator()
        evaluator.set_tree(encoded_root, [node.repr for node in encoded_nodes])
        evaluator.evaluate()
        if err := evaluator.get_error():
            print(err)


if __name__ == "__main__":
    main()
