"""Block-delimiter experiment. Run: python3 -I experiment.py

Reads corpus/<lang>/*.vf and probes/<lang>.vf (written in the `| ;;` design),
renders them into word designs, parses every rendering with vfp.py and writes
results.md, results.json and renders/.
"""

from __future__ import annotations

import json
import sys
import unicodedata
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from vfp import PUNCT, Design, Lexer, ParseError, lex, parse, word_design  # noqa: E402

LANGS = ["en", "ru", "zh"]
NATIVE_MODE = {"en": "L", "ru": "R", "zh": "Z"}

EN = word_design("do end", "do", "end")
RU = word_design("тут всё", "тут", "всё")
RU_ALIAS = word_design("тут всё/все", "тут", "всё", closers=("все",))
ZH1 = word_design("开始 结束", "开始", "结束")
ZH2 = word_design("始 终", "始", "终")
ALL_OPEN = ("do", "тут", "开始", "始")
ALL_CLOSE = ("end", "всё", "все", "结束", "终")
NATIVE_PAIR = {"en": ("do", "end"), "ru": ("тут", "всё"), "zh": ("开始", "结束")}


def all_pairs(lang: str) -> Design:
    o, c = NATIVE_PAIR[lang]
    return word_design("all pairs at once", o, c, openers=ALL_OPEN, closers=ALL_CLOSE)


def designs_for(lang: str) -> list[Design]:
    return [PUNCT, EN, RU, RU_ALIAS, ZH1, ZH2, all_pairs(lang)]


SLUG = {
    "| ;;": "punct",
    "do end": "do-end",
    "тут всё": "tut-vsyo",
    "тут всё/все": "tut-vsyo-vse",
    "开始 结束": "kaishi-jieshu",
    "始 终": "shi-zhong",
    "all pairs at once": "all-pairs",
}

# which renderings to save for reading: native pairs, plus `do end` in foreign code
SAVE = {("en", "do end"), ("ru", "тут всё"), ("ru", "do end"), ("zh", "开始 结束"),
        ("zh", "始 终"), ("zh", "do end")}


# -- rendering ---------------------------------------------------------------

def render(src: str, design: Design) -> str:
    if design.punct:
        return src
    lx = Lexer(src, PUNCT)
    toks = lx.run()
    edits = []
    for t in toks:
        if t.kind == "OPEN":
            edits.append((t.start, t.end, design.opener))
        elif t.kind == "CLOSE":
            edits.append((t.start, t.end, design.closer))
    for off in lx.line_comments:
        edits.append((off, off + 1, ";;"))
    out = src
    for a, b, rep in sorted(edits, reverse=True):
        out = out[:a] + rep + out[b:]
    return out


# -- measures ----------------------------------------------------------------

def width(s: str) -> int:
    return sum(2 if unicodedata.east_asian_width(c) in "WF" else 1 for c in s)


RU_LAYOUT_PUNCT = set('!"№;%:?*()_+-=\\/.,')  # Windows ЙЦУКЕН, as remembered


def modes_for(c: str, zh_model: str) -> set[str]:
    """Input modes in which character c can be typed without switching."""
    if c in " \t\n":
        return {"L", "R", "Z"}
    if c.isdigit() and c.isascii():
        return {"L", "R", "Z"}
    if c.isascii() and c.isalpha():
        return {"L"}
    if c.isascii():
        modes = {"L"}
        if c in RU_LAYOUT_PUNCT:
            modes.add("R")
        if zh_model == "half":
            modes.add("Z")
        return modes
    name = unicodedata.name(c, "")
    if "CYRILLIC" in name:
        return {"R"}
    if "CJK" in name or unicodedata.east_asian_width(c) in "WF":
        return {"Z"}
    return {"L"}


def switches(text: str, start: str, zh_model: str = "half") -> int:
    """Minimal number of input-mode switches to type text, starting in `start`."""
    inf = 10**9
    cost = {m: (0 if m == start else 1) for m in "LRZ"}
    for c in text:
        allowed = modes_for(c, zh_model)
        best = min(cost.values())
        new = {}
        for m in "LRZ":
            if m in allowed:
                new[m] = min(cost[m], best + 1)
            else:
                new[m] = inf
        cost = new
    return min(cost.values())


