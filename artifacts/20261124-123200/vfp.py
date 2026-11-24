"""Throwaway vf surface parser for the block-delimiter experiment.

Not the project parser. It reconstructs the shared vf shape from the devlog
replay of 24.11.26 (see report.md, "Grammar used") and makes the block
delimiters a parameter, so one corpus can be parsed under several designs:

* punctuation design: blocks `| ... ;;`, line comments `# ...`;
* word designs: blocks `<open> ... <close>` (whole identifier tokens),
  line comments `;; ...` (the status quo).

Both designs keep `#| ... |#` block comments and `#;` datum comments, and both
apply the same adjacency rule to their delimiters: a delimiter never touches
the block's contents; outside, it may touch only a bracket or a comma.

The parser lowers to plain tuples. Delimiters leave no trace in the tree, so
the same program written with different delimiters must lower identically.
"""

from __future__ import annotations

import unicodedata
from dataclasses import dataclass, field


class ParseError(Exception):
    def __init__(self, msg: str, line: int, col: int = 0):
        super().__init__(f"{line}:{col}: {msg}")
        self.msg = msg
        self.line = line
        self.col = col


@dataclass(frozen=True)
class Design:
    name: str
    opener: str  # "|" or the word used when rendering
    closer: str  # ";;" or the word used when rendering
    line_comment: str  # "#" or ";;"
    openers: frozenset = frozenset()  # every word recognised as an opener
    closers: frozenset = frozenset()  # every word recognised as a closer

    @property
    def punct(self) -> bool:
        return self.opener == "|"


PUNCT = Design("| ;;", "|", ";;", "#")


def word_design(name: str, opener: str, closer: str, openers=(), closers=()) -> Design:
    return Design(name, opener, closer, ";;",
                  frozenset({opener, *openers}), frozenset({closer, *closers}))


OP_CHARS = set("=+-*/%<>!&|:^.")
ID_EXTRA = set("_?'+-*/%<>!&")


def is_id_start(c: str) -> bool:
    return c == "_" or c.isalpha()


def is_id_continue(c: str) -> bool:
    return c.isalnum() or c in ID_EXTRA


@dataclass
class Tok:
    kind: str
    text: str
    start: int
    end: int
    line: int
    col: int
    ws: bool = False  # trivia between the previous token and this one
    nl: bool = False  # a layout newline between the previous token and this one
    discard: bool = False  # a `#;` precedes this token


# kinds: IDENT INT STR BYTES LEAF NYAD LPAREN RPAREN LBRACK RBRACK COMMA SEMI
#        COLON OP LAMBDA ROW BINDER OPEN CLOSE EOF DISCARD


