"""How far from the joint do `distant` gluing errors land? Run: python3 -I distance.py"""
import statistics, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from experiment import LANGS, designs_for, render, mutants  # noqa: E402
from vfp import PUNCT, ParseError, parse  # noqa: E402

for lang in LANGS:
    for design in designs_for(lang):
        if design.name not in ("| ;;", "do end", "тут всё", "开始 结束", "始 终"):
            continue
        dists = []
        for path in sorted((HERE / "corpus" / lang).glob("*.vf")):
            text = render(path.read_text(encoding="utf-8"), design)
            for _, mutated, joint in mutants(text, design):
                line = mutated.count("\n", 0, joint) + 1
                try:
                    parse(mutated, design)
                except ParseError as e:
                    if e.line != line:
                        dists.append(abs(e.line - line))
        if dists:
            print(f"{lang} · {design.name}: {len(dists)} distant, median {statistics.median(dists)} lines, max {max(dists)}")
        else:
            print(f"{lang} · {design.name}: none distant")
