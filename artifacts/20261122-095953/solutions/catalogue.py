"""The term-interaction catalogue: programs run on every variant. Throwaway.

Templates use the `$name` notation of ../case-lambdas/programs.py, plus two placeholders for
computed names:

    %x   a declaration slot that should declare whatever `x` holds
    ^x   a usage that should use whatever `x` holds

Forms of a template:
    plain  %x and ^x written as $x                 (counting rule: a repeat is a usage)
    role   %x as ~[{:decl}, $x], ^x as ~[{:use}, $x] (solution 1's explicit roles)
    data   hand-written: the header or clause list is built as data first (today's idiom)
"""

import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "case-lambdas"))

from compile_v0 import FIX, HEAD, TAIL, app, case, lam, let, lst, pair  # noqa: E402

ID = lam([["$id-i"]], case(["all"], "$id-i"))
LEN = lam([["$rec"], ["$h", "$t"]],
    case(["all", "leaf"], "~"),
    case(["all", "fork"], let([("$n", app("$rec", "$t"))], pair("{s}", "$n"))),
    case(["all", "fruit"], "{not-a-list}"))


def uniq(name):
    return app("$unique", lst("{" + name + "}"))


def numeral(n):
    return "[" + ", ".join(["{s}"] * n) + "]" if n else "~"


PLACEHOLDER = re.compile(r"([%^])([A-Za-z][A-Za-z0-9-]*(?:\.[0-9]+)?)")


def form(template, which):
    def sub(match):
        mark, name = match.groups()
        if which == "plain":
            return "$" + name
        return ("~[{:decl}, $" if mark == "%" else "~[{:use}, $") + name + "]"
    return PLACEHOLDER.sub(sub, template)


def forms(template, data=None):
    out = {"plain": form(template, "plain")}
    if PLACEHOLDER.search(template):
        out["role"] = form(template, "role")
    if data is not None:
        out["data"] = data
    return out


# Checks receive (machine, result) and return (ok, shown); ok None means report only.

def equals(text):
    def check(machine, result, display):
        shown = display(result)
        return shown == text, shown
    check.intended = text
    return check


def hole(name):
    def check(machine, result, display):
        from variants import plain
        tree = plain(result)
        if machine.c.roles and isinstance(tree, type(tree)) and machine.role(tree, b":use") is not None:
            tree = tree.right
        ok = machine.is_identifier(tree) and machine.name_of(tree) == name.encode()
        return ok, ("hole " + name if ok else display(result))
    check.intended = "hole " + name
    return check


def error(fragment):
    def check(machine, result, display):
        return False, display(result)
    check.intended = "error: " + fragment
    check.error = fragment
    return check


def report(machine, result, display):
    return None, display(result)


report.intended = "(report)"


def unset_fed_back(machine, result, display):
    from variants import plain
    tree = plain(result)
    shown = display(result)
    if not isinstance(tree, type(tree)) or not hasattr(tree, "left"):
        return False, shown
    left = tree.left
    if machine.c.roles and machine.role(left, b":use") is not None:
        left = left.right
    ok = machine.is_identifier(left) and tree.right == b"p"
    return ok, shown


unset_fed_back.intended = "~[hole l, {p}]"


def unset_mentioned(machine, result, display):
    from variants import plain
    tree = plain(result)
    right = getattr(tree, "right", None)
    if machine.c.roles and right is not None and machine.role(right, b":use") is not None:
        right = right.right
    ok = getattr(tree, "left", None) == b"u" and machine.is_identifier(right)
    return ok, display(result)


unset_mentioned.intended = "~[{u}, hole l]"


def count_is(n):
    def check(machine, result, display):
        from variants import plain
        tree, total = plain(result), 0
        while tree is not None and getattr(tree, "left", None) == b"s":
            tree, total = tree.right, total + 1
        return (tree is None and total == n), f"{total} s"
    check.intended = f"{n} s"
    return check


