#!/usr/bin/env -S PYTHONUNBUFFERED=1 python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import pprint
from cmd import Cmd, IDENTCHARS
import argparse

import tokenizer
import parser  # pylint: disable=wrong-import-order,deprecated-module
import backend
from eval import eval  # pylint: disable=redefined-builtin


class REPL(Cmd):
    intro = ""
    prompt = "tree> "
    identchars = IDENTCHARS + ":"

    def __init__(self):
        super().__init__()
        self.rt_lib = eval.load_rt_lib()

    def exit_impl(self, _):
        "Exit the repl"
        print("Exiting...")
        return True

    # pylint: disable-next=invalid-name
    def do_EOF(self, _):
        "Exit with EOF"
        return self.exit_impl(_)

    def default(self, line):
        raise RuntimeError("this should be updated after the refactoring")
        evaluator = eval.Evaluator(self.rt_lib)
        try:
            tokens = tokenizer.tokenize(line)
            p = parser.Parser()
            tree = p.parse(tokens)
            tree = parser.strip(parser.saturate(tree))
            encoded_root, encoded_nodes = backend.encode_pure_tree(
                tree, self.rt_lib)
            evaluator.set_tree(encoded_root, encoded_nodes)
            evaluator.evaluate()
            if err := evaluator.get_error():
                raise RuntimeError(err)

            root = evaluator.state.root
            nodes = evaluator.state.nodes
            # root = encoded_root
            # nodes = encoded_nodes
            # print(
            #     str(backend.decode_tree_to_number(self.eval_lib, root, nodes)))
            # print(
            #     backend.decode_list_of_numbers_to_string(
            #         self.eval_lib, root, nodes).encode('utf-8'))
            print(backend.dump_tree(self.rt_lib, root, nodes))

        except RuntimeError as e:
            print(e)


def augment_repl(repl: REPL, builtin_commands: list):
    for cmd_name, cmd in builtin_commands:
        name = "do_" + cmd_name
        setattr(repl, name, cmd)


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-p', '--path', help='Path to file to translate')
    arg_parser.add_argument('--parse',
                            help='Only parse the provided file and print it',
                            action='store_true')
    arg_parser.add_argument('--repl',
                            help='Enter repl mode',
                            action='store_true')
    args = arg_parser.parse_args()

    if args.repl:
        repl = REPL()
        augment_repl(REPL, [(":exit", repl.exit_impl)])
        repl.cmdloop()
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
        rt_lib = eval.load_rt_lib()
        encoded_root, encoded_nodes = backend.encode_pure_tree(tree, rt_lib)
        with eval.Evaluator(rt_lib, encoded_root, encoded_nodes) as evaluator:
            evaluator.evaluate()
            if err := evaluator.get_error():
                print(err)


if __name__ == "__main__":
    main()
