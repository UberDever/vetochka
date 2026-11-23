"""A compiler from a parsed surface tree to v0, written in v0, whose output runs in v0.

Throwaway experiment; nothing here is settled. The Python in this file only assembles the text
of one v0 program (`COMPILER`) and supplies parsed source trees. Compilation and execution both
happen inside that v0 program on `evaluator.py`.

Surface tree (what a parser would hand over; `{:...}` are symbols):

    program := [form, ..., expr]                      ;; defs, then one expression
    form    := [{:def}, {name}, [{param}], expr]       ;; one parameter in this experiment
    expr    := [{:ref}, {name}] | [{:lit}, tree] | [{:call}, expr, expr] | [{:do}, expr, ...]

Lowering, done by the v0 program:

- every declaration gets a fresh identifier from `unique`; a scope is a list of `~[name, id]`;
- `def[f][x] body` becomes the binder clause `~[F, [[{:lambda}], ~, [[X]], ~[[{:all}], body]]]`;
- a call becomes a fresh temporary clause `~[T, [[{$}], callee, arg]]` (ANF);
- `do e1; ...; en end` keeps the clauses of every expression and the value of the last;
- the program becomes one binder: all clauses in order, with the last atom as the body.

The program returns `[~[{code}, binder], result]`: the generated binder held as data in a
wrapper, and the result of taking it out, which runs it.
"""

import evaluator as ev
from programs import expand


# ---------------------------------------------------------------- text assembly helpers


def sym(name):
    return "{:" + name + "}"


def lst(*items):
    return "[" + ", ".join(items) + "]"


def pair(left, right):
    return f"~[{left}, {right}]"


def app(callee, operand):
    return f"[[{{$}}], {callee}, {operand}]"


def case(row, body):
    return pair(lst(*(sym(shape) for shape in row)), body)


def lam(header, *cases):
    return lst("[{:lambda}]", "~", lst(*(lst(*position) for position in header)), *cases)


def let(clauses, body):
    return lst("[{:binder}]", lst(*(pair(name, rhs) for name, rhs in clauses)), body)


# ---------------------------------------------------------------- library

FIX = lam([["$fix-f"]], case(["all"], let([
    ("$fix-w", lam([["$fix-x"], ["$fix-v"]], case(["all", "all"], let([
        ("$fix-self", app("$fix-x", "$fix-x")),
        ("$fix-fs", app("$fix-f", "$fix-self")),
        ("$fix-r", app("$fix-fs", "$fix-v")),
    ], "$fix-r")))),
    ("$fix-g", app("$fix-w", "$fix-w")),
], "$fix-g")))

HEAD = lam([["$hd-h", "$hd-t"]], case(["fork"], "$hd-h"))
TAIL = lam([["$tl-h", "$tl-t"]], case(["fork"], "$tl-t"))

APPEND = lam([["$ap-rec"], ["$ap-h", "$ap-t"], ["$ap-ys"]],
    case(["all", "leaf", "all"], "$ap-ys"),
    case(["all", "fork", "all"], let([
        ("$ap-p", app("$ap-rec", "$ap-t")),
        ("$ap-r", app("$ap-p", "$ap-ys")),
    ], pair("$ap-h", "$ap-r"))))

# lookup(scope)(name): the identifier bound to `name`, or `{unbound}`.
LOOKUP = lam([["$lk-rec"], ["$lk-h", "$lk-t"], ["$lk-name"]],
    case(["all", "leaf", "all"], "{unbound}"),
    case(["all", "fork", "all"], let([
        ("$lk-r", app(lam([["$lk-n", "$lk-id"]], case(["fork"], let([
            ("$lk-eq", app("$fruit-equal", lst("$lk-n", "$lk-name"))),
            ("$lk-r2", app(lam([["$lk-e"]],
                case(["fruit"], "$lk-id"),
                case(["leaf"], let([
                    ("$lk-p", app("$lk-rec", "$lk-t")),
                    ("$lk-q", app("$lk-p", "$lk-name")),
                ], "$lk-q"))), "$lk-eq")),
        ], "$lk-r2"))), "$lk-h")),
    ], "$lk-r")))

