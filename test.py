#!/usr/bin/env python3
import unittest
import main as m
from main import StringToken, LParenToken, RParenToken, TreeToken


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
        self.assertEqual(tokens, [StringToken(s='result'), StringToken(s='='), LParenToken(), TreeToken(), TreeToken(), StringToken(
            s='println'), StringToken(s='"\'some\' "stuff""'), RParenToken(), StringToken(s='another ^({^({})}) string')])


if __name__ == "__main__":
    unittest.main()
