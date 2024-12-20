#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest
import tokenizer
from tokenizer import String, Delim, Tree, Symbol


class TestTokenizer(unittest.TestCase):

    @staticmethod
    def tokenize_file(path: str):
        with open(path, 'r', encoding='utf-8') as file:
            b = file.read()
            return tokenizer.tokenize(b)

    def test_simplest(self):
        tokens = TestTokenizer.tokenize_file('tests/simplest.tree')
        self.assertEqual(tokens, [Tree()])

    def test_simple(self):
        tokens = TestTokenizer.tokenize_file('tests/simple.tree')
        self.assertEqual(tokens, [
            Symbol(s='result'),
            Symbol(s='='),
            Delim('('),
            Tree(),
            Tree(),
            Symbol(s='println'),
            String(s='"\'some\' "stuff""'),
            Delim(')'),
            String(s='another ^({^({})}) string')
        ])

    def test_simplest_application(self):
        tokens = TestTokenizer.tokenize_file('tests/simplest-application.tree')
        self.assertEqual(tokens, [Symbol(s='a'), Symbol(s='b')])

    def test_all_tokens(self):
        tokens = TestTokenizer.tokenize_file('tests/all-tokens.tree')
        self.assertEqual(tokens, [
            Symbol(s='a'),
            Symbol(s='0'),
            Symbol(s='1234'),
            Symbol(s='1234.567'),
            Delim(s='('),
            Symbol(s='s'),
            Symbol(s='expr'),
            Delim(s=')'),
            Delim(s='('),
            Delim(s=')'),
            Tree(),
            Delim(s='['),
            Symbol(s='x'),
            Delim(s=','),
            Symbol(s='y'),
            Delim(s=','),
            Symbol(s='z'),
            Delim(s=']'),
            String(s='plain text')
        ])


if __name__ == "__main__":
    unittest.main()