# close(~[clauses, atom]): the single case of a lambda whose body evaluates them.
# Returning a case pair keeps a generated binder from running at compile time.
CLOSE = lam([["$cl-b", "$cl-a"]], case(["fork"], let([
    ("$cl-r", app(lam([["$cl-h", "$cl-t"]],
        case(["leaf"], pair(lst(sym("all")), "$cl-a")),
        case(["fork"], pair(lst(sym("all")), lst("[{:binder}]", "$cl-b", "$cl-a")))), "$cl-b")),
], "$cl-r")))

# compile(scope)(expr) = ~[clauses, atom]
REF = let([
    ("$c-nm", app("$head", "$c-rest")),
    ("$c-l1", app("$lookup", "$c-scope")),
    ("$c-id", app("$c-l1", "$c-nm")),
], pair("~", "$c-id"))

LIT = let([("$c-tr", app("$head", "$c-rest"))], pair("~", "$c-tr"))

CALL = let([
    ("$c-ce", app("$head", "$c-rest")),
    ("$c-rr", app("$tail", "$c-rest")),
    ("$c-ae", app("$head", "$c-rr")),
    ("$c-c1", app("$c-rec", "$c-scope")),
    ("$c-p1", app("$c-c1", "$c-ce")),
    ("$c-c2", app("$c-rec", "$c-scope")),
    ("$c-p2", app("$c-c2", "$c-ae")),
    ("$c-t", app("$unique", lst("{t}"))),
    ("$c-r4", app(lam([["$c-b1", "$c-a1"]], case(["fork"], let([
        ("$c-r5", app(lam([["$c-b2", "$c-a2"]], case(["fork"], let([
            ("$c-ap1", app("$append", "$c-b1")),
            ("$c-b12", app("$c-ap1", "$c-b2")),
            ("$c-cl", lst(pair("$c-t", app("$c-a1", "$c-a2")))),
            ("$c-ap2", app("$append", "$c-b12")),
            ("$c-b", app("$c-ap2", "$c-cl")),
        ], pair("$c-b", "$c-t")))), "$c-p2")),
    ], "$c-r5"))), "$c-p1")),
], "$c-r4")

DO = let([
    ("$c-d1", app("$dos", "$c-rec")),
    ("$c-d2", app("$c-d1", "$c-scope")),
    ("$c-d3", app("$c-d2", "$c-rest")),
], "$c-d3")


def dispatch(symbol, flag, hit, miss):
    return let([
        (f"$c-is{flag}", app("$fruit-equal", lst("$c-k", sym(symbol)))),
        (f"$c-r{flag}", app(lam([[f"$c-q{flag}"]], case(["fruit"], hit), case(["leaf"], miss)),
                            f"$c-is{flag}")),
    ], f"$c-r{flag}")


COMPILE = lam([["$c-rec"], ["$c-scope"], ["$c-k", "$c-rest"]],
    case(["all", "all", "fork"],
         dispatch("ref", "a", REF, dispatch("lit", "b", LIT, dispatch("call", "c", CALL, DO)))))

# dos(compile)(scope)(exprs) = ~[clauses of all, atom of the last]
DOS = lam([["$d-self"], ["$d-cmp"], ["$d-scope"], ["$d-e", "$d-more"]],
    case(["all", "all", "all", "fork"], let([
        ("$d-c", app("$d-cmp", "$d-scope")),
        ("$d-p", app("$d-c", "$d-e")),
        ("$d-r", app(lam([["$d-h", "$d-t"]],
            case(["leaf"], "$d-p"),
            case(["fork"], let([
                ("$d-s1", app("$d-self", "$d-cmp")),
                ("$d-s2", app("$d-s1", "$d-scope")),
                ("$d-q", app("$d-s2", "$d-more")),
                ("$d-r2", app(lam([["$d-b1", "$d-a1"]], case(["fork"], let([
                    ("$d-r3", app(lam([["$d-b2", "$d-a2"]], case(["fork"], let([
                        ("$d-ap", app("$append", "$d-b1")),
                        ("$d-bb", app("$d-ap", "$d-b2")),
                    ], pair("$d-bb", "$d-a2")))), "$d-q")),
                ], "$d-r3"))), "$d-p")),
            ], "$d-r2"))), "$d-more")),
    ], "$d-r")))

