#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest

import tokenizer as t
import parser  # pylint: disable=wrong-import-order,deprecated-module
# pylint: disable-next=wrong-import-order,deprecated-module
from parser import Node, TreeNode, ListExpression


def strip(root: Node) -> Node:
    "Strips tree from dormant nodes (unary and without token)"

    def aux(n: Node) -> Node:
        if n.token is None and len(n.children) == 1:
            return aux(n.children[0])
        return type(n)(n.token, [aux(c) for c in n.children])

    return aux(root)


class TestParser(unittest.TestCase):

    @staticmethod
    def parse_file(path: str):
        with open(path, 'r', encoding='utf-8') as file:
            b = file.read()
            tokens = t.tokenize(b)
            p = parser.Parser()
            return p.parse(tokens)

    def test_empty(self):
        p = parser.Parser()
        tree = p.parse([])
        self.assertEqual(tree, None)

    def test_simplest(self):
        tree = parser.Parser().parse(t.tokenize('^'))
        self.assertEqual(strip(tree), TreeNode(token=t.Tree(), children=[]))

    def test_list_expression_empty(self):
        tree = parser.Parser().parse(t.tokenize('[]'))
        self.assertEqual(strip(tree),
                         ListExpression(token=t.Delim(s='['), children=[]))

    def test_list_expression_single(self):
        tree = parser.Parser().parse(t.tokenize('[^]'))
        self.assertEqual(
            strip(tree),
            ListExpression(token=t.Delim(s='['),
                           children=[TreeNode(token=t.Tree(), children=[])]))

    def test_list_expression_single_trailing(self):
        tree = parser.Parser().parse(t.tokenize('[^,]'))
        self.assertEqual(
            strip(tree),
            ListExpression(token=t.Delim(s='['),
                           children=[TreeNode(token=t.Tree(), children=[])]))

    def test_list_expression_many(self):
        tree = parser.Parser().parse(t.tokenize('[^,^,^]'))
        self.assertEqual(
            strip(tree),
            ListExpression(token=t.Delim(s='['),
                           children=[
                               TreeNode(token=t.Tree(), children=[]),
                               TreeNode(token=t.Tree(), children=[]),
                               TreeNode(token=t.Tree(), children=[])
                           ]))

    def test_list_expression_many_trailing(self):
        tree = parser.Parser().parse(t.tokenize('[^,^,^,]'))
        self.assertEqual(
            strip(tree),
            ListExpression(token=t.Delim(s='['),
                           children=[
                               TreeNode(token=t.Tree(), children=[]),
                               TreeNode(token=t.Tree(), children=[]),
                               TreeNode(token=t.Tree(), children=[])
                           ]))


if __name__ == "__main__":
    unittest.main()