def second_is(text):
    def check(machine, result, display):
        from variants import plain
        tree = plain(result)
        try:
            second = tree.right.left
        except AttributeError:
            return False, display(result)
        return display(second) == text, display(result)
    check.intended = f"[_, {text}]"
    return check


def pair_counts(a, b):
    def check(machine, result, display):
        from variants import plain
        tree = plain(result)

        def count(t):
            total = 0
            while t is not None and getattr(t, "left", None) == b"s":
                t, total = t.right, total + 1
            return total

        try:
            got = (count(tree.left), count(tree.right.left))
        except AttributeError:
            return False, display(result)
        return got == (a, b), f"{got[0]} s, {got[1]} s"
    check.intended = f"{a} s, {b} s"
    return check


# ---------------------------------------------------------------- programs

C2_PLAIN = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL),
    ("$xh", app("$head", lst("$x"))),
    ("$w", pair("{code}", let([("%x", "{9}")], "^xh"))),
    ("$r", app("$tail", "$w")),
], "$r")
C2_DATA = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL),
    ("$xh", app("$head", lst("$x"))),
    ("$cl", lst(pair("$x", "{9}"))),
    ("$w", pair("{code}", lst("[{:binder}]", "$cl", "$xh"))),
    ("$r", app("$tail", "$w")),
], "$r")

C3_PLAIN = let([
    ("$x", uniq("x")), ("$id", ID), ("$tail", TAIL),
    ("$xh", app("$id", "$x")),
    ("$w", pair("{code}", let([("%x", "{9}")], "^xh"))),
    ("$r", app("$tail", "$w")),
], "$r")
C3_DATA = let([
    ("$x", uniq("x")), ("$id", ID), ("$tail", TAIL),
    ("$xh", app("$id", "$x")),
    ("$cl", lst(pair("$x", "{9}"))),
    ("$w", pair("{code}", lst("[{:binder}]", "$cl", "$xh"))),
    ("$r", app("$tail", "$w")),
], "$r")

C4_PLAIN = let([
    ("$x", uniq("x")), ("$y", uniq("y")), ("$head", HEAD), ("$tail", TAIL),
    ("$yh", app("$head", lst("$y"))),
    ("$w", pair("{code}", let([("%x", "{9}")], "^yh"))),
    ("$r", app("$tail", "$w")),
], "$r")
C4_DATA = let([
    ("$x", uniq("x")), ("$y", uniq("y")), ("$head", HEAD), ("$tail", TAIL),
    ("$yh", app("$head", lst("$y"))),
    ("$cl", lst(pair("$x", "{9}"))),
    ("$w", pair("{code}", lst("[{:binder}]", "$cl", "$yh"))),
    ("$r", app("$tail", "$w")),
], "$r")

C5 = let([("$x", uniq("x")), ("$head", HEAD), ("$xh", app("$head", lst("$x"))), ("$z", "^xh")], "$z")

C6_PLAIN = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL),
    ("$xh", app("$head", lst("$x"))),
    ("$h", lam([["$z"]], case(["all"], pair("^xh", "$z")))),
    ("$w", pair("{code}", let([("%x", "{9}"), ("$o", app("$h", "{1}"))], "$o"))),
    ("$r", app("$tail", "$w")),
], "$r")
C6_DATA = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL),
    ("$xh", app("$head", lst("$x"))),
    ("$h", lam([["$z"]], case(["all"], pair("$xh", "$z")))),
    ("$cl", lst(pair("$x", "{9}"), pair("$o", app("$h", "{1}")))),
    ("$w", pair("{code}", lst("[{:binder}]", "$cl", "$o"))),
    ("$r", app("$tail", "$w")),
], "$r")

D1_PLAIN = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$xh", app("$head", lst("$x"))),
    ("$mk", lam([["$d"], ["$b"]], case(["all", "all"], lam([["%d"]], case(["all"], "^b"))))),
    ("$m1", app("$mk", "$xh")), ("$f", app("$m1", "$xh")), ("$r", app("$f", "{hello}")),
], "$r")
D1_DATA = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$xh", app("$head", lst("$x"))),
    ("$mk", lam([["$d"], ["$b"]], case(["all", "all"], let([("$hdr", lst(lst("$d")))],
        lst("[{:lambda}]", "~", "$hdr", case(["all"], "$b")))))),
    ("$m1", app("$mk", "$xh")), ("$f", app("$m1", "$xh")), ("$r", app("$f", "{hello}")),
], "$r")

