"""Sanity checks for the throwaway parser. Run: python3 -I test_vfp.py"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from vfp import PUNCT, ParseError, parse, word_design  # noqa: E402

DO = word_design("do end", "do", "end")
RU = word_design("тут всё", "тут", "всё")
ZH = word_design("开始 结束", "开始", "结束")

failures = 0


def ok(src, design=PUNCT):
    global failures
    try:
        return parse(src, design)
    except ParseError as e:
        failures += 1
        print(f"FAIL (expected parse) [{design.name}] {src!r}: {e}")


def bad(src, design=PUNCT, line=None):
    global failures
    try:
        parse(src, design)
    except ParseError as e:
        if line is not None and e.line != line:
            failures += 1
            print(f"FAIL (error on line {e.line}, expected {line}) {src!r}: {e}")
        return
    failures += 1
    print(f"FAIL (expected error) [{design.name}] {src!r}")


def same(a, da, b, db):
    global failures
    ta, tb = ok(a, da), ok(b, db)
    if ta is not None and tb is not None and ta != tb:
        failures += 1
        print(f"FAIL (trees differ)\n  {a!r}\n  {b!r}")


# basic shapes
ok("f(x)[y]{z}")
ok("x :: y")
ok("x + -y")
bad("x -y")
bad("f(x)-y")
ok("v[.len] = v[.len] + 1")
ok("[a: 1, b: 2]")
ok("f x: 1 y: 2")
ok("\\[x] x * x")
bad("\\[] x")
ok("=[a = 1, b = a + 1] b")
ok("\\[~[l, r]]\n  ?[{:fork}] l\n  ?[{:all}] ~")

# blocks
ok("noop | ;;")
ok("on_start | init() ;;")
ok("if(a > b) |\n    a = b\n;;")
ok("if(a) |\n    x\n;; else: |\n    y\n;;")
ok("for_each(xs, \\[x] |\n    print(x)\n;;)")
ok("[| a ;;, | b ;;]")
ok("(| x ;;)")
ok("if(ready) | #| note |# go() ;;")
ok("x = 1 # comment")
ok("[1, #;old(2) 3]")

# adjacency
bad("x;;")
bad("noop |;;")
bad("if(x)| y ;;")
bad("if(x) |y ;;")
bad("if(x) | y ;;;;")
bad("if(x) | if(y) | z ;;;; ;;")
bad("f(x) | y ;;z")
ok("f(| y ;;)")

# ASI: a newline after a closer ends the head
t = ok("if(a) |\n  x\n;;\nelse: |\n  y\n;;")
assert t is not None and len(t[1]) == 2, t

# word designs lower identically
src = "if(a > b) |\n    a = b\n;; else: |\n    c\n;;"
same(src, PUNCT, src.replace("|", "do").replace(";;", "end"), DO)
same(src, PUNCT, src.replace("|", "тут").replace(";;", "всё"), RU)
same(src, PUNCT, src.replace("|", "开始").replace(";;", "结束"), ZH)

# word designs: whole tokens; label words stay labels
ok("прочитать_всё(файл)", RU)
ok("f do: 1", DO)
ok("x ;; a comment", DO)
bad("x end", DO)

print("ok" if not failures else f"{failures} failure(s)")
sys.exit(1 if failures else 0)
