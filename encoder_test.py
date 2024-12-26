#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest

import parser  # pylint: disable=wrong-import-order,deprecated-module
import tokenizer
import encoder
from encoder import Node


class TestNodeEncoder(unittest.TestCase):

    @staticmethod
    def encode(text: str):
        tree = parser.Parser().parse(tokenizer.tokenize(text))
        staturated = parser.saturate(tree)
        striped = parser.strip(staturated)
        return encoder.encode_tree_nodes(striped)

    def test_node_set_tag(self):
        n = Node()
        n.set_tag(Node.APP)
        self.assertEqual(n.tag(), Node.APP)

    def test_node_set_lhs(self):
        n = Node()
        n.set_lhs(42)
        self.assertEqual(n.lhs(), 42)

    def test_node_set_rhs(self):
        n = Node()
        n.set_rhs(42)
        self.assertEqual(n.rhs(), 42)

    def test_node_set_both(self):
        n = Node()
        n.set_lhs(69)
        n.set_rhs(42)
        self.assertEqual(n.lhs(), 69)
        self.assertEqual(n.rhs(), 42)

    def test_node_set_all(self):
        n = Node()
        n.set_tag(Node.APP)
        n.set_lhs(69)
        n.set_rhs(42)
        self.assertEqual(n.tag(), Node.APP)
        self.assertEqual(n.lhs(), 69)
        self.assertEqual(n.rhs(), 42)

    def test_encoder_simplest(self):
        tree = TestNodeEncoder.encode('^')
        self.assertEqual(tree, (0, [Node(0, 0, 0)]))

    def test_encoder_simple(self):
        tree = TestNodeEncoder.encode('^ ^ ^')
        self.assertEqual(tree, (0, [
            Node(Node.TREE, 1, 2),
            Node(Node.TREE, Node.int_max, Node.int_max),
            Node(Node.TREE, Node.int_max, Node.int_max)
        ]))

    def test_encoder_simplest_redux_k(self):
        tree = TestNodeEncoder.encode('^ ^ ^ ^')
        self.assertEqual(tree, (0, [
            Node(Node.APP, 1, 4),
            Node(Node.TREE, 1, 2),
            Node(Node.TREE, Node.int_max, Node.int_max),
            Node(Node.TREE, Node.int_max, Node.int_max),
            Node(Node.TREE, Node.int_max, Node.int_max),
        ]))

    def test_encoder_simplest_redux_s(self):
        tree = TestNodeEncoder.encode('^ (^ ^) ^ ^')
        self.assertEqual(tree, (0, [
            Node(Node.APP, 1, 5),
            Node(Node.TREE, 1, 3),
            Node(Node.TREE, 1, 0),
            Node(Node.TREE, Node.int_max, Node.int_max),
            Node(Node.TREE, Node.int_max, Node.int_max),
            Node(Node.TREE, Node.int_max, Node.int_max),
        ]))


if __name__ == "__main__":
    unittest.main()
