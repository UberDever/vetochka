# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

from dataclasses import dataclass
import logging
import sys

import tokenizer
from tokenizer import Delimeters


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
class Application(Node):
    pass


@dataclass
class ListExpression(Node):
    pass


@dataclass
class Scope(Node):
    name: str | None


@dataclass
class ScopeBinding(Node):
    pass


@dataclass
class String(Node):
    pass


@dataclass
class Symbol(Node):
    pass


@dataclass
class TreeNode(Node):

    def is_leaf(self):
        return len(self.children) == 0

    def is_stem(self):
        return len(self.children) == 1

    def is_fork(self):
        return len(self.children) == 2


def strip(root: Node) -> Node:
    "Strips tree from dormant nodes (unary and without token)"

    def aux(n: Node) -> Node:
        if n.token is None and len(n.children) == 1:
            return aux(n.children[0])
        return type(n)(n.token, [aux(c) for c in n.children])

    return aux(root)


def saturate(root: Node) -> Node:
    """Changes ($($($ ^ ^) ^) ^) to ($ (^ ^ ^) ^),
        therefore transforms a tree node
        to a leaf, stem or fork, to canonical form"""

    def aux(r: Node) -> Node:
        if not r.children:
            return r

        children = [aux(n) for n in r.children]

        if isinstance(r, Application):
            left = children[0]
            right = children[1]
            if isinstance(left, TreeNode):
                match left:
                    case _ if left.is_leaf():
                        return TreeNode(token=left.token,
                                        children=[aux(right)])
                    case _ if left.is_stem():
                        return TreeNode(
                            token=left.token,
                            children=[aux(left.children[0]),
                                      aux(right)])
                    case _ if left.is_fork():
                        return type(r)(token=r.token, children=children)
                    case _:
                        sys.exit(-1)
            else:
                return type(r)(token=r.token, children=children)
        else:
            return type(r)(token=r.token, children=children)

    return aux(root)


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
        if self.match_token(tokenizer.Symbol('scope')):
            nodes.append(self.parse_scope())
        else:
            nodes.append(self.parse_application(0 + 1))
        return Expression(None, nodes)

    def operator_precedence(self):
        if self.at_eof:
            return None, 0

        token = self.cur()
        # NOTE: until binary operators are implemented,
        # need to list starting symbols for application
        # Better to list binary operators instead
        match token:
            case tokenizer.String(_) | tokenizer.Symbol(_) | tokenizer.Tree(
                _) | Delimeters.rparen | Delimeters.rsquare:
                return Application, 10
            case tokenizer.Delim:
                logging.fatal('Not implemented')
                sys.exit(-1)
        return None, 0

    def parse_application(self, cur_prec: int) -> Node:
        lhs = self.parse_operand()
        while True:
            ctor, prec = self.operator_precedence()
            if prec < cur_prec:
                return lhs
            token = self.cur()
            if ctor is Application:
                token = lhs.token
            else:
                self.next()
            rhs = self.parse_application(prec + 1)
            lhs = ctor(token, [lhs, rhs])

    def parse_operand(self) -> Node:
        token = self.cur()
        if self.match_token(tokenizer.String(''), match_contents=False):
            self.next()
            return String(token, [])
        if self.match_token(tokenizer.Symbol(''), match_contents=False):
            self.next()
            return Symbol(token, [])
        if self.match_token(tokenizer.Tree()):
            return self.parse_tree()
        if self.match_token(Delimeters.rsquare):
            return self.parse_list_expression()

        self.expect(Delimeters.rparen)
        node = self.parse_expression()
        self.expect(Delimeters.lparen)
        return node

    def parse_tree(self) -> Node:
        token = self.cur()
        self.expect(tokenizer.Tree())
        return TreeNode(token, [])

    def parse_list_expression(self) -> Node:
        nodes = []
        token = self.cur()
        self.expect(Delimeters.rsquare)
        while True:
            if self.match_token(Delimeters.lsquare):
                break
            if self.at_eof:
                break
            nodes.append(self.parse_expression())
            if self.match_token(Delimeters.comma):
                self.expect(Delimeters.comma)
        self.expect(Delimeters.lsquare)

        return ListExpression(token, nodes)

    def is_scope_binding(self) -> bool:
        at = self.cur_token
        result = False
        if self.match_token(tokenizer.Symbol, match_contents=False):
            self.next()
            result = self.match_token(tokenizer.Symbol('='))
            self.cur_token = at
        return result

    def parse_scope_binding(self) -> Node:
        token = self.cur()
        self.next()  # Symbol
        self.next()  # =
        bound_to = Symbol(token, [])
        expr = self.parse_expression()
        self.expect(tokenizer.Delimeters.semicolon)
        return ScopeBinding(token, [bound_to, expr])

    def parse_scope(self) -> Node:
        token = self.cur()
        self.expect(tokenizer.Symbol('scope'))
        scope_name = None
        if self.match_token(tokenizer.String(''), match_contents=False):
            self.next()
            scope_name = self.cur().s
        self.expect(tokenizer.Symbol('do'))
        if self.match_token(tokenizer.Symbol('end')):
            logging.error('[Parser] Expected expression in scope')
            self.at_eof = True
            self.was_error = True
            return None

        nodes = []
        while self.is_scope_binding():
            nodes.append(self.parse_scope_binding())
        if self.match_token(tokenizer.Symbol('end')):
            logging.error('[Parser] Expected expression in scope')
            self.at_eof = True
            self.was_error = True
            return None

        nodes.append(self.parse_expression())
        self.expect(tokenizer.Symbol('end'))
        return Scope(token, nodes, scope_name)

    def parse(self, tokens: [tokenizer.Token]) -> Node | None:
        if not tokens:
            return None

        self.tokens = tokens
        result = self.parse_source()
        if self.was_error:
            logging.error("[Parser] There was some errors")
            return None
        if not self.at_eof:
            logging.error("[Parser] Failed to parse string past %s at %d",
                          self.cur(), self.cur_token)
            return None
        return result
