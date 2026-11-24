"""Throwaway parser for vf with symbols and colon blocks (24.11.26, 6)-(24.11.26, 13).

Not the project parser. Changes from artifacts/20261124-123200/vfp.py:

* no labels; `:` alone opens a block, `;;` closes it, `#` starts a line comment;
* `~name` is a symbol and lowers to the fruit `{:name}`;
* loose application takes only a symbol (with its tight postfixes) or a block,
  and lowers exactly like the parenthesized application: `f ~x` as `f(~x)`;
* `|` is an ordinary operator character;
* adjacency: a delimiter never touches its block's contents; the opener may
  touch a preceding symbol or closing bracket or brace; otherwise, outside,
  a delimiter may touch only the bracket enclosing its block, or a comma.
  A plain identifier may take a block across whitespace (`defer : ... ;;`);
  the corpus uses a symbol instead (`defer ~do: ... ;;`) by convention.

v0 forms (`\\[...]`, `?[...]`, `=[...]`) parse as before: vf includes v0.
"""

from __future__ import annotations

from dataclasses import dataclass


class ParseError(Exception):
    def __init__(self, msg: str, line: int, col: int = 0):
        super().__init__(f"{line}:{col}: {msg}")
        self.msg = msg
        self.line = line
        self.col = col


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
    ws: bool = False
    nl: bool = False


class Lexer:
    def __init__(self, text: str):
        self.s = text
        self.i = 0
        self.toks: list[Tok] = []

    def err(self, msg: str, pos: int | None = None):
        pos = self.i if pos is None else pos
        line = self.s.count("\n", 0, pos) + 1
        col = pos - (self.s.rfind("\n", 0, pos) + 1) + 1
        raise ParseError(msg, line, col)

    def at(self, lit: str) -> bool:
        return self.s.startswith(lit, self.i)

    def at_line_comment(self) -> bool:
        return self.at("#") and not self.at("#|") and not self.at("#;")

    def skip_trivia(self) -> tuple[bool, bool]:
        ws = nl = False
        s = self.s
        while self.i < len(s):
            c = s[self.i]
            if c in " \t\v\f":
                ws = True
                self.i += 1
            elif c in "\r\n":
                ws = nl = True
                self.i += 1
            elif self.at("#|"):
                ws = True
                self.block_comment()
            elif self.at("..."):
                ws = True
                self.i += 3
                while self.i < len(s) and s[self.i] in " \t":
                    self.i += 1
                if self.at_line_comment():
                    while self.i < len(s) and s[self.i] != "\n":
                        self.i += 1
                if self.i < len(s) and s[self.i] == "\n":
                    self.i += 1
                else:
                    self.err("`...` must end its line")
            elif self.at_line_comment():
                ws = True
                while self.i < len(s) and s[self.i] != "\n":
                    self.i += 1
            else:
                break
        return ws, nl

    def block_comment(self):
        start = self.i
        depth = 0
        while self.i < len(self.s):
            if self.at("#|"):
                depth += 1
                self.i += 2
            elif self.at("|#"):
                depth -= 1
                self.i += 2
                if depth == 0:
                    return
            else:
                self.i += 1
        self.err("unterminated block comment", start)

    def push(self, kind: str, start: int, ws: bool, nl: bool):
        line = self.s.count("\n", 0, start) + 1
        col = start - (self.s.rfind("\n", 0, start) + 1) + 1
        self.toks.append(Tok(kind, self.s[start : self.i], start, self.i, line, col, ws, nl))

    def string(self):
        depth = 0
        while self.i < len(self.s):
            c = self.s[self.i]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    self.i += 1
                    return
            self.i += 1
        self.err("unterminated string")

    def run(self) -> list[Tok]:
        s = self.s
        simple = {";": "SEMI", ",": "COMMA", "(": "LPAREN", ")": "RPAREN", "[": "LBRACK", "]": "RBRACK"}
        while True:
            ws, nl = self.skip_trivia()
            if self.i >= len(s):
                self.push("EOF", self.i, ws, nl)
                break
            start = self.i
            c = s[self.i]
            if self.at("#;"):
                self.i += 2
                self.push("DISCARD", start, ws, nl)
            elif self.at(";;"):
                self.i += 2
                self.push("CLOSE", start, ws, nl)
            elif c in simple:
                self.i += 1
                self.push(simple[c], start, ws, nl)
            elif c == "{":
                self.string()
                self.push("STR", start, ws, nl)
            elif c == "~":
                self.i += 1
                if self.at("{"):
                    self.string()
                    hexes = "".join(s[start + 2 : self.i - 1].split())
                    if any(ch not in "0123456789abcdefABCDEF" for ch in hexes) or len(hexes) % 2:
                        self.err("malformed raw bytes", start)
                    self.push("BYTES", start, ws, nl)
                elif self.at("["):
                    self.i += 1
                    self.push("NYAD", start, ws, nl)
                elif self.i < len(s) and is_id_start(s[self.i]):
                    while self.i < len(s) and is_id_continue(s[self.i]):
                        self.i += 1
                    self.push("SYMBOL", start, ws, nl)
                else:
                    self.push("LEAF", start, ws, nl)
            elif c == "\\" and s.startswith("[", self.i + 1):
                self.i += 2
                self.push("LAMBDA", start, ws, nl)
            elif c == "?" and s.startswith("[", self.i + 1):
                self.i += 2
                self.push("ROW", start, ws, nl)
            elif c.isdigit() and c.isascii():
                while self.i < len(s) and s[self.i].isdigit() and s[self.i].isascii():
                    self.i += 1
                self.push("INT", start, ws, nl)
            elif is_id_start(c):
                while self.i < len(s) and is_id_continue(s[self.i]):
                    self.i += 1
                self.push("IDENT", start, ws, nl)
            elif c in OP_CHARS:
                while self.i < len(s) and s[self.i] in OP_CHARS and not self.at("..."):
                    self.i += 1
                text = s[start : self.i]
                if text == ":":
                    self.push("OPEN", start, ws, nl)
                elif text == "=" and self.at("["):
                    self.i += 1
                    self.push("BINDER", start, ws, nl)
                else:
                    self.push("OP", start, ws, nl)
            else:
                self.err(f"unexpected character {c!r}")
        self.check_adjacency()
        return self.toks

    def check_adjacency(self):
        toks = self.toks
        enclosing_open = {"LPAREN", "LBRACK", "COMMA"}
        may_precede_opener = enclosing_open | {"SYMBOL", "RPAREN", "RBRACK", "STR", "BYTES"}
        enclosing_close = {"RPAREN", "RBRACK", "COMMA"}
        for k, t in enumerate(toks):
            if t.kind not in ("OPEN", "CLOSE"):
                continue
            prev = toks[k - 1] if k else None
            nxt = toks[k + 1]
            if t.kind == "OPEN":
                if prev is not None and not t.ws and prev.kind not in may_precede_opener:
                    raise ParseError(f"block opener touches {prev.text!r}", t.line, t.col)
                if nxt.kind != "EOF" and not nxt.ws:
                    raise ParseError("block opener touches the block's contents", nxt.line, nxt.col)
            else:
                if not t.ws:
                    raise ParseError("block closer touches the block's contents", t.line, t.col)
                if nxt.kind != "EOF" and not nxt.ws and nxt.kind not in enclosing_close:
                    raise ParseError(f"block closer touches {nxt.text!r}", nxt.line, nxt.col)