# cdef(scope)(def) = ~[clause, scope extended with the def's name]
CDEF = lam([["$f-scope"], ["$f-k", "$f-rest"]], case(["all", "fork"], let([
    ("$f-nm", app("$head", "$f-rest")),
    ("$f-r1", app("$tail", "$f-rest")),
    ("$f-ps", app("$head", "$f-r1")),
    ("$f-r2", app("$tail", "$f-r1")),
    ("$f-body", app("$head", "$f-r2")),
    ("$f-pn", app("$head", "$f-ps")),
    ("$f-fid", app("$unique", lst("$f-nm"))),
    ("$f-pid", app("$unique", lst("$f-pn"))),
    ("$f-inner", pair(pair("$f-pn", "$f-pid"), "$f-scope")),
    ("$f-c1", app("$compile", "$f-inner")),
    ("$f-pr", app("$f-c1", "$f-body")),
    ("$f-case", app("$close", "$f-pr")),
    # The header is built as data: an identifier written in a header position is a declaration
    # (general shadowing), so the computed declaration has to arrive with the whole header.
    ("$f-hdr", lst(lst("$f-pid"))),
    ("$f-lam", lst("[{:lambda}]", "~", "$f-hdr", "$f-case")),
    ("$f-cl", pair("$f-fid", "$f-lam")),
    ("$f-sc", pair(pair("$f-nm", "$f-fid"), "$f-scope")),
], pair("$f-cl", "$f-sc"))))

# forms(scope)(program) = ~[clauses, atom]: defs in order, then the final expression.
FORMS = lam([["$g-rec"], ["$g-scope"], ["$g-form", "$g-more"]],
    case(["all", "all", "fork"], let([
        ("$g-r", app(lam([["$g-h", "$g-t"]],
            case(["leaf"], let([
                ("$g-c", app("$compile", "$g-scope")),
                ("$g-p", app("$g-c", "$g-form")),
            ], "$g-p")),
            case(["fork"], let([
                ("$g-d", app("$cdef", "$g-scope")),
                ("$g-ds", app("$g-d", "$g-form")),
                ("$g-r2", app(lam([["$g-cl", "$g-sc"]], case(["fork"], let([
                    ("$g-n1", app("$g-rec", "$g-sc")),
                    ("$g-q", app("$g-n1", "$g-more")),
                    ("$g-r3", app(lam([["$g-bs", "$g-at"]], case(["fork"],
                        pair(pair("$g-cl", "$g-bs"), "$g-at"))), "$g-q")),
                ], "$g-r3"))), "$g-ds")),
            ], "$g-r2"))), "$g-more")),
    ], "$g-r")))

COMPILER = let([
    ("$fix", FIX),
    ("$head", HEAD),
    ("$tail", TAIL),
    ("$append", app("$fix", APPEND)),
    ("$lookup", app("$fix", LOOKUP)),
    ("$close", CLOSE),
    ("$dos", app("$fix", DOS)),
    ("$compile", app("$fix", COMPILE)),
    ("$cdef", CDEF),
    ("$forms", app("$fix", FORMS)),
    ("$m-c", app("$forms", "~")),
    ("$m-p", app("$m-c", "$source")),
    ("$m-w", app(lam([["$m-bs", "$m-at"]], case(["fork"],
        pair("{code}", lst("[{:binder}]", "$m-bs", "$m-at")))), "$m-p")),
    # Taking the generated binder out of its wrapper enters it, and entering runs a binder
    # however it is reached (22.11.26, 24); so the wrapper is the code output.
    ("$m-result", app("$tail", "$m-w")),
], lst("$m-w", "$m-result"))