def closer_lines(text: str, design: Design) -> tuple[int, int]:
    """Lines holding only closers (optionally with `)`/`,`), and the longest run of them."""
    count = run = longest = 0
    for line in text.splitlines():
        stripped = line.strip().rstrip("),")
        parts = stripped.split()
        if parts and all(p == design.closer for p in parts):
            count += 1
            run += 1
            longest = max(longest, run)
        else:
            run = 0
    return count, longest


def strip_comments(text: str, design: Design) -> str:
    """Drop line comments so typing measures count code, not prose."""
    lx = Lexer(text, design)
    lx.run()
    out = list(text)
    for off in lx.line_comments:
        j = off
        while j < len(out) and out[j] != "\n":
            out[j] = ""
            j += 1
    return "".join(out)


# -- mutations ---------------------------------------------------------------

def mutants(text: str, design: Design):
    """Yield (class, mutated_text, joint_offset) gluing each delimiter to a neighbour."""
    toks = lex(text, design)
    for k, t in enumerate(toks):
        if t.kind not in ("OPEN", "CLOSE"):
            continue
        prev = toks[k - 1] if k else None
        nxt = toks[k + 1]
        sides = []
        if prev is not None:
            sides.append(("before", prev.end, t.start))
        if nxt.kind != "EOF":
            sides.append(("after", t.end, nxt.start))
        for side, a, b in sides:
            gap = text[a:b]
            if not gap or gap.strip():  # already touching, or a comment sits between
                continue
            cls = f"{'opener' if t.kind == 'OPEN' else 'closer'} {side}"
            yield cls, text[:a] + text[b:], a


def classify(mutated: str, design: Design, joint: int, ref) -> str:
    joint_line = mutated.count("\n", 0, joint) + 1
    try:
        tree = parse(mutated, design)
    except ParseError as e:
        return "local" if e.line == joint_line else "distant"
    return "tolerated" if tree == ref else "silent"


# -- run ---------------------------------------------------------------------

def main():
    examples = defaultdict(lambda: defaultdict(list))
    results = {"programs": {}, "aggregate": {}, "mutations": {}, "probes": {}, "typing": {}}
    corpus = {lang: sorted((HERE / "corpus" / lang).glob("*.vf")) for lang in LANGS}
    for lang in LANGS:
        for design in designs_for(lang):
            agg = Counter()
            mut = defaultdict(Counter)
            name = f"{lang} · {design.name}"
            typing = Counter()
            failures = []
            for path in corpus[lang]:
                src = path.read_text(encoding="utf-8")
                ref = parse(src, PUNCT)
                text = render(src, design)
                if (lang, design.name) in SAVE:
                    out = HERE / "renders" / lang / SLUG[design.name] / path.name
                    out.parent.mkdir(parents=True, exist_ok=True)
                    out.write_text(text, encoding="utf-8")
                key = f"{lang}/{path.stem}/{SLUG[design.name]}"
                try:
                    tree = parse(text, design)
                except ParseError as e:
                    failures.append(f"{path.stem}: {e}")
                    results["programs"][key] = {"ok": False, "error": str(e)}
                    continue
                if tree != ref:
                    failures.append(f"{path.stem}: tree differs from `| ;;`")
                    results["programs"][key] = {"ok": False, "error": "tree differs"}
                    continue
                toks = lex(text, design)
                delims = [t for t in toks if t.kind in ("OPEN", "CLOSE")]
                cl, longest = closer_lines(text, design)
                agg["programs"] += 1
                agg["chars"] += len(text)
                agg["columns"] += width(text)
                agg["delimiters"] += len(delims)
                agg["delimiter_columns"] += sum(width(t.text) for t in delims)
                agg["tokens"] += len(toks) - 1
                agg["closer_lines"] += cl
                agg["longest_closer_run"] = max(agg["longest_closer_run"], longest)
                agg["lines"] += text.count("\n") + 1
                code = strip_comments(text, design)
                start = NATIVE_MODE[lang]
                typing["half"] += switches(code, start, "half")
                typing["full"] += switches(code, start, "full")
                for cls, mutated, joint in mutants(text, design):
                    outcome = classify(mutated, design, joint, ref)
                    mut[cls][outcome] += 1
                    if outcome in ("distant", "silent", "tolerated") and len(examples[name][outcome]) < 6:
                        line = mutated.count("\n", 0, joint)
                        examples[name][outcome].append(f"{path.stem}:{line + 1}: {mutated.splitlines()[line].strip()}")
            agg["failures"] = failures
            results["aggregate"][name] = dict(agg)
            results["mutations"][name] = {k: dict(v) for k, v in mut.items()}
            results["typing"][name] = dict(typing)
            # probes
            src = (HERE / "probes" / f"{lang}.vf").read_text(encoding="utf-8")
            text = render(src, design)
            try:
                tree = parse(text, design)
                status = "ok" if tree == parse(src, PUNCT) else "tree differs"
            except ParseError as e:
                status = f"error {e}"
            results["probes"][name] = status
    results["mutation_examples"] = {k: dict(v) for k, v in examples.items()}
    (HERE / "results.json").write_text(json.dumps(results, ensure_ascii=False, indent=1), encoding="utf-8")
    (HERE / "results.md").write_text(report(results), encoding="utf-8")
    print((HERE / "results.md").read_text(encoding="utf-8"))


