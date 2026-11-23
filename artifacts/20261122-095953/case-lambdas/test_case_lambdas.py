import pytest

import evaluator as ev
from programs import PROGRAMS, expand


def program(name):
    return ev.parse(expand(PROGRAMS[name]))


def results(name):
    """Run a program whose body is a list of identifiers; return their values."""
    value = ev.run(program(name))
    return [ev.rewrite(ev.at(item, value.env)) for item in ev.elements(value.tree)]


def shown(value):
    return ev.display(ev.materialize(value))


def generated(name, key):
    return ev.make_identifier(name.encode(), bytes.fromhex(key))


def test_builder_uses_one_generated_identifier_for_declaration_and_occurrence():
    f, r = results("builder")
    assert shown(r) == "{hello}"
    header = ev.read_header(ev.Term(f.tree.right.right.left, f.env))
    assert header == [(generated("x", "8000"),)]


def test_shapes_bind_by_operand_shape_and_complete_one_partial_application_three_times():
    f, g, v1, v2, v3 = results("shapes")
    assert shown(v1) == "~[{p}, ~[{b}, {c}]]"
    assert shown(v2) == "~[{p}, {d}]"
    assert shown(v3) == "{p}"


def test_partial_application_stores_count_header_and_shares_cases():
    f, g, *_ = results("shapes")
    assert ev.kind(g) == b":lambda"
    assert ev.display(g.tree.right.left) == "~[{:private}, ~{01}]"
    header = g.tree.right.right.left
    assert isinstance(header, ev.Term) and header.env is f.env
    assert header.tree is f.tree.right.right.left
    assert ev.read_header(header) == [
        (generated("a", "8000"),),
        (generated("l", "8001"), generated("r", "8002")),
    ]
    # Shared, either raw or as a term node wrapping the same subtree.
    cases = g.tree.right.right.right
    assert getattr(cases, "tree", cases) is f.tree.right.right.right


def test_case_selection_waits_for_the_last_argument():
    assert [shown(v) for v in results("two-positions")] == ["{fl}", "~[{b}, {c}]", "{other}"]


def test_first_matching_case_wins():
    assert [shown(v) for v in results("overlap")] == ["{first}", "{any}", "{any}"]


def test_binder_taken_out_of_its_wrapper_runs():
    # `unbox`'s body rewrites to the binder, and entering a body runs a binder however it is
    # reached (23.11.26, 0), so `b` is already the binder's result.
    b, res = results("binder")
    assert shown(b) == "~[{v}, {v}]"
    assert shown(res) == "~[{v}, {v}]"


def test_generated_identifiers_with_the_same_spelling_are_distinct():
    value = ev.run(program("identity"))
    # Compare trees: a pair whose right child is a list displays as a longer list.
    assert ev.materialize(value) == ev.Fork(b"arg", generated("x", "8001"))


def test_partial_application_to_identifier_data_keeps_its_own_declarations():
    value = ev.run(program("identifier-argument"))
    assert shown(value) == "~[" + ev.display(generated("i", "8000")) + ", {w}]"


def test_ablation_without_the_defining_header_environment_breaks_that_program(monkeypatch):
    monkeypatch.setattr(ev, "WRAP_HEADER", False)
    # The second application reads `p` through its own binding and declares the argument's
    # identifier instead, so the row that inspects `p` finds no binding for it.
    with pytest.raises(ev.VError, match="not bound"):
        ev.run(program("identifier-argument"))


def test_shape_inspection_sees_the_rewritten_tree():
    assert shown(ev.run(program("observe"))) == "{fruit}"


@pytest.mark.parametrize(
    "name, message",
    [
        ("no-match", "no case matches"),
        ("fork-on-one-name", "two-name position"),
        ("short-row", "cover every parameter"),
        ("no-cases", "at least one case"),
    ],
)
def test_errors(name, message):
    with pytest.raises(ev.VError, match=message):
        ev.run(program(name))


def test_unique_issues_fresh_identities_per_call():
    env = ev.global_env()
    unique = env[ev.UNIQUE_KEY]
    first = unique.apply(ev.Term(ev.parse("[{x}]"), {}))
    second = unique.apply(ev.Term(ev.parse("[{x}]"), {}))
    assert first != second
    assert ev.is_identifier(first) and ev.is_identifier(second)


def test_reflective_reentry_shadows_instead_of_rewriting_the_declaration():
    # Declarations are read without looking at the places a term was carried through, so the
    # lambda built inside `k` redeclares `p`'s identity and returns its own argument.
    for name in ("reflect", "reflect-identifier"):
        pid, out = results(name)
        assert ev.is_identifier(pid.tree)
        assert shown(out) == "{z}"


def elements_p(pid):
    # `p` is the third identifier the simulated parser issued in this program.
    return ev.make_identifier(b"p", bytes.fromhex("03"))


def test_lists_held_in_variables_are_their_trees():
    assert [shown(v) for v in results("spines")] == ["{a}", "~[{1}, {2}]", "{w}"]


def test_fix_makes_lambdas_recursive_without_a_recursive_binding():
    assert [shown(v) for v in results("fix")] == ["[{s}, {s}, {s}]", "[{a}, {b}, {c}, {d}]"]


def count(value):
    """Length of a unary numeral, walking its spine through rewriting."""
    total = 0
    current = ev.rewrite(value)
    while current.tree is not None:
        assert isinstance(current.tree, ev.Fork)
        assert ev.rewrite(ev.at(current.tree.left, current.env)).tree == b"s"
        total += 1
        current = ev.rewrite(ev.at(current.tree.right, current.env))
    return total


@pytest.mark.parametrize("n, factorial, fibonacci", [
    (0, 1, 0), (1, 1, 1), (2, 2, 1), (3, 6, 2), (4, 24, 3), (5, 120, 5), (6, 720, 8), (10, None, 55),
])
def test_factorial_and_fibonacci_with_fix(n, factorial, fibonacci):
    import sys
    from programs import numeral

    # The evaluator recurses in Python; the depth here is a host limit, not a language one.
    old = sys.getrecursionlimit()
    sys.setrecursionlimit(100_000)
    try:
        text = expand(PROGRAMS["arith"])
        # Factorial of 10 would be 3,628,800 unary digits; give it 0 when it is not checked.
        text = text.replace("<F>", numeral(n if factorial is not None else 0))
        value = ev.run(ev.parse(text.replace("<N>", numeral(n))))
        out1, out2 = [ev.rewrite(ev.at(item, value.env)) for item in ev.elements(value.tree)]
        if factorial is not None:
            assert count(out1) == factorial
        assert count(out2) == fibonacci
    finally:
        sys.setrecursionlimit(old)


def test_declarations_that_look_outward_break_recursion(monkeypatch):
    def declaration(term):
        declared = ev.rewrite(term, holes=True)
        if not (isinstance(declared, ev.Term) and ev.is_identifier(declared.tree)):
            raise ev.VError("a declaration must be an identifier")
        return declared.tree

    monkeypatch.setattr(ev, "declaration", declaration)
    with pytest.raises(ev.VError, match="declaration must be an identifier"):
        results("fix")
