#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest

import tokenizer as t
import parser  # pylint: disable=wrong-import-order,deprecated-module
# pylint: disable-next=wrong-import-order,deprecated-module
from parser import (strip, saturate, Scope, ScopeBinding, TreeNode,
                    ListExpression, Symbol, Application, String)


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

    def test_list_expression_with_application(self):
        tree = parser.Parser().parse(t.tokenize('[^ ^, ^ ^ ^]'))
        self.assertEqual(
            strip(tree),
            ListExpression(token=t.Delim(s='['),
                           children=[
                               Application(token=t.Tree(),
                                           children=[
                                               TreeNode(token=t.Tree(),
                                                        children=[]),
                                               TreeNode(token=t.Tree(),
                                                        children=[]),
                                           ]),
                               Application(token=t.Tree(),
                                           children=[
                                               Application(
                                                   token=t.Tree(),
                                                   children=[
                                                       TreeNode(token=t.Tree(),
                                                                children=[]),
                                                       TreeNode(token=t.Tree(),
                                                                children=[]),
                                                   ]),
                                               TreeNode(token=t.Tree(),
                                                        children=[]),
                                           ])
                           ]))

    def test_single_symbol(self):
        tree = parser.Parser().parse(t.tokenize('a'))
        self.assertEqual(strip(tree), Symbol(token=t.Symbol(s='a'),
                                             children=[]))

    def test_application_simplest(self):
        tree = parser.Parser().parse(t.tokenize('a b'))
        self.assertEqual(
            strip(tree),
            Application(token=t.Symbol(s='a'),
                        children=[
                            Symbol(token=t.Symbol(s='a'), children=[]),
                            Symbol(token=t.Symbol(s='b'), children=[])
                        ]))

    def test_application_multiple(self):
        tree = parser.Parser().parse(t.tokenize('a b c d'))
        self.assertEqual(
            strip(tree),
            Application(token=t.Symbol(s='a'),
                        children=[
                            Application(
                                token=t.Symbol(s='a'),
                                children=[
                                    Application(
                                        token=t.Symbol(s='a'),
                                        children=[
                                            Symbol(token=t.Symbol(s='a'),
                                                   children=[]),
                                            Symbol(token=t.Symbol(s='b'),
                                                   children=[])
                                        ]),
                                    Symbol(token=t.Symbol(s='c'), children=[])
                                ]),
                            Symbol(token=t.Symbol(s='d'), children=[])
                        ]))

    def test_application_parenthesized(self):
        tree = parser.Parser().parse(t.tokenize('(((a b)))'))
        self.assertEqual(
            strip(tree),
            Application(token=t.Symbol(s='a'),
                        children=[
                            Symbol(token=t.Symbol(s='a'), children=[]),
                            Symbol(token=t.Symbol(s='b'), children=[])
                        ]))

    def test_application_different_precedence(self):
        tree = parser.Parser().parse(t.tokenize('a b (c d)'))
        self.assertEqual(
            strip(tree),
            Application(token=t.Symbol(s='a'),
                        children=[
                            Application(token=t.Symbol(s='a'),
                                        children=[
                                            Symbol(token=t.Symbol(s='a'),
                                                   children=[]),
                                            Symbol(token=t.Symbol(s='b'),
                                                   children=[])
                                        ]),
                            Application(token=t.Symbol(s='c'),
                                        children=[
                                            Symbol(token=t.Symbol(s='c'),
                                                   children=[]),
                                            Symbol(token=t.Symbol(s='d'),
                                                   children=[])
                                        ])
                        ]))

    def test_application_different_stuff(self):
        tree = parser.Parser().parse(
            t.tokenize('[1, 2, 3] ^ (c d) {something}'))
        self.assertEqual(
            strip(tree),
            Application(
                token=t.Delim(s='['),
                children=[
                    Application(
                        token=t.Delim(s='['),
                        children=[
                            Application(
                                token=t.Delim(s='['),
                                children=[
                                    ListExpression(
                                        token=t.Delim(s='['),
                                        children=[
                                            Symbol(token=t.Symbol(s='1'),
                                                   children=[]),
                                            Symbol(token=t.Symbol(s='2'),
                                                   children=[]),
                                            Symbol(token=t.Symbol(s='3'),
                                                   children=[])
                                        ]),
                                    TreeNode(token=t.Tree(s='^'), children=[])
                                ]),
                            Application(token=t.Symbol(s='c'),
                                        children=[
                                            Symbol(token=t.Symbol(s='c'),
                                                   children=[]),
                                            Symbol(token=t.Symbol(s='d'),
                                                   children=[])
                                        ])
                        ]),
                    String(token=t.String(s='something'), children=[])
                ]))

    def test_scope_simplest(self):
        tree = parser.Parser().parse(t.tokenize('scope do ^ end'))
        self.assertEqual(
            strip(tree),
            Scope(token=t.Symbol(s='scope'),
                  children=[TreeNode(token=t.Tree(s='^'), children=[])],
                  name=None))

    def test_scope_simplest_application(self):
        tree = parser.Parser().parse(t.tokenize('scope do ^ ^ ^ end'))
        self.assertEqual(
            strip(tree),
            Scope(token=t.Symbol(s='scope'),
                  children=[
                      Application(token=t.Tree(s='^'),
                                  children=[
                                      Application(
                                          token=t.Tree(s='^'),
                                          children=[
                                              TreeNode(token=t.Tree(s='^'),
                                                       children=[]),
                                              TreeNode(token=t.Tree(s='^'),
                                                       children=[])
                                          ]),
                                      TreeNode(token=t.Tree(s='^'),
                                               children=[])
                                  ])
                  ],
                  name=None))

    def test_scope_application_nested(self):
        tree = parser.Parser().parse(
            t.tokenize('scope do ^ scope {Stuff} do ^ end end'))
        self.assertEqual(
            strip(tree),
            Scope(token=t.Symbol(s='scope'),
                  children=[
                      Application(token=t.Tree(s='^'),
                                  children=[
                                      TreeNode(token=t.Tree(s='^'),
                                               children=[]),
                                      Scope(token=t.Symbol(s='scope'),
                                            children=[
                                                TreeNode(token=t.Tree(s='^'),
                                                         children=[])
                                            ],
                                            name="Stuff")
                                  ])
                  ],
                  name=None))

    def test_scope_simple_binding(self):
        tree = parser.Parser().parse(t.tokenize('scope do a = ^; ^ end'))
        self.assertEqual(
            strip(tree),
            Scope(token=t.Symbol(s='scope'),
                  children=[
                      ScopeBinding(token=t.Symbol(s='a'),
                                   children=[
                                       Symbol(token=t.Symbol(s='a'),
                                              children=[]),
                                       TreeNode(token=t.Tree(s='^'),
                                                children=[])
                                   ]),
                      TreeNode(token=t.Tree(s='^'), children=[])
                  ],
                  name=None))

    def test_scope_everything_at_once(self):
        text = """
            scope do
                a = ^;
                b = ^^;
                c = a b;
                scope {Named} do
                    d = scope do ^^ end;
                    e = c;
                    e
                end
            end
        """
        tree = parser.Parser().parse(t.tokenize(text))
        self.assertEqual(
            strip(tree),
            Scope(token=t.Symbol(s='scope'),
                  children=[
                      ScopeBinding(token=t.Symbol(s='a'),
                                   children=[
                                       Symbol(token=t.Symbol(s='a'),
                                              children=[]),
                                       TreeNode(token=t.Tree(s='^'),
                                                children=[])
                                   ]),
                      ScopeBinding(token=t.Symbol(s='b'),
                                   children=[
                                       Symbol(token=t.Symbol(s='b'),
                                              children=[]),
                                       Application(
                                           token=t.Tree(s='^'),
                                           children=[
                                               TreeNode(token=t.Tree(s='^'),
                                                        children=[]),
                                               TreeNode(token=t.Tree(s='^'),
                                                        children=[])
                                           ])
                                   ]),
                      ScopeBinding(token=t.Symbol(s='c'),
                                   children=[
                                       Symbol(token=t.Symbol(s='c'),
                                              children=[]),
                                       Application(
                                           token=t.Symbol(s='a'),
                                           children=[
                                               Symbol(token=t.Symbol(s='a'),
                                                      children=[]),
                                               Symbol(token=t.Symbol(s='b'),
                                                      children=[])
                                           ])
                                   ]),
                      Scope(
                          token=t.Symbol(s='scope'),
                          children=[
                              ScopeBinding(
                                  token=t.Symbol(s='d'),
                                  children=[
                                      Symbol(token=t.Symbol(s='d'),
                                             children=[]),
                                      Scope(
                                          token=t.Symbol(s='scope'),
                                          children=[
                                              Application(
                                                  token=t.Tree(s='^'),
                                                  children=[
                                                      TreeNode(
                                                          token=t.Tree(s='^'),
                                                          children=[]),
                                                      TreeNode(
                                                          token=t.Tree(s='^'),
                                                          children=[])
                                                  ])
                                          ],
                                          name=None)
                                  ]),
                              ScopeBinding(token=t.Symbol(s='e'),
                                           children=[
                                               Symbol(token=t.Symbol(s='e'),
                                                      children=[]),
                                               Symbol(token=t.Symbol(s='c'),
                                                      children=[])
                                           ]),
                              Symbol(token=t.Symbol(s='e'), children=[])
                          ],
                          name='Named')
                  ],
                  name=None))


