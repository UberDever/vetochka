"""Checks for the reference machine, and cross-tests against the throwaway evaluator."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "case-lambdas"))

import compile_v0  # noqa: E402
import evaluator as ev  # noqa: E402
import machine as m  # noqa: E402
from programs import PROGRAMS, expand, numeral  # noqa: E402

GLOBALS = {
    b"\xf0": m.intrinsic("unique"),
    b"\xf1": m.intrinsic("fruit-equal"),
    b"\xf3": m.intrinsic("new"),
    b"\xf4": m.intrinsic("get"),
    b"\xf5": m.intrinsic("set"),
}


def text(name, **inputs):
    source = expand(PROGRAMS[name])
    for marker, value in inputs.items():
        source = source.replace(f"<{marker}>", value)
    return source


def run(source):
    result, store, steps = m.run(m.parse(source), GLOBALS)
    return result


def items(tree):
    elements, tail = m.spine(tree, stop_at_identifiers=False)
    assert tail is None
    return elements


def show(tree):
    """Generated v0 in the notation of compile_v0.show_code."""
    if m.is_identifier(tree):
        name, private = items(tree)[1:]
        return name.decode() + "#" + private.right.hex()
    if isinstance(tree, bytes):
        return tree.decode()
    if tree is None:
        return "~"
    if m.kind(tree) == b":binder":
        _, clauses, body = items(tree)
        rendered = [show(c.left) + " = " + show(c.right) for c in items(clauses)]
        return "=[" + ", ".join(rendered) + "] " + show(body)
    if m.kind(tree) == b"$":
        _, callee, operand = items(tree)
        return show(callee) + "(" + show(operand) + ")"
    if m.kind(tree) == b":lambda":
        _, _, header, *cases = items(tree)
        names = ", ".join(", ".join(show(n) for n in items(p)) for p in items(header))
        rows = ["?[" + ", ".join("{" + s.decode() + "}" for s in items(c.left)) + "] " + show(c.right)
                for c in cases]
        return "\\[" + names + "] " + " ".join(rows)
    elements, tail = m.spine(tree, stop_at_identifiers=False)
    if tail is None:
        return "[" + ", ".join(show(e) for e in elements) + "]"
    return "~[" + show(tree.left) + ", " + show(tree.right) + "]"


def count(tree):
    total = 0
    while tree is not None:
        assert tree.left == b"s"
        total, tree = total + 1, tree.right
    return total


# ---------------------------------------------------------------- the reference machine itself


def test_case_lambda_shapes_and_partial_application():
    f, g, v1, v2, v3 = items(run(text("shapes")))
    assert [m.display(v) for v in (v1, v2, v3)] == ["~[{p}, ~[{b}, {c}]]", "~[{p}, {d}]", "{p}"]
    assert m.kind(g) == b":lambda"
    # count 1, then the shape of the bound argument (leaf 0, fork 1, fruit 2)
    assert m.display(items(g)[1]) == "~[{:private}, ~{01 02}]"


def test_selection_waits_for_all_arguments_and_first_match_wins():
    assert [m.display(v) for v in items(run(text("two-positions")))] == ["{fl}", "~[{b}, {c}]", "{other}"]
    assert [m.display(v) for v in items(run(text("overlap")))] == ["{first}", "{any}", "{any}"]


def test_a_header_position_declares_the_identifier_written_there():
    # `mk = \[decl, body] [[{:lambda}], ~, [[decl]], ...]`: the inner lambda declares `decl`
    # itself (general shadowing), so `body`'s generated identifier stays a hole.
    f, r = items(run(text("builder")))
    assert show(f).startswith("\\[decl#")
    assert m.is_identifier(r) and items(r)[1] == b"x"


def test_binder_taken_out_of_its_wrapper_runs():
    b, res = items(run(text("binder")))
    assert m.display(b) == m.display(res) == "~[{v}, {v}]"


def test_identities_not_spellings():
    r = run(text("identity"))
    assert r.left == b"arg" and m.is_identifier(r.right) and items(r.right)[2].right == b"\x80\x01"


def test_reflective_reentry_shadows():
    for name in ("reflect", "reflect-identifier"):
        assert m.display(items(run(text(name)))[1]) == "{z}"


def test_fix_factorial_fibonacci():
    v1, v2 = items(run(text("fix")))
    assert (m.display(v1), m.display(v2)) == ("[{s}, {s}, {s}]", "[{a}, {b}, {c}, {d}]")
    for n, factorial, fibonacci in [(0, 1, 0), (3, 6, 2), (5, 120, 5), (10, None, 55)]:
        fact_in = numeral(n if factorial is not None else 0)
        out1, out2 = items(run(text("arith", F=fact_in, N=numeral(n))))
        if factorial is not None:
            assert count(out1) == factorial
        assert count(out2) == fibonacci


def test_recursion_depth_does_not_use_the_host_stack():
    # The throwaway evaluator overflowed Python's stack between 50 and 100 elements.
    # Only `subst` recurses, once per level of tree depth (here about n). A machine that recursed
    # per program step would need several host frames per recursion level and fail this limit.
    n = 1500
    source = text("fix").replace("[{a}, {b}, {c}]]]", "[" + ", ".join(["{a}"] * n) + "]]]", 1)
    old = sys.getrecursionlimit()
    sys.setrecursionlimit(3 * n)
    try:
        result, _, steps = m.run(m.parse(source), GLOBALS)
        ok = True
    except RecursionError:
        ok = False  # assert outside the handler: pytest would print the whole deep tree
    finally:
        sys.setrecursionlimit(old)
    assert ok, "a machine step recursed in the host"
    assert count(items(result)[0]) == n


def test_store_addresses():
    program = expand("""
