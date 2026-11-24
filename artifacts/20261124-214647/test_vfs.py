"""Sanity checks for vfs.py. Run: python3 -I test_vfs.py"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from vfs import ParseError, parse  # noqa: E402

failures = 0


def ok(src):
    global failures
    try:
        return parse(src)
    except ParseError as e:
        failures += 1
        print(f"FAIL (expected parse) {src!r}: {e}")


def bad(src, line=None):
    global failures
    try:
        parse(src)
    except ParseError as e:
        if line is not None and e.line != line:
            failures += 1
            print(f"FAIL (error on line {e.line}, expected {line}) {src!r}: {e}")
        return
    failures += 1
    print(f"FAIL (expected error) {src!r}")


def same(a, b):
    global failures
    ta, tb = ok(a), ok(b)
    if ta is not None and tb is not None and ta != tb:
        failures += 1
        print(f"FAIL (trees differ) {a!r} vs {b!r}\n  {ta}\n  {tb}")


# symbols
same("~fork", "{:fork}")
same("\\[x] ?[~fork] x", "\\[x] ?[{:fork}] x")
same("f ~x", "f(~x)")
same("f ~x(1) ~y(2)", "f(~x(1))(~y(2))")
same("f : b ;;", "f((: b ;;))")
same("f(x) : b ;;", "f(x)((: b ;;))")
same("defer ~do: b ;;", "defer(~do)((: b ;;))")
same("if(a): x ;; ~else: y ;;", "if(a)((: x ;;))(~else)((: y ;;))")
ok("~else-if(c)")
ok("x = ~")
ok("~[x, y]")
ok("~{00 ff}")

# blocks and adjacency
ok("if(x):\n    y\n;;")
ok("if(x) :\n    y\n;;")
ok("~else: y ;;")
ok("test{name}: run() ;;")
ok("fn ~f[a] ~ret(i32): a ;;")
ok("noop ~do: ;;")
ok("noop : ;;")
ok("[: a ;;, : b ;;]")
ok("(: x ;;)")
ok("f(: y ;;)")
bad("port: 8080", 1)
bad("defer: close(f) ;;", 1)
ok("defer : close(f) ;;")
ok("defer ~do: close(f) ;;")
bad("if(x): y;;", 1)
bad("if(x):y ;;", 1)
bad("if(x): y ;;;;", 1)
bad("if(x): y ;;~else: z ;;", 1)

# operators: `|` and `:`-runs
ok("a | b")
ok("[h | t]")
ok("a |- b")
ok("x :: T")
ok("x := 1")

# ASI: a newline after a closer ends the head
t = ok("if(a):\n  x\n;;\n~else:\n  y\n;;")
assert t is not None and len(t[1]) == 2, t

# v0 forms remain
ok("size = \\[self, ~[l, r]]\n  ?[~all, ~fork] 1\n  ?[~all, ~leaf] 0")

print("ok" if not failures else f"{failures} failure(s)")
sys.exit(1 if failures else 0)
