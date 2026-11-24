"""Evidence for symbols and colon blocks. Run: python3 -I experiment.py

Reads corpus/<lang>/*.vf and probes/*.vf, writes results.md and results.json.
"""

from __future__ import annotations

import json
import sys
import unicodedata
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
OLD = HERE.parent / "20261124-123200"
sys.path.insert(0, str(HERE))

from vfs import ParseError, lex, parse  # noqa: E402

LANGS = ["en", "ru", "zh"]


def width(s: str) -> int:
    return sum(2 if unicodedata.east_asian_width(c) in "WF" else 1 for c in s)


def shape(tree):
    """Erase names, strings, and numbers so programs in different languages compare."""
    if isinstance(tree, tuple):
        if tree and tree[0] in ("id", "str", "int", "bytes"):
            return (tree[0],)
        return tuple(shape(x) for x in tree)
    if isinstance(tree, list):
        return [shape(x) for x in tree]
    if isinstance(tree, str):
        return "_"
    return tree


def outcome(mutated: str, joint_line: int, ref) -> tuple[str, int | None]:
    try:
        tree = parse(mutated)
    except ParseError as e:
        return ("local" if e.line == joint_line else "distant"), e.line
    return ("tolerated" if tree == ref else "silent"), None


def line_of(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def matching(toks, k):
    depth = 0
    for j in range(k, len(toks)):
        if toks[j].kind in ("LPAREN", "LBRACK", "NYAD", "LAMBDA", "ROW", "BINDER"):
            depth += 1
        elif toks[j].kind in ("RPAREN", "RBRACK"):
            depth -= 1
            if depth == 0:
                return j
    raise ValueError("unbalanced")


def mutations(text: str):
    """Yield (class, mutated text, joint offset)."""
    toks = lex(text)
    for k, t in enumerate(toks):
        if t.kind == "EOF":
            break
        prev = toks[k - 1] if k else None
        nxt = toks[k + 1]
        # gluing a delimiter to a neighbour
        if t.kind in ("OPEN", "CLOSE"):
            for side, a, b in (("before", prev.end if prev else None, t.start), ("after", t.end, nxt.start)):
                if a is None or (side == "after" and nxt.kind == "EOF"):
                    continue
                gap = text[a:b]
                if gap and not gap.strip():
                    cls = f"glue {'opener' if t.kind == 'OPEN' else 'closer'} {side}"
                    yield cls, text[:a] + text[b:], a
        if t.kind != "SYMBOL":
            continue
        name = t.text[1:]
        # forgetting the sigil
        cls = "drop ~ before opener" if nxt.kind == "OPEN" and not nxt.ws else "drop ~ elsewhere"
        yield cls, text[: t.start] + text[t.start + 1 :], t.start
        # writing a payload as an old label or YAML key: `~ret(bool)` -> `ret: bool`
        if nxt.kind == "LPAREN" and not nxt.ws:
            close = matching(toks, k + 1)
            inner = text[nxt.end : toks[close].start]
            if "\n" not in inner:
                mutated = text[: t.start] + f"{name}: {inner}" + text[toks[close].end :]
                yield "old label habit", mutated, t.start
        # an alternative on its own line: `;; ~else:` -> `;;\n~else:`
        gap = text[prev.end : t.start] if prev is not None else ""
        if prev is not None and prev.kind == "CLOSE" and nxt.kind == "OPEN" and gap and not gap.strip(" "):
            line_start = text.rfind("\n", 0, prev.start) + 1
            line = text[line_start:]
            indent = line[: len(line) - len(line.lstrip(" "))]
            mutated = text[: prev.end] + "\n" + indent + text[t.start :]
            yield "alternative on next line", mutated, t.start


def main():
    results = {"parse": {}, "shape": {}, "size": {}, "mutations": {}, "examples": {}, "openers": {}}
    corpus = {lang: sorted((HERE / "corpus" / lang).glob("*.vf")) for lang in LANGS}
    shapes = defaultdict(dict)
    for lang in LANGS:
        size = Counter()
        mut = defaultdict(Counter)
        examples = defaultdict(list)
        openers = Counter()
        for path in corpus[lang]:
            text = path.read_text(encoding="utf-8")
            try:
                ref = parse(text)
            except ParseError as e:
                results["parse"][f"{lang}/{path.stem}"] = str(e)
                continue
            results["parse"][f"{lang}/{path.stem}"] = "ok"
            shapes[path.stem][lang] = shape(ref)
            toks = lex(text)
            old = (OLD / "corpus" / lang / path.name).read_text(encoding="utf-8")
            size["chars"] += len(text)
            size["columns"] += width(text)
            size["old_chars"] += len(old)
            size["old_columns"] += width(old)
            size["symbols"] += sum(t.kind == "SYMBOL" for t in toks)
            size["blocks"] += sum(t.kind == "OPEN" for t in toks)
            for k, t in enumerate(toks):
                if t.kind == "OPEN":
                    prev = toks[k - 1] if k else None
                    if prev is None or t.nl or prev.kind in ("SEMI",):
                        openers["statement start"] += 1
                    elif not t.ws:
                        openers[f"glued after {prev.kind}"] += 1
                    else:
                        openers[f"spaced after {prev.kind}"] += 1
            for cls, mutated, joint in mutations(text):
                kind, err_line = outcome(mutated, line_of(mutated, joint), ref)
                mut[cls][kind] += 1
                if kind != "local" and len(examples[f"{cls} / {kind}"]) < 4:
                    ln = line_of(mutated, joint)
                    snippet = mutated.splitlines()[ln - 1].strip()
                    where = f", error on line {err_line}" if err_line else ""
                    examples[f"{cls} / {kind}"].append(f"{path.stem}:{ln}: {snippet}{where}")
        results["size"][lang] = dict(size)
        results["mutations"][lang] = {k: dict(v) for k, v in mut.items()}
        results["examples"][lang] = dict(examples)
        results["openers"][lang] = dict(openers)
    for stem, by_lang in shapes.items():
        results["shape"][stem] = "same" if len({repr(v) for v in by_lang.values()}) == 1 and len(by_lang) == 3 else \
            "differs: " + ", ".join(sorted(by_lang))
    for path in sorted((HERE / "probes").glob("*.vf")):
        try:
            parse(path.read_text(encoding="utf-8"))
            results["parse"][f"probes/{path.stem}"] = "ok"
        except ParseError as e:
            results["parse"][f"probes/{path.stem}"] = str(e)
    (HERE / "results.json").write_text(json.dumps(results, ensure_ascii=False, indent=1), encoding="utf-8")
    (HERE / "results.md").write_text(report(results), encoding="utf-8")
    print((HERE / "results.md").read_text(encoding="utf-8"))


def report(r) -> str:
    out = ["# Results (generated by experiment.py)", ""]
    bad = {k: v for k, v in r["parse"].items() if v != "ok"}
    out += [f"Parsed: {len(r['parse']) - len(bad)}/{len(r['parse'])} files (corpus and probes).", ""]
    for k, v in bad.items():
        out.append(f"- {k}: {v}")
    diff = {k: v for k, v in r["shape"].items() if v != "same"}
    out += [f"Same tree shape in en, ru, zh once names are erased: {len(r['shape']) - len(diff)}/{len(r['shape'])} programs.", ""]
    for k, v in diff.items():
        out.append(f"- {k}: {v}")
    out += ["", "## Size against the `| ;;` corpus with labels", "",
            "| Corpus | Chars (old → new) | Columns (old → new) | Symbols | Blocks |", "| --- | --- | --- | --- | --- |"]
    for lang, s in r["size"].items():
        out.append(f"| {lang} | {s['old_chars']} → {s['chars']} | {s['old_columns']} → {s['columns']} | {s['symbols']} | {s['blocks']} |")
    out += ["", "## Where openers sit", "", "| Corpus | Position | Count |", "| --- | --- | --- |"]
    for lang, o in r["openers"].items():
        for pos, n in sorted(o.items()):
            out.append(f"| {lang} | {pos} | {n} |")
    out += ["", "## Mutations", "",
            "`local`: an error on the mutated line; `distant`: an error elsewhere; `tolerated`: the original tree;",
            "`silent`: parses to a different tree.", "",
            "| Corpus | Mutation | local | distant | tolerated | silent |", "| --- | --- | --- | --- | --- | --- |"]
    order = ["glue opener before", "glue opener after", "glue closer before", "glue closer after",
             "drop ~ before opener", "drop ~ elsewhere", "old label habit", "alternative on next line"]
    for lang, m in r["mutations"].items():
        for cls in order:
            c = m.get(cls)
            if c:
                out.append(f"| {lang} | {cls} | {c.get('local', 0)} | {c.get('distant', 0)} | {c.get('tolerated', 0)} | {c.get('silent', 0)} |")
    out += ["", "## Non-local examples", ""]
    for lang, ex in r["examples"].items():
        for k, v in ex.items():
            out.append(f"- {lang} · {k}:")
            out += [f"  - `{x}`" for x in v]
    out.append("")
    return "\n".join(out)


if __name__ == "__main__":
    main()