class Lexer:
    def __init__(self, text: str, design: Design):
        self.s = text
        self.d = design
        self.i = 0
        self.line = 1
        self.line_start = 0
        self.toks: list[Tok] = []
        self.line_comments: list[int] = []  # offsets of line-comment introducers

    def err(self, msg: str, pos: int | None = None):
        pos = self.i if pos is None else pos
        line = self.s.count("\n", 0, pos) + 1
        col = pos - (self.s.rfind("\n", 0, pos) + 1) + 1
        raise ParseError(msg, line, col)

    def at(self, lit: str) -> bool:
        return self.s.startswith(lit, self.i)

    def adv(self, n: int = 1):
        for _ in range(n):
            if self.s[self.i] == "\n":
                self.line += 1
                self.line_start = self.i + 1
            self.i += 1

    def skip_trivia(self) -> tuple[bool, bool]:
        ws = nl = False
        s = self.s
        while self.i < len(s):
            c = s[self.i]
            if c in " \t\v\f":
                ws = True
                self.adv()
            elif c in "\r\n":
                ws = nl = True
                self.adv()
            elif self.at("#|"):
                ws = True
                self.block_comment()
            elif self.at("..."):
                # continuation: `...` hws* line_comment? newline
                ws = True
                self.adv(3)
                while self.i < len(s) and s[self.i] in " \t":
                    self.adv()
                if self.at_line_comment():
                    self.line_comments.append(self.i)
                    while self.i < len(s) and s[self.i] != "\n":
                        self.adv()
                if self.i < len(s) and s[self.i] == "\n":
                    self.adv()
                else:
                    self.err("`...` must end its line")
            elif self.at_line_comment():
                ws = True
                self.line_comments.append(self.i)
                while self.i < len(s) and s[self.i] != "\n":
                    self.adv()
            else:
                break
        return ws, nl

    def at_line_comment(self) -> bool:
        if self.d.line_comment == "#":
            return self.at("#") and not self.at("#|") and not self.at("#;")
        return self.at(";;")

    def block_comment(self):
        start = self.i
        depth = 0
        while self.i < len(self.s):
            if self.at("#|"):
                depth += 1
                self.adv(2)
            elif self.at("|#"):
                depth -= 1
                self.adv(2)
                if depth == 0:
                    return
            else:
                self.adv()
        self.err("unterminated block comment", start)

    def push(self, kind: str, start: int, ws: bool, nl: bool):
        col = start - self.line_start + 1
        line = self.s.count("\n", 0, start) + 1
        self.toks.append(Tok(kind, self.s[start:self.i], start, self.i, line, col, ws, nl))

    def string(self):
        depth = 0
        while self.i < len(self.s):
            c = self.s[self.i]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    self.adv()
                    return
            self.adv()
        self.err("unterminated string")

    def run(self) -> list[Tok]:
        s = self.s
        pending_discard = False
        while True:
            ws, nl = self.skip_trivia()
            if self.i >= len(s):
                self.push("EOF", self.i, ws, nl)
                break
            start = self.i
            c = s[self.i]
            if self.at("#;"):
                self.adv(2)
                self.push("DISCARD", start, ws, nl)
                continue
            if self.d.punct and self.at(";;"):
                self.adv(2)
                self.push("CLOSE", start, ws, nl)
            elif c == ";":
                self.adv()
                self.push("SEMI", start, ws, nl)
            elif c == ",":
                self.adv()
                self.push("COMMA", start, ws, nl)
            elif c == "(":
                self.adv()
                self.push("LPAREN", start, ws, nl)
            elif c == ")":
                self.adv()
                self.push("RPAREN", start, ws, nl)
            elif c == "[":
                self.adv()
                self.push("LBRACK", start, ws, nl)
            elif c == "]":
                self.adv()
                self.push("RBRACK", start, ws, nl)
            elif c == "{":
                self.string()
                self.push("STR", start, ws, nl)
            elif c == "~":
                self.adv()
                if self.at("{"):
                    self.string()
                    body = s[start + 2 : self.i - 1]
                    hexes = "".join(body.split())
                    if any(ch not in "0123456789abcdefABCDEF" for ch in hexes) or len(hexes) % 2:
                        self.err("malformed raw bytes", start)
                    self.push("BYTES", start, ws, nl)
                elif self.at("["):
                    self.adv()
                    self.push("NYAD", start, ws, nl)
                else:
                    self.push("LEAF", start, ws, nl)
            elif c == "\\" and s.startswith("[", self.i + 1):
                self.adv(2)
                self.push("LAMBDA", start, ws, nl)
            elif c == "?" and s.startswith("[", self.i + 1):
                self.adv(2)
                self.push("ROW", start, ws, nl)
            elif c.isdigit() and c.isascii():
                while self.i < len(s) and s[self.i].isdigit() and s[self.i].isascii():
                    self.adv()
                if self.i < len(s) and is_id_continue(s[self.i]) and s[self.i] not in "+-*/%<>!&":
                    self.err("glued integer")
                self.push("INT", start, ws, nl)
            elif is_id_start(c):
                while self.i < len(s) and is_id_continue(s[self.i]):
                    self.adv()
                text = s[start : self.i]
                kind = "IDENT"
                label_word = self.at(":") and not self.at("::")
                if label_word:
                    pass  # `do:` and `end:` stay labels (20.10.26, 1)
                elif not self.d.punct and text in self.d.openers:
                    kind = "OPEN"
                elif not self.d.punct and text in self.d.closers:
                    kind = "CLOSE"
                self.push(kind, start, ws, nl)
            elif c in OP_CHARS:
                while self.i < len(s) and s[self.i] in OP_CHARS:
                    # `...` is continuation, not an operator
                    if self.at("..."):
                        break
                    self.adv()
                text = s[start : self.i]
                if text == ":":
                    self.push("COLON", start, ws, nl)
                elif text == "=" and self.at("["):
                    self.adv()
                    self.push("BINDER", start, ws, nl)
                elif self.d.punct and text == "|":
                    self.push("OPEN", start, ws, nl)
                else:
                    self.push("OP", start, ws, nl)
            else:
                self.err(f"unexpected character {c!r}")
        self.check_adjacency()
        return self.toks

    def check_adjacency(self):
        toks = self.toks
        # outside, a delimiter may touch only the bracket enclosing its block, or a comma
        before_open = {"LPAREN", "LBRACK", "COMMA"}
        after_close = {"RPAREN", "RBRACK", "COMMA"}
        for k, t in enumerate(toks):
            if t.kind not in ("OPEN", "CLOSE"):
                continue
            prev = toks[k - 1] if k else None
            nxt = toks[k + 1]
            if t.kind == "OPEN":
                if prev is not None and not t.ws and prev.kind not in before_open:
                    raise ParseError("block opener touches the preceding token", t.line, t.col)
                if nxt.kind != "EOF" and not nxt.ws:
                    raise ParseError("block opener touches the block's contents", nxt.line, nxt.col)
            else:
                if not t.ws:
                    raise ParseError("block closer touches the block's contents", t.line, t.col)
                if nxt.kind != "EOF" and not nxt.ws and nxt.kind not in after_close:
                    raise ParseError("block closer touches the following token", nxt.line, nxt.col)