# ---------------------------------------------------------------- sources (parsed trees)

# def[f][x] do x; end
# def[g][y] do y; end
# g(f({42}))
USER = """[
  [{:def}, {f}, [{x}], [{:do}, [{:ref}, {x}]]],
  [{:def}, {g}, [{y}], [{:do}, [{:ref}, {y}]]],
  [{:call}, [{:ref}, {g}], [{:call}, [{:ref}, {f}], [{:lit}, {42}]]]
]"""

# A probe beyond the user's program: a def whose body calls another def and uses `do` with two
# expressions, so a generated lambda's body is a binder that needs the lambda's parameter.
# def[f][x] do x; end
# def[h][z] do f(z); f(z); end
# h({7})
PROBE = """[
  [{:def}, {f}, [{x}], [{:do}, [{:ref}, {x}]]],
  [{:def}, {h}, [{z}], [{:do}, [{:call}, [{:ref}, {f}], [{:ref}, {z}]],
                               [{:call}, [{:ref}, {f}], [{:ref}, {z}]]]],
  [{:call}, [{:ref}, {h}], [{:lit}, {7}]]
]"""


def show_code(value):
    """Render generated v0 in vf-like notation: identifiers as name#identity."""
    value = ev.rewrite(value, holes=False)
    tree = value.tree

    def child(node):
        return ev.sub(value, node)

    if ev.is_identifier(tree):
        items = ev.elements(tree)
        return items[1].decode() + "#" + items[2].right.hex()
    if isinstance(tree, bytes):
        return tree.decode("utf-8", "replace")
    if tree is None:
        return "~"
    kind = ev.kind(value)
    parts = ev.items(value) if ev.is_list(tree) or kind else None
    if kind == b":binder":
        _, clauses, body = parts
        rendered = []
        for clause in ev.items(clauses):
            clause = ev.view(clause)
            rendered.append(show_code(ev.sub(clause, clause.tree.left)) + " = "
                            + show_code(ev.sub(clause, clause.tree.right)))
        return "=[" + ", ".join(rendered) + "] " + show_code(body)
    if kind == b"$":
        _, callee, operand = parts
        return show_code(callee) + "(" + show_code(operand) + ")"
    if kind == b":lambda":
        _, _, header, *cases = parts
        names = ", ".join(", ".join(show_code(n) for n in ev.items(p)) for p in ev.items(header))
        rows = []
        for case in cases:
            case = ev.view(case)
            row = ", ".join("{" + ev.view(s).tree.decode() + "}" for s in ev.items(ev.sub(case, case.tree.left)))
            rows.append("?[" + row + "] " + show_code(ev.sub(case, case.tree.right)))
        return "\\[" + names + "] " + " ".join(rows)
    if parts is not None:
        return "[" + ", ".join(show_code(p) for p in parts) + "]"
    return "~[" + show_code(child(tree.left)) + ", " + show_code(child(tree.right)) + "]"


def compiler_text():
    return expand(COMPILER)


def compile_and_run(source_text):
    """Run the v0 compiler on a parsed source tree; return (generated code, result)."""
    value = ev.run(ev.parse(compiler_text()), source=ev.parse(source_text))
    wrapper, result = [ev.rewrite(ev.at(item, value.env)) for item in ev.elements(value.tree)]
    code = ev.rewrite(ev.sub(wrapper, wrapper.tree.right))
    return code, result


if __name__ == "__main__":
    import sys

    sys.setrecursionlimit(100_000)
    for name, source in (("user", USER), ("probe", PROBE)):
        try:
            code, result = compile_and_run(source)
            print(name, "code:  ", show_code(code))
            print(name, "result:", ev.display(ev.materialize(result)))
        except ev.VError as error:
            print(name, "error:", error)
