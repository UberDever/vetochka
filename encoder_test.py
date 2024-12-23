#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest

from encoder import Node


class TestNodeEncoder(unittest.TestCase):

    def test_node_set_tag(self):
        n = Node()
        n.set_tag(Node.FORK)
        self.assertEqual(n.tag(), Node.FORK)

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


if __name__ == "__main__":
    unittest.main()