[[{:binder}], [
  ~[$a, [[{$}], $new, [{1}]]],
  ~[$u, [[{$}], $set, [$a, {2}]]],
  ~[$v, [[{$}], $get, [$a]]]
 ], [$a, $v]]""")
    a, v = items(run(program))
    assert m.kind(a) == b":address" and m.display(v) == "{2}"


@pytest.mark.parametrize("name, message", [
    ("no-match", "no case matches"),
    ("fork-on-one-name", "two-name position"),
    ("short-row", "cover every parameter"),
    ("no-cases", "at least one case"),
])
def test_errors(name, message):
    with pytest.raises(m.MachineError, match=message):
        run(text(name))


def test_running_a_hole_is_an_error():
    program = expand("[[{:binder}], [~[$x, [[{$}], $unique, [{x}]]], ~[$y, $x]], $y]")
    with pytest.raises(m.MachineError, match="unbound identifier"):
        run(program)


# ---------------------------------------------------------------- the compiler on both machines


@pytest.mark.parametrize("source", [compile_v0.USER, compile_v0.PROBE], ids=["user", "probe"])
def test_compiler_runs_on_the_reference_machine_like_on_the_evaluator(source):
    out, _, _ = m.run(m.parse(compile_v0.compiler_text()), GLOBALS | {b"\xf2": m.parse(source)})
    wrapper, result = items(out)
    old_code, old_result = compile_v0.compile_and_run(source)
    assert show(wrapper.right) == compile_v0.show_code(old_code)
    assert m.display(result) == ev.display(ev.materialize(old_result))
    assert m.display(result) in ("{42}", "{7}")


# ---------------------------------------------------------------- cross-test on all programs

# Programs whose values the two machines must agree on. `builder` and `spines` differ by design:
# they put an occurrence in a header position, which declares that occurrence under general
# shadowing, while the evaluator reads headers through their bindings.
AGREE = ["two-positions", "overlap", "binder", "identity", "identifier-argument", "observe",
         "reflect", "reflect-identifier", "fix"]


@pytest.mark.parametrize("name", AGREE)
def test_both_machines_agree(name):
    source = text(name)
    reference = run(source)
    value = ev.run(ev.parse(source))
    assert m.display(reference) == ev.display(ev.materialize(value))


@pytest.mark.parametrize("name", ["builder", "spines"])
def test_header_occurrences_are_where_the_machines_differ(name):
    source = text(name)
    reference = run(source)
    value = ev.run(ev.parse(source))
    assert m.display(reference) != ev.display(ev.materialize(value))
