#!/usr/bin/env python3
import unittest
import main as m
from main import StringToken, DelimToken, TreeToken, SymbolToken


class TestTokenizer(unittest.TestCase):
    def tokenize_file(path: str):
        with open(path, 'r') as file:
            bytes = file.read()
            return m.tokenize(bytes)

    def test_simplest(self):
        tokens = TestTokenizer.tokenize_file('tests/simplest.tree')
        self.assertEqual(tokens, [m.TreeToken()])

    def test_simple(self):
        tokens = TestTokenizer.tokenize_file('tests/simple.tree')
        self.assertEqual(tokens, [SymbolToken(s='result'), SymbolToken(s='='), DelimToken('('), TreeToken(), TreeToken(), SymbolToken(
            s='println'), StringToken(s='"\'some\' "stuff""'), DelimToken(')'), StringToken(s='another ^({^({})}) string')])

    def test_simplest_application(self):
        tokens = TestTokenizer.tokenize_file('tests/simplest-application.tree')
        self.assertEqual(tokens, [SymbolToken(s='a'), SymbolToken(s='b')])

    def test_all_tokens(self):
        tokens = TestTokenizer.tokenize_file('tests/all-tokens.tree')
        self.assertEqual(tokens, [SymbolToken(s='a'), SymbolToken(s='0'), DelimToken(s='('), DelimToken(s=')'), TreeToken(), DelimToken(s='['), DelimToken(
            s=','), SymbolToken(s='x'), DelimToken(s=','), SymbolToken(s='y'), DelimToken(s=']'), SymbolToken(s='z'), StringToken(s='plain text')])


if __name__ == "__main__":
    unittest.main()
