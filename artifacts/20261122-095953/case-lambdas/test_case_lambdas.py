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
    assert ev.static_kind(g.tree) == b":lambda"
    assert ev.display(g.tree.right.left) == "~[{:private}, ~{01}]"
    header = g.tree.right.right.left
    assert isinstance(header, ev.Term) and header.env is f.env
    assert header.tree is f.tree.right.right.left
    assert ev.read_header(header) == [
        (generated("a", "8000"),),
        (generated("l", "8001"), generated("r", "8002")),
    ]
    assert g.tree.right.right.right is f.tree.right.right.right


def test_case_selection_waits_for_the_last_argument():
    assert [shown(v) for v in results("two-positions")] == ["{fl}", "~[{b}, {c}]", "{other}"]


def test_first_matching_case_wins():
    assert [shown(v) for v in results("overlap")] == ["{first}", "{any}", "{any}"]


def test_binder_built_as_data_runs_only_when_a_clause_forces_it():
    b, res = results("binder")
    assert ev.static_kind(b.tree) == b":binder"
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
    first = unique.apply(ev.Term(b"x", {}))
    second = unique.apply(ev.Term(b"x", {}))
    assert first != second
    assert ev.is_identifier(first) and ev.is_identifier(second)


def test_reflective_reentry_rewrites_a_bound_declaration_to_its_value():
    # Inside `k`, `p`'s identity is bound to `{arg}`, so a lambda declaring it declares `{arg}`.
    with pytest.raises(ev.VError, match="declaration must be an identifier"):
        ev.run(program("reflect"))


def test_reflective_reentry_with_an_identifier_argument_declares_that_identifier():
    pid, out = results("reflect-identifier")
    assert ev.static_kind(pid.tree) == b":id" and pid.tree == elements_p(pid)
    assert shown(out) == "{z}"


def elements_p(pid):
    # `p` is the third identifier the simulated parser issued in this program.
    return ev.make_identifier(b"p", bytes.fromhex("03"))


def test_lists_held_in_variables_are_their_trees():
    assert [shown(v) for v in results("spines")] == ["{a}", "~[{1}, {2}]", "{w}"]


def test_fix_makes_lambdas_recursive_without_a_recursive_binding():
    assert [shown(v) for v in results("fix")] == ["[{s}, {s}, {s}]", "[{a}, {b}, {c}, {d}]"]