class TestTreeUtilities(unittest.TestCase):

    def test_saturation_k(self):
        tree = parser.Parser().parse(t.tokenize('^ ^ ^ ^'))
        self.assertEqual(
            strip(saturate(tree)),
            Application(token=t.Tree(s='^'),
                        children=[
                            TreeNode(token=t.Tree(s='^'),
                                     children=[
                                         TreeNode(token=t.Tree(s='^'),
                                                  children=[]),
                                         TreeNode(token=t.Tree(s='^'),
                                                  children=[])
                                     ]),
                            TreeNode(token=t.Tree(s='^'), children=[])
                        ]))

    def test_saturation_s(self):
        tree = parser.Parser().parse(t.tokenize('^ (^ ^) ^ ^'))
        self.assertEqual(
            strip(saturate(tree)),
            Application(token=t.Tree(s='^'),
                        children=[
                            TreeNode(token=t.Tree(s='^'),
                                     children=[
                                         TreeNode(token=t.Tree(s='^'),
                                                  children=[
                                                      TreeNode(
                                                          token=t.Tree(s='^'),
                                                          children=[])
                                                  ]),
                                         TreeNode(token=t.Tree(s='^'),
                                                  children=[])
                                     ]),
                            TreeNode(token=t.Tree(s='^'), children=[])
                        ]))

    def test_saturation_with_other_tokens(self):
        tree = parser.Parser().parse(t.tokenize('^ (^ w x) y z'))
        self.assertEqual(
            strip(saturate(tree)),
            Application(token=t.Tree(s='^'),
                        children=[
                            TreeNode(token=t.Tree(s='^'),
                                     children=[
                                         TreeNode(
                                             token=t.Tree(s='^'),
                                             children=[
                                                 Symbol(token=t.Symbol(s='w'),
                                                        children=[]),
                                                 Symbol(token=t.Symbol(s='x'),
                                                        children=[])
                                             ]),
                                         Symbol(token=t.Symbol(s='y'),
                                                children=[])
                                     ]),
                            Symbol(token=t.Symbol(s='z'), children=[])
                        ]))


if __name__ == "__main__":
    unittest.main()
