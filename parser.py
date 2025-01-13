# pylint: disable=missing-module-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring

from dataclasses import dataclass
import sys

import copy
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
class Use(Node):
    scope_name: str | None


@dataclass
class Module(Node):
    name: str | None


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


def strip(root: Node | None) -> Node | None:
    "Strips tree from dormant nodes (unary and without token)"

    if root is None:
        return root

    stripped_ones = (Source, Expression)

    def aux(n: Node) -> Node:
        if isinstance(n, stripped_ones):
            if len(n.children) == 0:
                return None
            return aux(n.children[0])
        new_node = copy.copy(n)
        new_node.children = [aux(c) for c in n.children]
        return new_node

    return aux(root)


def saturate(root: Node | None) -> Node | None:
    """Changes ($($($ ^ ^) ^) ^) to ($ (^ ^ ^) ^),
        therefore transforms a tree node
        to a leaf, stem or fork, to canonical form"""

    if root is None:
        return root

    def aux(r: Node) -> Node:

        def clone_with_new_children(node: Node, children: list):
            n = copy.copy(node)
            n.children = children
            return n

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
                        return clone_with_new_children(r, children)
                    case _:
                        sys.exit(-1)
            else:
                return clone_with_new_children(r, children)
        else:
            return clone_with_new_children(r, children)

    return aux(root)


class ParserExpressionError(Exception):
    pass


# pylint: disable-next=too-many-public-methods
class Parser:
    saved_self: dict | None = None
    tokens: [tokenizer.Token]
    cur_token = 0
    at_eof = False
    errors: list[str] = []

    def add_error(self, error: str):
        self.errors.append(error)

    def cur(self):
        return self.tokens[self.cur_token]

    def next(self) -> None:
        while True:
            self.cur_token += 1
            if self.cur_token >= len(self.tokens):
                self.at_eof = True
            return

    def save_self(self):
        self.saved_self = copy.copy(self.__dict__)

    def rollback(self) -> None:
        assert self.saved_self
        self.__dict__ = self.saved_self
        self.saved_self = None

    def match_token(self, token: tokenizer.Token, match_contents=True) -> bool:
        if self.at_eof:
            return False

        t = self.cur()
        if match_contents:
            return t == token

        return type(t) is type(token)

    def expect(self, token: tokenizer.Token, match_contents=True) -> bool:
        if self.at_eof:
            return False

        ok = self.match_token(token, match_contents)
        if not ok:
            self.add_error(f"[Parser] Unexpected '{self.cur()}' "
                           f"expected '{token}' at {self.cur_token}")
            self.at_eof = True
            return False

        self.next()
        return True

    def parse_source(self) -> Node:
        nodes = []
        while self.match_token(tokenizer.Symbol('module')):
            nodes.append(self.parse_module())
        nodes.append(self.parse_expression())
        return Source(None, nodes)

    def parse_expression(self) -> Node:
        nodes = []
        if self.match_token(tokenizer.Symbol('scope')):
            nodes.append(self.parse_scope())
        elif self.match_token(tokenizer.Symbol('use')):
            nodes.append(self.parse_use())
        else:
            app = self.parse_application(0 + 1)
            if not app:
                self.add_error(
                    f'[Parser] Expected expression at {self.cur_token}')
                raise ParserExpressionError
            nodes.append(app)
        return Expression(None, nodes)

    def operator_precedence(self):
        if self.at_eof:
            return None, 0

        token = self.cur()
        # NOTE: until binary operators are implemented,
        # need to list starting symbols for application
        # Better to list binary operators instead
        match token:
            case tokenizer.Symbol('end'):
                return None, 0
            case (tokenizer.String(_) | tokenizer.Symbol(_) | tokenizer.Tree(_)
                  | Delimeters.lparen | Delimeters.lsquare):
                return Application, 10
        return None, 0

    def parse_application(self, cur_prec: int) -> Node:
        lhs = self.parse_operand()
        if not lhs:
            return None
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

    # pylint: disable-next=too-many-return-statements
    def parse_operand(self) -> Node:
        if self.match_token(tokenizer.Symbol('scope')):
            return self.parse_scope()
        if self.match_token(tokenizer.Symbol('use')):
            return self.parse_scope()
        if self.match_token(tokenizer.String(''), match_contents=False):
            token = self.cur()
            self.next()
            return String(token, [])
        if self.match_token(tokenizer.Symbol(''), match_contents=False):
            token = self.cur()
            self.next()
            return Symbol(token, [])
        if self.match_token(tokenizer.Tree()):
            return self.parse_tree()
        if self.match_token(Delimeters.lsquare):
            return self.parse_list_expression()

        if not self.expect(Delimeters.lparen):
            return None
        node = self.parse_expression()
        if not node:
            return None
        self.expect(Delimeters.rparen)
        return node

    def parse_tree(self) -> Node:
        token = self.cur()
        self.expect(tokenizer.Tree())
        return TreeNode(token, [])

    def parse_list_expression(self) -> Node:
        nodes = []
        token = self.cur()
        self.expect(Delimeters.lsquare)
        while True:
            if self.match_token(Delimeters.rsquare):
                break
            if self.at_eof:
                break
            nodes.append(self.parse_expression())
            if self.match_token(Delimeters.comma):
                self.expect(Delimeters.comma)
        self.expect(Delimeters.rsquare)

        return ListExpression(token, nodes)

    def is_scope_binding(self) -> bool:
        self.save_self()
        result = False
        if self.match_token(tokenizer.Symbol(''), match_contents=False):
            self.next()
            result = self.match_token(tokenizer.Symbol('='))
            self.rollback()
        return result

    def parse_scope_body(self) -> list[Node]:
        nodes = []
        while True:
            if self.match_token(tokenizer.Symbol('module')):
                nodes.append(self.parse_module())
                continue
            if self.is_scope_binding():
                nodes.append(self.parse_scope_binding())
                continue
            break
        return nodes

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
            scope_name = self.cur().s
            self.next()
        self.expect(tokenizer.Symbol('do'))
        if self.match_token(tokenizer.Symbol('end')):
            self.add_error('[Parser] Expected expression in scope')
            self.at_eof = True
            return None

        nodes = self.parse_scope_body()
        if self.match_token(tokenizer.Symbol('end')):
            self.add_error('[Parser] Expected expression in scope')
            self.at_eof = True
            return None

        nodes.append(self.parse_expression())
        self.expect(tokenizer.Symbol('end'))
        return Scope(token, nodes, scope_name)

    def parse_module(self) -> Node:
        token = self.cur()
        self.expect(tokenizer.Symbol('module'))
        module_name = self.cur().s
        self.expect(tokenizer.String(''), match_contents=False)
        self.expect(tokenizer.Symbol('do'))
        nodes = self.parse_scope_body()
        self.expect(tokenizer.Symbol('end'))
        return Module(token, nodes, module_name)

    def parse_use(self) -> Node:
        token = self.cur()
        self.expect(tokenizer.Symbol('use'))
        scope_name = self.cur().s
        self.expect(tokenizer.String(''), match_contents=False)
        self.expect(tokenizer.Symbol('in'))

        expr = self.parse_expression()
        return Use(token, [expr], scope_name)

    def parse(self, tokens: [tokenizer.Token]) -> Node:
        if not tokens:
            return None

        self.tokens = tokens
        try:
            result = self.parse_source()
        except ParserExpressionError:
            result = None

        if not self.at_eof:
            self.add_error(f"[Parser] Failed to parse string past"
                           f" {self.cur()} at {self.cur_token}")
        if self.errors:
            raise RuntimeError("Parser encountered errors:\n"
                               "\n".join(self.errors))
        return result
