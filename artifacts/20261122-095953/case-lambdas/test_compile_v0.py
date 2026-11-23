import pytest

import compile_v0 as c
import evaluator as ev


def shown(value):
    return ev.display(ev.materialize(value))


def test_user_program_compiles_in_v0_and_runs_in_v0():
    code, result = c.compile_and_run(c.USER)
    assert ev.kind(code) == b":binder"
    assert c.show_code(code) == (
        "=[f#8000 = \\[x#8001] ?[{:all}] x#8001, "
        "g#8002 = \\[y#8003] ?[{:all}] y#8003, "
        "t#8004 = f#8000(42), "
        "t#8005 = g#8002(t#8004)] t#8005"
    )
    assert shown(result) == "{42}"


def test_generated_lambda_body_can_be_a_binder_that_uses_the_parameter():
    code, result = c.compile_and_run(c.PROBE)
    assert c.show_code(code) == (
        "=[f#8000 = \\[x#8001] ?[{:all}] x#8001, "
        "h#8002 = \\[z#8003] ?[{:all}] =[t#8004 = f#8000(z#8003), t#8005 = f#8000(z#8003)] t#8005, "
        "t#8006 = h#8002(7)] t#8006"
    )
    assert shown(result) == "{7}"


def test_ablation_last_place_rule_cannot_run_generated_code(monkeypatch):
    monkeypatch.setattr(ev, "INHERIT_PLACES", False)
    with pytest.raises(ev.VError, match="expected a callable"):
        c.compile_and_run(c.USER)
