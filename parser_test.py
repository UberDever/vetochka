#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest

import tokenizer as t
import parser
from parser import TreeNode, Source, Expression, ListExpression


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
        self.assertEqual(
            tree,
            Source(token=None,
                   children=[
                       Expression(
                           token=None,
                           children=[TreeNode(token=t.Tree(), children=[])])
                   ]))

    def test_list_expression(self):
        tree = parser.Parser().parse(t.tokenize('[^,^,^]'))
        self.assertEqual(
            tree,
            Source(token=None,
                   children=[
                       Expression(
                           token=None,
                           children=[
                               ListExpression(
                                   token=None,
                                   children=[
                                       Expression(token=None,
                                                  children=[
                                                      TreeNode(token=t.Tree(),
                                                               children=[])
                                                  ]),
                                       Expression(token=None,
                                                  children=[
                                                      TreeNode(token=t.Tree(),
                                                               children=[])
                                                  ]),
                                       Expression(token=None,
                                                  children=[
                                                      TreeNode(token=t.Tree(),
                                                               children=[])
                                                  ])
                                   ])
                           ])
                   ]))


if __name__ == "__main__":
    unittest.main()