D2_PLAIN = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL), ("$xh", app("$head", lst("$x"))),
    ("$mkb", lam([["$d"], ["$u"]], case(["all", "all"], pair("{code}", let([("%d", "{4}")], "^u"))))),
    ("$m", app("$mkb", "$xh")), ("$w", app("$m", "$xh")), ("$r", app("$tail", "$w")),
], "$r")
D2_DATA = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL), ("$xh", app("$head", lst("$x"))),
    ("$mkb", lam([["$d"], ["$u"]], case(["all", "all"], let([("$cl", lst(pair("$d", "{4}")))],
        pair("{code}", lst("[{:binder}]", "$cl", "$u")))))),
    ("$m", app("$mkb", "$xh")), ("$w", app("$m", "$xh")), ("$r", app("$tail", "$w")),
], "$r")

D4 = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$xh", app("$head", lst("$x"))),
    ("$f", lam([["%xh"]], case(["all"], lam([["%xh"]], case(["all"], "^xh"))))),
    ("$f1", app("$f", "{1}")), ("$r", app("$f1", "{2}")),
], "$r")

H2_PLAIN = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL), ("$xh", app("$head", lst("$x"))),
    ("$a", app("$new", lst("$xh"))),
    ("$w", pair("{code}", let([("%x", "{9}"), ("$y", app("$get", lst("$a")))], "$y"))),
    ("$r", app("$tail", "$w")),
], "$r")
H2_DATA = let([
    ("$x", uniq("x")), ("$head", HEAD), ("$tail", TAIL), ("$xh", app("$head", lst("$x"))),
    ("$a", app("$new", lst("$xh"))),
    ("$cl", lst(pair("$x", "{9}"), pair("$y", app("$get", lst("$a"))))),
    ("$w", pair("{code}", lst("[{:binder}]", "$cl", "$y"))),
    ("$r", app("$tail", "$w")),
], "$r")

COMPOSE_H = let([
    ("$id", ID),
    ("$g", lam([["$f"]], case(["all"], lam([["$b"]], case(["all"], let([("$r0", app("$f", "$b"))], "$r0")))))),
    ("$g1", app("$g", "$id")), ("$h", app("$g", "$g1")),
], "$h")

MUTUAL = let([
    ("$ev", lam([["$e1"], ["$o1"], ["$h1", "$t1"]],
        case(["all", "all", "leaf"], "{t}"),
        case(["all", "all", "fork"], let([("$p1", app("$o1", "$e1")), ("$q1", app("$p1", "$o1")),
                                          ("$r1", app("$q1", "$t1"))], "$r1")))),
    ("$od", lam([["$e2"], ["$o2"], ["$h2", "$t2"]],
        case(["all", "all", "leaf"], "{f}"),
        case(["all", "all", "fork"], let([("$p2", app("$e2", "$e2")), ("$q2", app("$p2", "$o2")),
                                          ("$r2", app("$q2", "$t2"))], "$r2")))),
    ("$a", app("$ev", "$ev")), ("$b", app("$a", "$od")), ("$r", app("$b", numeral(3))),
], "$r")

FIB_FACT = None  # filled from programs.py below


