#!/usr/bin/env python3

# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

import unittest
import tokenizer as t
from tokenizer import String, Delim, Tree, Symbol


class TestTokenizer(unittest.TestCase):

    @staticmethod
    def tokenize_file(path: str):
        with open(path, 'r', encoding='utf-8') as file:
            b = file.read()
            return t.tokenize(b)

    def test_simplest(self):
        tokens = t.tokenize('^')
        self.assertEqual(tokens, [Tree()])

    def test_simplest_string(self):
        tokens = t.tokenize('{}')
        self.assertEqual(tokens, [String('')])

    def test_all_kinds_of_stuff(self):
        tokens = t.tokenize("""result = (^^println {"'some' "stuff""})
            {another ^({^({})}) string}""")
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
        tokens = t.tokenize('a b')
        self.assertEqual(tokens, [Symbol(s='a'), Symbol(s='b')])

    def test_all_tokens(self):
        tokens = t.tokenize(
            'a 0 1234 1234.567 (s expr) ( ) ^ [x, y, z] {plain text}')
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

    def test_very_bad_string1(self):
        self.assertRaises(RuntimeError, lambda: t.tokenize('{{}'))

    def test_very_bad_string2(self):
        self.assertRaises(RuntimeError, lambda: t.tokenize('{}}'))

    def test_very_bad_string3(self):
        self.assertRaises(RuntimeError, lambda: t.tokenize('{ }}'))


if __name__ == "__main__":
    unittest.main()