def report(r) -> str:
    out = ["# Results (generated by experiment.py)", ""]
    out.append("## Parsing and tree equivalence")
    out.append("")
    out.append("Each rendering must parse and lower to the same tree as the `| ;;` original.")
    out.append("")
    out.append("| Corpus · design | Programs ok | Failures |")
    out.append("| --- | --- | --- |")
    for name, a in r["aggregate"].items():
        fails = "; ".join(a["failures"]) or "—"
        out.append(f"| {name} | {a.get('programs', 0)}/17 | {fails} |")
    out.append("")
    out.append("## Size, width and closer stacking (programs that parsed)")
    out.append("")
    out.append("| Corpus · design | Chars | Columns | Delimiter columns | Share of columns | Closer-only lines | Longest closer run |")
    out.append("| --- | --- | --- | --- | --- | --- | --- |")
    for name, a in r["aggregate"].items():
        if not a.get("programs"):
            continue
        share = 100 * a["delimiter_columns"] / a["columns"]
        out.append(f"| {name} | {a['chars']} | {a['columns']} | {a['delimiter_columns']} | {share:.1f}% | "
                   f"{a['closer_lines']} | {a['longest_closer_run']} |")
    out.append("")
    out.append("## Keyboard input-mode switches (code only, comments removed)")
    out.append("")
    out.append("Minimal switches between US Latin, Windows Russian and Chinese IME modes, starting in the corpus's")
    out.append("native mode. `half`: the Chinese IME emits ASCII punctuation; `full`: it emits full-width")
    out.append("punctuation, so every ASCII punctuation mark needs Latin mode. Only the zh rows differ.")
    out.append("")
    out.append("| Corpus · design | Switches (half) | Switches (full) |")
    out.append("| --- | --- | --- |")
    for name, t in r["typing"].items():
        if t:
            out.append(f"| {name} | {t['half']} | {t['full']} |")
    out.append("")
    out.append("## Gluing mutations")
    out.append("")
    out.append("Each delimiter is glued to its neighbour by deleting the whitespace between them.")
    out.append("`local`: an error on the joint's line; `distant`: an error elsewhere; `tolerated`: the mutant")
    out.append("parses to the original tree; `silent`: the mutant parses to a different tree.")
    out.append("")
    out.append("| Corpus · design | Mutation | local | distant | tolerated | silent |")
    out.append("| --- | --- | --- | --- | --- | --- |")
    for name, m in r["mutations"].items():
        for cls in ("opener before", "opener after", "closer before", "closer after"):
            c = m.get(cls, {})
            if c:
                out.append(f"| {name} | {cls} | {c.get('local', 0)} | {c.get('distant', 0)} | {c.get('tolerated', 0)} | {c.get('silent', 0)} |")
    out.append("")
    out.append("## Collision probes")
    out.append("")
    out.append("| Corpus · design | Probe |")
    out.append("| --- | --- |")
    for name, status in r["probes"].items():
        out.append(f"| {name} | {status} |")
    out.append("")
    return "\n".join(out)


if __name__ == "__main__":
    main()
