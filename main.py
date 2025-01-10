#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import pprint
from cmd import Cmd
import argparse

import tokenizer
import parser  # pylint: disable=wrong-import-order,deprecated-module
import encoder
from eval import eval  # pylint: disable=redefined-builtin


class REPL(Cmd):
    intro = ""
    prompt = "tree> "

    def do_exit(self, _):
        print("Exiting...")
        return True

    # pylint: disable-next=invalid-name
    def do_EOF(self, _):
        return self.do_exit(_)

    def do_echo(self, arg):
        print(arg)

    def default(self, line):
        eval_lib = eval.load_eval_lib()
        evaluator = eval.Evaluator(eval_lib)
        try:
            tokens = tokenizer.tokenize(line)
            p = parser.Parser()
            tree = p.parse(tokens)
            tree = parser.strip(parser.saturate(tree))
            encoded_root, encoded_nodes = encoder.encode_tree_nodes(
                tree, eval_lib)
            evaluator.set_tree(encoded_root, encoded_nodes)
            evaluator.evaluate()
            if err := evaluator.get_error():
                raise RuntimeError(err)
        except RuntimeError as e:
            print(e)


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-p', '--path', help='Path to file to translate')
    arg_parser.add_argument('--parse',
                            help='Only parse the provided file and print it')
    arg_parser.add_argument('--repl',
                            help='Enter repl mode',
                            action='store_true')
    args = arg_parser.parse_args()

    if args.repl:
        REPL().cmdloop()
        return
    assert args.path, "Please provide path to entry file"

    with open(args.path, 'r', encoding='utf-8') as file:
        text = file.read()
        tokens = tokenizer.tokenize(text)
        p = parser.Parser()
        tree = p.parse(tokens)
        tree = parser.strip(parser.saturate(tree))
        if args.parse:
            pprint.pprint(tree)
            return
        eval_lib = eval.load_eval_lib()
        encoded_root, encoded_nodes = encoder.encode_tree_nodes(tree, eval_lib)
        evaluator = eval.Evaluator(eval_lib)
        evaluator.set_tree(encoded_root, encoded_nodes)
        evaluator.evaluate()
        if err := evaluator.get_error():
            print(err)


if __name__ == "__main__":
    main()