ENTRIES = [
    # A. lexical basics
    ("A1", "parameter bound by application", equals("{1}"),
     forms(let([("$r", app(lam([["$x"]], case(["all"], "$x")), "{1}"))], "$r"))),
    ("A2", "binder clauses in order", equals("~[{1}, {1}]"),
     forms(let([("$a", "{1}"), ("$b", pair("$a", "$a"))], "$b"))),
    ("A3", "nested lambdas, distinct identities", equals("~[{1}, {2}]"),
     forms(let([("$f", lam([["$x"]], case(["all"], lam([["$y"]], case(["all"], pair("$x", "$y")))))),
                ("$g", app("$f", "{1}")), ("$r", app("$g", "{2}"))], "$r"))),
    ("A4", "same spelling, different identities", equals("~[{1}, {2}]"),
     forms(let([("$f", lam([["$x"]], case(["all"], lam([["$x.2"]], case(["all"], pair("$x", "$x.2")))))),
                ("$g", app("$f", "{1}")), ("$r", app("$g", "{2}"))], "$r"))),
    ("A5", "closure keeps its captured value", equals("{1}"),
     forms(let([("$f", lam([["$x"]], case(["all"], lam([["$y"]], case(["all"], "$x"))))),
                ("$f1", app("$f", "{1}")), ("$r", app("$f1", "{2}"))], "$r"))),
    # B. values inserted into scopes
    ("B1", "closed lambda passed and called", equals("{3}"),
     forms(let([("$id", ID), ("$ap", lam([["$f"], ["$v"]], case(["all", "all"], let([("$r0", app("$f", "$v"))], "$r0")))),
                ("$p", app("$ap", "$id")), ("$r", app("$p", "{3}"))], "$r"))),
    ("B2", "compose: inserted lambda redeclares the same identity", equals("{7}"),
     forms(let([("$id", ID),
                ("$g", lam([["$f"]], case(["all"], lam([["$b"]], case(["all"], let([("$r0", app("$f", "$b"))], "$r0")))))),
                ("$g1", app("$g", "$id")), ("$h", app("$g", "$g1")), ("$r", app("$h", "{7}"))], "$r"))),
    ("B3", "twice: one inserted lambda used twice", equals("~[{w}, ~[{w}, {0}]]"),
     forms(let([("$wrap", lam([["$q"]], case(["all"], pair("{w}", "$q")))),
                ("$twice", lam([["$f"]], case(["all"], lam([["$b"]], case(["all"], let([("$r1", app("$f", "$b")), ("$r2", app("$f", "$r1"))], "$r2")))))),
                ("$t", app("$twice", "$wrap")), ("$r", app("$t", "{0}"))], "$r"))),
    ("B4", "fix: self-application", equals("[{s}, {s}, {s}]"),
     forms(let([("$fix", FIX), ("$len", LEN), ("$length", app("$fix", "$len")),
                ("$r", app("$length", "[{a}, {b}, {c}]"))], "$r"))),
    ("B5", "partial application holds a lambda, later args reach it", equals("{5}"),
     forms(let([("$id", ID), ("$head", HEAD), ("$mk", lam([["$a"], ["$b"]], case(["all", "all"], pair("$a", "$b")))),
                ("$p", app("$mk", "$id")), ("$q", app("$p", "{2}")), ("$h", app("$head", "$q")),
                ("$r", app("$h", "{5}"))], "$r"))),
    ("B6", "lambda applied to itself", equals("{1}"),
     forms(let([("$k", lam([["$x"]], case(["all"], "$x"))), ("$kk", app("$k", "$k")), ("$r", app("$kk", "{1}"))], "$r"))),
    # C. holes
    ("C1", "unique identifier held as data", hole("x"), forms(let([("$x", uniq("x"))], "$x"))),
    ("C2", "binder built around a hole binds it", equals("{9}"), forms(C2_PLAIN, C2_DATA)),
    ("C3", "hole passed through a function, then bound", equals("{9}"), forms(C3_PLAIN, C3_DATA)),
    ("C4", "a different hole escapes the binder", hole("y"), forms(C4_PLAIN, C4_DATA)),
    ("C5", "running a hole", error("unbound"), forms(C5)),
    ("C6", "hole inside a function value, bound where the value is used", equals("~[{9}, {1}]"),
     forms(C6_PLAIN, C6_DATA)),
    # D. computed declarations
    ("D1", "mk: header declares a computed identifier", equals("{hello}"), forms(D1_PLAIN, D1_DATA)),
    ("D2", "binder clause declares a computed identifier", equals("{4}"), forms(D2_PLAIN, D2_DATA)),
    ("D3", "reflective re-entry", second_is("{z}"), None),
    ("D4", "two generated declarations of one identity, nested", report, forms(D4)),
    # E. inspection
    ("E1", "inspect a partial application", report,
     forms(let([("$f", lam([["$a"], ["$b"]], case(["all", "all"], pair("$a", "$b")))), ("$p", app("$f", "{1}"))], "$p"))),
    ("E2", "take a lambda apart and rebuild it", equals("~[{r}, {1}]"),
     forms(let([("$head", HEAD), ("$tail", TAIL), ("$f", lam([["$a"]], case(["all"], pair("{r}", "$a")))),
                ("$t1", app("$tail", "$f")), ("$t2", app("$tail", "$t1")), ("$hdr", app("$head", "$t2")),
                ("$t3", app("$tail", "$t2")), ("$cs", app("$head", "$t3")),
                ("$f2", lst("[{:lambda}]", "~", "$hdr", "$cs")), ("$r", app("$f2", "{1}"))], "$r"))),
    # F. unset names
    ("F1", "arm mentions its unset name", unset_mentioned,
     forms(let([("$s", lam([["$l", "$r"]], case(["leaf"], pair("{u}", "$l")), case(["all"], "{x}"))),
                ("$o", app("$s", "~"))], "$o"))),
    ("F2", "unset name fed back into the same lambda", unset_fed_back,
     forms(let([("$lam2", lam([["$x"], ["$l", "$r"]], case(["all", "leaf"], "$l"), case(["all", "fork"], pair("$x", "$l")))),
                ("$a1", app("$lam2", "{0}")), ("$hole", app("$a1", "~")), ("$b1", app("$lam2", "$hole")),
                ("$out", app("$b1", pair("{p}", "{q}")))], "$out"))),
    # G. activations
    ("G1", "closures returned from activations", equals("[~[{1}, {x}], ~[{2}, {y}]]"),
     forms(let([("$mkadd", lam([["$a"]], case(["all"], lam([["$b"]], case(["all"], pair("$a", "$b")))))),
                ("$f1", app("$mkadd", "{1}")), ("$f2", app("$mkadd", "{2}")),
                ("$r1", app("$f1", "{x}")), ("$r2", app("$f2", "{y}"))], lst("$r1", "$r2")))),
    ("G2", "mutual recursion", equals("{f}"), forms(MUTUAL)),
    ("G3", "deep recursion (length 300)", count_is(300),
     forms(let([("$fix", FIX), ("$len", LEN), ("$length", app("$fix", "$len")),
                ("$r", app("$length", "[" + ", ".join(["{a}"] * 300) + "]"))], "$r"))),
    ("G4", "factorial 3 and Fibonacci 6", pair_counts(6, 8), None),
    # H. store
    ("H1", "code kept in the store, read back and run", equals("{5}"),
     forms(let([("$tail", TAIL), ("$w", pair("{code}", let([("$z", "{5}")], "$z"))),
                ("$c", app("$new", lst("$w"))), ("$g", app("$get", lst("$c"))), ("$r", app("$tail", "$g"))], "$r"))),
    ("H2", "hole read from the store after its binder bound", hole("x"), forms(H2_PLAIN, H2_DATA)),
    ("H3", "store written in one activation, read in another", equals("{7}"),
     forms(let([("$setter", lam([["$a"]], case(["all"], let([("$u", app("$set", lst("$a", "{7}")))], "$u")))),
                ("$a", app("$new", lst("~"))), ("$s", app("$setter", "$a")), ("$g", app("$get", lst("$a")))], "$g"))),
]


def _programs_module():
    import programs
    return programs


def entries():
    programs = _programs_module()
    out = []
    for key, title, check, fs in ENTRIES:
        if key == "D3":
            fs = {"plain": programs.PROGRAMS["reflect"]}
        if key == "G4":
            fs = {"plain": programs.PROGRAMS["arith"].replace("<F>", numeral(3)).replace("<N>", numeral(6))}
        out.append((key, title, check, fs))
    return out
