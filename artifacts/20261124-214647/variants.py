"""What if `port: 8080` were allowed? Run: python3 -I variants.py

Measured for (24.11.26, 15), before the corpus replaced `defer :` and the
like with `defer ~do:` by convention (24.11.26, 16); rerunning on the later
corpus gives slightly different counts.

Variants of vfs.py, measured on corpus/:

* strict:  the decided rule; a plain identifier may not touch `:`.
* relaxed: a plain identifier may touch `:`; blocks still close with `;;`.
* inline:  relaxed, and a block whose contents start on the opener's line ends
           at that line's end unless `;;` closes it first; a block opened at the
           end of a line closes with `;;`.
"""

from __future__ import annotations

import sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import vfs  # noqa: E402
from vfs import ParseError  # noqa: E402


class RelaxedLexer(vfs.Lexer):
    def check_adjacency(self):
        toks = self.toks
        for k, t in enumerate(toks):
            if t.kind == "OPEN" and k and not t.ws and toks[k - 1].kind == "IDENT":
                t.ws = True  # pretend trivia so the strict check passes; the tree is unchanged
        super().check_adjacency()


class InlineParser(vfs.Parser):
    def block(self):
        self.expect("OPEN")
        if self.peek().nl or self.peek().kind == "CLOSE":
            items = self.block_list("CLOSE")
            self.expect("CLOSE")
            return ("block", items)
        items = []
        while True:  # inline: statements until `;;` or the end of the line
            items.append(self.expression(True))
            tok = self.peek()
            if tok.kind == "SEMI":
                self.next()
                continue
            if tok.kind == "CLOSE" and not tok.nl:
                self.next()
                return ("block", items)
            if tok.nl or tok.kind in ("EOF", "CLOSE", "RPAREN", "RBRACK", "COMMA"):
                return ("block", items)
            self.err(f"expected a statement separator, found {tok.text!r}")


def parse_with(variant: str, text: str):
    if variant == "strict":
        return vfs.parse(text)
    toks = RelaxedLexer(text).run()
    parser = InlineParser(toks) if variant == "inline" else vfs.Parser(toks)
    return parser.source()


def outcome(variant, mutated, line, ref):
    try:
        tree = parse_with(variant, mutated)
    except ParseError as e:
        return "local" if e.line == line else "distant"
    return "same" if tree == ref else "silent"


def mutants(text: str):
    toks = vfs.lex(text)
    for k, t in enumerate(toks[:-1]):
        nxt = toks[k + 1]
        prev = toks[k - 1] if k else None
        line = text.count("\n", 0, t.start) + 1
        if t.kind == "SYMBOL" and nxt.kind == "LPAREN" and not nxt.ws:
            depth, j = 0, k + 1
            while True:
                if toks[j].kind in ("LPAREN", "LBRACK", "NYAD"):
                    depth += 1
                elif toks[j].kind in ("RPAREN", "RBRACK"):
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            inner = text[nxt.end : toks[j].start]
            if "\n" not in inner:
                yield "old label habit `ret: bool`", text[: t.start] + f"{t.text[1:]}: {inner}" + text[toks[j].end :], line
        if t.kind == "SYMBOL" and nxt.kind == "OPEN" and not nxt.ws:
            yield "`else:` without `~`", text[: t.start] + text[t.start + 1 :], line
        if t.kind == "CLOSE" and nxt.kind == "SYMBOL" and not nxt.nl and prev is not None and not prev.nl:
            # an inline then-branch followed by `~else:` on the same line, its `;;` forgotten
            yield "inline `;;` before `~else` forgotten", text[: prev.end] + text[t.end :], line


def main():
    corpus = sorted((HERE / "corpus").glob("*/*.vf"))
    inline_closers = 0
    for path in corpus:
        toks = vfs.lex(path.read_text(encoding="utf-8"))
        for k, t in enumerate(toks):
            if t.kind == "OPEN" and not toks[k + 1].nl and toks[k + 1].kind != "CLOSE":
                inline_closers += 1
    print(f"single-line blocks in the corpus (en+ru+zh), whose `;;` the inline variant makes optional: {inline_closers}")
    print()
    print("| Mutation | Variant | local | distant | same tree | silent |")
    print("| --- | --- | --- | --- | --- | --- |")
    table = {}
    for path in corpus:
        text = path.read_text(encoding="utf-8")
        for variant in ("strict", "relaxed", "inline"):
            ref = parse_with(variant, text)
            for cls, mutated, line in mutants(text):
                table.setdefault((cls, variant), Counter())[outcome(variant, mutated, line, ref)] += 1
    for (cls, variant), c in table.items():
        print(f"| {cls} | {variant} | {c['local']} | {c['distant']} | {c['same']} | {c['silent']} |")
    # the habit itself, as data
    print()
    for variant in ("strict", "relaxed", "inline"):
        for src in ("config :\n    port: 8080\n;;", "config :\n    port: 8080 ;;\n;;"):
            try:
                tree = parse_with(variant, src)
                print(f"{variant:8} {src!r}: {tree[1][0]}")
            except ParseError as e:
                print(f"{variant:8} {src!r}: error {e}")


if __name__ == "__main__":
    main()