def lex(text: str) -> list[Tok]:
    return Lexer(text).run()


class Parser:
    def __init__(self, toks: list[Tok]):
        self.t = toks
        self.k = 0

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

    def skip_discards(self, layout: bool):
        while self.peek().kind == "DISCARD":
            self.next()
            self.expression(layout)

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
            elif not tok.nl:
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
            elif tok.kind != end:
                self.err(f"expected ',' or closing bracket, found {tok.text!r}")

    def expression(self, layout: bool):
        kind = self.peek().kind
        if kind == "OPEN":
            return self.block()
        if kind == "LAMBDA":
            return self.closure(layout)
        if kind == "BINDER":
            return self.binder(layout)
        return self.infix(layout)

    def block(self):
        self.expect("OPEN")
        items = self.block_list("CLOSE")
        self.expect("CLOSE")
        return ("block", items)

    # v0 forms
    def closure(self, layout: bool):
        self.expect("LAMBDA")
        params = self.comma_list("RBRACK")
        self.expect("RBRACK")
        if not params:
            self.err("a lambda needs at least one named parameter")
        if self.peek().kind == "ROW":
            rows = []
            while self.peek().kind == "ROW":
                self.next()
                pattern = self.comma_list("RBRACK")
                self.expect("RBRACK")
                rows.append(("row", pattern, self.expression(layout)))
            return ("lambda", params, ("rows", rows))
        return ("lambda", params, self.expression(layout))

    def binder(self, layout: bool):
        self.expect("BINDER")
        clauses = self.comma_list("RBRACK")
        self.expect("RBRACK")
        return ("binder", clauses, self.expression(layout))

    def infix(self, layout: bool):
        operands = [self.prefix(layout)]
        ops = []
        while True:
            tok = self.peek()
            if tok.kind != "OP" or (layout and tok.nl):
                break
            if not tok.ws or not self.peek(1).ws:
                self.err(f"infix operator {tok.text!r} needs trivia on both sides", tok)
            ops.append(self.next().text)
            nxt = self.peek().kind
            if nxt == "LAMBDA":
                operands.append(self.closure(layout))
                break
            if nxt == "BINDER":
                operands.append(self.binder(layout))
                break
            operands.append(self.prefix(layout))
        return operands[0] if not ops else ("infix", ops, operands)

    def prefix(self, layout: bool):
        tok = self.peek()
        if tok.kind == "OP":
            if self.peek(1).ws:
                self.err(f"prefix operator {tok.text!r} must touch its operand", tok)
            self.next()
            return ("prefix", tok.text, self.prefix(layout))
        return self.postfix(layout)

    def tight(self, expr):
        while True:
            tok = self.peek()
            if tok.ws:
                return expr
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
                return expr

    def postfix(self, layout: bool):
        expr = self.tight(self.primary())
        while True:
            tok = self.peek()
            glued_opener = tok.kind == "OPEN" and not tok.ws  # allowed by adjacency
            if not (tok.ws or glued_opener) or (layout and tok.nl):
                return expr
            if tok.kind == "OPEN":
                expr = ("call", expr, [self.block()])
            elif tok.kind == "SYMBOL":
                expr = ("call", expr, [self.tight(self.primary())])
            else:
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
        if k == "SYMBOL":
            self.next()
            return ("str", ":" + tok.text[1:])  # `~name` is the fruit `{:name}`
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


def parse(text: str):
    return Parser(lex(text)).source()
