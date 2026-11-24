"""Convert the 24.11.26 `| ;;` corpus to symbols and colon blocks.

Run: python3 -I convert.py

Uses the earlier parser (artifacts/20261124-123200/vfp.py) to find each
label's payload, then rewrites:

* `name: |` (block payload)                -> `~name:`
* `name: [..]`, `name: (..)`, `name: {..}`  -> `~name[..]`, `~name(..)`, `~name{..}`
* `name: payload` (anything else)          -> `~name(payload)`
* `|` opening any other block              -> `:`, glued after `)`, `]` or `}`

v0 lambda callbacks are left for hand rewriting; corpus/ is the hand-edited result.
"""

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
OLD = HERE.parent / "20261124-123200"
sys.path.insert(0, str(OLD))

import vfp  # noqa: E402


class SpanParser(vfp.Parser):
    def __init__(self, toks):
        super().__init__(toks)
        self.labels = []  # (name_index or None, colon_index, payload_start, payload_end, is_block)

    def labeled(self, layout):
        name_k = None
        if self.peek().kind != "COLON":
            name_k = self.k
            self.next()
        colon_k = self.k
        self.expect("COLON")
        start = self.k
        is_block = self.peek().kind == "OPEN"
        payload = self.block() if is_block else self.argument(layout)
        self.labels.append((name_k, colon_k, start, self.k, is_block))
        return ("label", None if name_k is None else self.t[name_k].text, payload)


BRACKETS = {"LBRACK": "RBRACK", "LPAREN": "RPAREN"}


def single_group(toks, start, end):
    """Is toks[start:end] exactly one bracketed group or one string?"""
    first = toks[start]
    if first.kind == "STR":
        return end == start + 1
    if first.kind not in BRACKETS:
        return False
    depth = 0
    for k in range(start, end):
        kind = toks[k].kind
        if kind in ("LBRACK", "LPAREN", "NYAD", "LAMBDA", "ROW", "BINDER"):
            depth += 1
        elif kind in ("RBRACK", "RPAREN"):
            depth -= 1
            if depth == 0:
                return k == end - 1
    return False


def convert(src: str) -> str:
    toks = vfp.lex(src, vfp.PUNCT)
    p = SpanParser(toks)
    p.source()
    edits = []  # (start, end, replacement)
    label_openers = set()
    wrapped_ends = set()  # token indices that gain a closing `)`
    for name_k, colon_k, start, end, is_block in p.labels:
        if name_k is None:
            raise ValueError("omitted label has no symbol form")
        name = toks[name_k]
        if is_block:
            opener = toks[start]
            label_openers.add(start)
            edits.append((name.start, opener.end, f"~{name.text}:"))
        elif single_group(toks, start, end):
            edits.append((name.start, toks[start].start, f"~{name.text}"))
        else:
            edits.append((name.start, toks[start].start, f"~{name.text}("))
            edits.append((toks[end - 1].end, toks[end - 1].end, ")"))
            wrapped_ends.add(end - 1)
    for k, t in enumerate(toks):
        if t.kind != "OPEN" or k in label_openers:
            continue
        prev = toks[k - 1] if k else None
        gap = src[prev.end : t.start] if prev else ""
        closes = prev is not None and (prev.kind in ("RPAREN", "RBRACK", "STR") or k - 1 in wrapped_ends)
        if closes and gap and not gap.strip(" "):
            edits.append((prev.end, t.end, ":"))
        else:
            edits.append((t.start, t.end, ":"))
    out = src
    for a, b, rep in sorted(edits, key=lambda e: (e[0], e[1]), reverse=True):
        out = out[:a] + rep + out[b:]
    return out


def main():
    for lang in ("en", "ru", "zh"):
        for path in sorted((OLD / "corpus" / lang).glob("*.vf")):
            out = HERE / "converted" / lang / path.name
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_text(convert(path.read_text(encoding="utf-8")), encoding="utf-8")
        probe = OLD / "probes" / f"{lang}.vf"
        out = HERE / "converted" / "probes" / f"{lang}.vf"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(convert(probe.read_text(encoding="utf-8")), encoding="utf-8")


if __name__ == "__main__":
    main()
