# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

from dataclasses import dataclass
import logging

import tokenizer


@dataclass
class Node:
    token: tokenizer.Token | None
    children: ['Node']


@dataclass
class Source(Node):
    pass


@dataclass
class Expression(Node):
    pass


@dataclass
class ListExpression(Node):
    pass


@dataclass
class TreeNode(Node):
    pass


class Parser:
    tokens: [tokenizer.Token]
    cur_token = 0
    at_eof = False
    was_error = False

    def cur(self):
        return self.tokens[self.cur_token]

    def next(self) -> None:
        while True:
            self.cur_token += 1
            if self.cur_token >= len(self.tokens):
                self.at_eof = True
            return

    def match_token(self, token: tokenizer.Token, match_contents=True) -> bool:
        if self.at_eof:
            return False

        t = self.tokens[self.cur_token]
        if match_contents:
            return t == token

        return type(t) is type(token)

    def expect(self, token: tokenizer.Token, match_contents=True) -> bool:
        if self.at_eof:
            return False

        ok = self.match_token(token, match_contents)
        if not ok:
            logging.error("[Parser] Unexpected '%s' expected '%s'", self.cur(),
                          token)
            self.at_eof = True
            self.was_error = True
            return False

        self.next()
        return True

    def parse_source(self) -> Node:
        nodes = []
        nodes.append(self.parse_expression())
        return Source(None, nodes)

    def parse_expression(self) -> Node:
        nodes = []
        if self.match_token(tokenizer.Tree()):
            nodes.append(self.parse_tree())
        elif self.match_token(tokenizer.Delim('[')):
            nodes.append(self.parse_list_expression())

        return Expression(None, nodes)

    def parse_list_expression(self) -> Node:
        nodes = []
        self.expect(tokenizer.Delim('['))
        while True:
            if self.match_token(tokenizer.Delim(']')):
                break
            if self.at_eof:
                break
            nodes.append(self.parse_expression())
            if self.match_token(tokenizer.Delim(',')):
                self.expect(tokenizer.Delim(','))
        self.expect(tokenizer.Delim(']'))

        return ListExpression(None, nodes)

    def parse_tree(self) -> Node:
        token = self.cur()
        self.expect(tokenizer.Tree())
        return TreeNode(token, [])

    def parse(self, tokens: [tokenizer.Token]) -> Node | None:
        if not tokens:
            return None

        self.tokens = tokens
        result = self.parse_source()
        if not self.at_eof:
            logging.error("[Parser] Failed to parse string past %s at %d",
                          self.cur(), self.cur_token)
            return None
        return result