def lex(text: str, design: Design) -> list[Tok]:
    return Lexer(text, design).run()


class Parser:
    def __init__(self, toks: list[Tok]):
        self.t = toks
        self.k = 0

    # -- token helpers -------------------------------------------------------
    def peek(self, n: int = 0) -> Tok:
        return self.t[min(self.k + n, len(self.t) - 1)]

    def next(self) -> Tok:
        tok = self.t[self.k]
        self.k += 1
        return tok

    def err(self, msg: str, tok: Tok | None = None):
        tok = tok or self.peek()
        raise ParseError(msg, tok.line, tok.col)

    def expect(self, kind: str) -> Tok:
        tok = self.peek()
        if tok.kind != kind:
            what = "end of input" if tok.kind == "EOF" else repr(tok.text)
            self.err(f"expected {kind}, found {what}")
        return self.next()

    def continues(self, tok: Tok, layout: bool) -> bool:
        """A complete expression may extend over `tok` only if no layout newline intervenes."""
        return not (layout and tok.nl)

    def skip_discards(self, layout: bool):
        while self.peek().kind == "DISCARD":
            self.next()
            self.expression(layout)

    # -- lists ---------------------------------------------------------------
    def source(self):
        items = self.block_list("EOF")
        self.expect("EOF")
        return ("source", items)

    def block_list(self, end: str):
        items = []
        while True:
            self.skip_discards(True)
            while self.peek().kind == "SEMI":
                self.next()
                self.skip_discards(True)
            if self.peek().kind == end:
                return items
            items.append(self.expression(True))
            self.skip_discards(True)
            tok = self.peek()
            if tok.kind == "SEMI":
                self.next()
            elif tok.kind == end:
                return items
            elif tok.nl:
                continue
            else:
                self.err(f"expected a statement separator, found {tok.text!r}")

    def comma_list(self, end: str):
        items = []
        while True:
            self.skip_discards(False)
            if self.peek().kind == end:
                return items
            items.append(self.expression(False))
            self.skip_discards(False)
            tok = self.peek()
            if tok.kind == "COMMA":
                self.next()
            elif tok.kind == end:
                return items
            else:
                self.err(f"expected ',' or closing bracket, found {tok.text!r}")

    # -- expressions -----------------------------------------------------------
    def at_label(self) -> bool:
        tok = self.peek()
        if tok.kind == "COLON":
            return True
        nxt = self.peek(1)
        return tok.kind in ("IDENT", "OPEN", "CLOSE") and nxt.kind == "COLON" and not nxt.ws

    def expression(self, layout: bool):
        tok = self.peek()
        if self.at_label():
            return self.labeled(layout)
        if tok.kind == "OPEN":
            return self.block()
        if tok.kind == "LAMBDA":
            return self.closure(layout, tight=False)
        if tok.kind == "BINDER":
            return self.binder(layout, tight=False)
        return self.infix(layout, tight=False)

    def labeled(self, layout: bool):
        name = None
        if self.peek().kind != "COLON":
            name = self.next().text
        self.expect("COLON")
        tok = self.peek()
        # the payload may start on the next line: the label still needs syntax
        if tok.kind == "OPEN":
            payload = self.block()
        else:
            payload = self.argument(layout)
        return ("label", name, payload)

    def argument(self, layout: bool):
        tok = self.peek()
        if tok.kind == "LAMBDA":
            return self.closure(layout, tight=True)
        if tok.kind == "BINDER":
            return self.binder(layout, tight=True)
        return self.infix(layout, tight=True)

    def block(self):
        self.expect("OPEN")
        items = self.block_list("CLOSE")
        self.expect("CLOSE")
        return ("block", items)

    def closure(self, layout: bool, tight: bool):
        self.expect("LAMBDA")
        params = self.comma_list("RBRACK")
        self.expect("RBRACK")
        if not params:
            self.err("a lambda needs at least one named parameter")
        if self.peek().kind == "ROW":
            return ("lambda", params, self.rows(layout))
        if self.peek().kind == "OPEN":
            return ("lambda", params, self.block())
        body = self.argument(layout) if tight else self.expression(layout)
        return ("lambda", params, body)

    def rows(self, layout: bool):
        rows = []
        while self.peek().kind == "ROW":
            # rows continue across newlines (22.11.26, 7)
            self.next()
            pattern = self.comma_list("RBRACK")
            self.expect("RBRACK")
            rows.append(("row", pattern, self.expression(layout)))
        return ("rows", rows)

    def binder(self, layout: bool, tight: bool):
        self.expect("BINDER")
        clauses = self.comma_list("RBRACK")
        self.expect("RBRACK")
        body = self.argument(layout) if tight else self.expression(layout)
        return ("binder", clauses, body)

    def infix(self, layout: bool, tight: bool):
        operands = [self.prefix(layout, tight)]
        ops = []
        while True:
            tok = self.peek()
            if tok.kind != "OP" or not self.continues(tok, layout):
                break
            if not tok.ws or not self.peek(1).ws:
                self.err(f"infix operator {tok.text!r} needs trivia on both sides", tok)
            ops.append(self.next().text)
            nxt = self.peek()
            # a lambda or binder may end an infix chain, as in `f = \[x] x`
            # (artifacts/20261122-095953/force-flat.vf); it extends to the end
            if nxt.kind == "LAMBDA":
                operands.append(self.closure(layout, tight))
                break
            if nxt.kind == "BINDER":
                operands.append(self.binder(layout, tight))
                break
            operands.append(self.prefix(layout, tight))
        if not ops:
            return operands[0]
        return ("infix", ops, operands)

    def prefix(self, layout: bool, tight: bool):
        tok = self.peek()
        if tok.kind == "OP":
            if self.peek(1).ws:
                self.err(f"prefix operator {tok.text!r} must touch its operand", tok)
            self.next()
            return ("prefix", tok.text, self.prefix(layout, tight))
        return self.postfix(layout, tight)

    def postfix(self, layout: bool, tight: bool):
        expr = self.primary()
        while True:
            tok = self.peek()
            if tok.ws:
                break
            if tok.kind == "LPAREN":
                self.next()
                args = self.comma_list("RPAREN")
                self.expect("RPAREN")
                expr = ("call", expr, args)
            elif tok.kind == "LBRACK":
                self.next()
                items = self.comma_list("RBRACK")
                self.expect("RBRACK")
                expr = ("apply-list", expr, items)
            elif tok.kind == "STR":
                self.next()
                expr = ("apply-str", expr, tok.text[1:-1])
            else:
                break
        if tight:
            return expr
        while True:
            tok = self.peek()
            if not tok.ws or not self.continues(tok, layout):
                break
            if tok.kind == "OPEN":
                expr = ("apply-block", expr, self.block())
            elif self.at_label():
                expr = ("apply-label", expr, self.labeled(layout))
            else:
                break
        return expr

    def primary(self):
        tok = self.peek()
        k = tok.kind
        if k == "INT":
            self.next()
            return ("int", int(tok.text))
        if k == "STR":
            self.next()
            return ("str", tok.text[1:-1])
        if k == "BYTES":
            self.next()
            return ("bytes", "".join(tok.text[2:-1].split()).lower())
        if k == "IDENT":
            self.next()
            return ("id", tok.text)
        if k == "LEAF":
            self.next()
            return ("leaf",)
        if k == "NYAD":
            self.next()
            kids = self.comma_list("RBRACK")
            self.expect("RBRACK")
            if len(kids) > 2:
                self.err("a nyad has at most two children", tok)
            return ("nyad", kids)
        if k == "LBRACK":
            self.next()
            items = self.comma_list("RBRACK")
            self.expect("RBRACK")
            return ("list", items)
        if k == "LPAREN":
            self.next()
            inner = self.expression(False)
            self.expect("RPAREN")
            return inner
        if k == "EOF":
            self.err("unexpected end of input")
        if k in ("OPEN", "CLOSE"):
            self.err(f"block delimiter {tok.text!r} where an operand is expected")
        self.err(f"unexpected {tok.text!r}")


def parse(text: str, design: Design):
    return Parser(lex(text, design)).source()
