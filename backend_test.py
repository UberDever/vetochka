#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest

import parser  # pylint: disable=wrong-import-order,deprecated-module
import tokenizer
import backend
from eval.eval import load_rt_lib
from backend import NodeLib


class TestNodeEncoder(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.rt_lib = load_rt_lib()
        cls.node_lib = NodeLib(cls.rt_lib)

    def encode(self, text: str):
        tree = parser.Parser().parse(tokenizer.tokenize(text))
        staturated = parser.saturate(tree)
        striped = parser.strip(staturated)
        return backend.encode_pure_tree(striped, self.rt_lib)

    def test_node_accessors(self):
        n = self.node_lib.new_tree()
        self.assertEqual(self.node_lib.tag(n), self.node_lib.tag_tree())
        n = self.node_lib.new_app(42, 69)
        self.assertEqual(self.node_lib.tag(n), self.node_lib.tag_app())
        self.assertEqual(self.node_lib.lhs(n), 42)
        self.assertEqual(self.node_lib.rhs(n), 69)
        n = self.node_lib.new_data(69420)
        self.assertEqual(self.node_lib.tag(n), self.node_lib.tag_data())
        self.assertEqual(self.node_lib.data(n), 69420)

    def test_encoder_simplest(self):
        tree = self.encode('^')
        self.assertEqual(tree, (0, [self.node_lib.new_tree()]))

    def test_encoder_simple(self):
        tree = self.encode('^ ^ ^')
        self.assertEqual(tree, (0, [
            self.node_lib.new_tree(1, 2),
            self.node_lib.new_tree(),
            self.node_lib.new_tree(),
        ]))

    def test_encoder_simplest_redux_k(self):
        tree = self.encode('^ ^ ^ ^')
        self.assertEqual(tree, (0, [
            self.node_lib.new_app(1, 4),
            self.node_lib.new_tree(1, 2),
            self.node_lib.new_tree(),
            self.node_lib.new_tree(),
            self.node_lib.new_tree(),
        ]))

    def test_encoder_simplest_redux_s(self):
        tree = self.encode('^ (^ ^) ^ ^')
        self.assertEqual(tree, (0, [
            self.node_lib.new_app(1, 5),
            self.node_lib.new_tree(1, 3),
            self.node_lib.new_tree(1, 0),
            self.node_lib.new_tree(),
            self.node_lib.new_tree(),
            self.node_lib.new_tree(),
        ]))


# class TestNumberAsListEncoding(unittest.TestCase):
#
#     @classmethod
#     def setUpClass(cls):
#         cls.eval_lib = load_eval_lib()
#         cls.node_lib = NodeLib(cls.eval_lib)
#
#     def encode(self, text: str):
#         tree = parser.Parser().parse(tokenizer.tokenize(text))
#         staturated = parser.saturate(tree)
#         striped = parser.strip(staturated)
#         root, nodes = backend.encode_pure_tree(striped, self.eval_lib)
#         return backend.decode_list_to_number(self.eval_lib, root, nodes)
#
#     def test_number_encoding_simple(self):
#         self.assertEqual(self.encode('^'), 0)
#         self.assertEqual(self.encode('^^'), 1)
#         self.assertEqual(self.encode('^(^^)'), 2)
#
#     def test_number_encoding_newline(self):
#         self.assertEqual(self.encode('^(^(^(^(^(^(^(^(^(^^)))))))))'),
#                          int.from_bytes(b'\n', byteorder='little'))
#
#
# class TestNumberAsTreeEncoding(unittest.TestCase):
#
#     @classmethod
#     def setUpClass(cls):
#         cls.eval_lib = load_eval_lib()
#         cls.node_lib = NodeLib(cls.eval_lib)
#
#     def encode(self, text: str):
#         tree = parser.Parser().parse(tokenizer.tokenize(text))
#         staturated = parser.saturate(tree)
#         striped = parser.strip(staturated)
#         root, nodes = backend.encode_pure_tree(striped, self.eval_lib)
#         return backend.decode_tree_to_number(self.eval_lib, root, nodes)
#
#     def test_number_encoding_simple(self):
#         self.assertEqual(self.encode('^'), 0)
#         self.assertEqual(self.encode('^^'), 1)
#         self.assertEqual(self.encode('^^^'), 2)
#         self.assertEqual(self.encode('^(^^^)'), 3)
#
#     def test_number_encoding_newline(self):
#         self.assertEqual(self.encode('^(^(^(^^^)^)^)(^^^)'),
#                          int.from_bytes(b'\n', byteorder='little'))

if __name__ == "__main__":
    unittest.main()
