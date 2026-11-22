import unittest

from spike import Term, apply, display, force, inspect, parse, run


class ForceTests(unittest.TestCase):
    def test_single_binder_returns_application_intact(self):
        self.assertEqual(run("""[
          [{:binder}],
          [~[{next}, [[{$}], [[{:lambda}], [{unused}], [[{$}], [[{:id}], {f}], ~]], ~]]],
          [[{:id}], {next}]
        ]"""), '[[{$}], [[{:id}], {f}], ~]')

    def test_library_force_runs_each_returned_call_once(self):
        calls = []

        def tick(_):
            calls.append("tick")
            return b"done"

        self.assertEqual(force("""
          [[{$}], [[{:lambda}], [{unused}],
            [[{$}], [[{:id}], {tick}], ~]], ~]
        """, intrinsics={"tick": tick}), "{done}")
        self.assertEqual(calls, ["tick"])

    def test_library_force_stops_at_a_fork_containing_a_call(self):
        def tick(_):
            self.fail("a quoted call was executed")

        self.assertEqual(force("""
          [[[{$}], [[{:id}], {tick}], ~]]
        """, intrinsics={"tick": tick}), '[[[{$}], [[{:id}], {tick}], ~]]')

    def test_library_force_keeps_the_returned_calls_environment(self):
        self.assertEqual(force("""[
          [{$}],
          [[{:lambda}], [{local}], [[{$}], [[{:id}], {local}], ~]],
          [[{:lambda}], [{unused}], {local-result}]
        ]"""), "{local-result}")

    def test_library_force_steps_a_returned_binder(self):
        self.assertEqual(force("""[
          [{$}],
          [[{:triage}], [{part}, {rest}], ~, [[{:id}], {part}], ~],
          [[ [{:binder}], [~[{next}, {binder-result}]], [[{:id}], {next}] ]]
        ]"""), "{binder-result}")

    def test_library_force_errors_on_an_unbound_identifier(self):
        with self.assertRaisesRegex(ValueError, "unbound identifier"):
            force("[[{:id}], {missing}]")

    def test_shared_triage_names_bind_by_shape_and_mask_outer_names(self):
        template = """[
          [{:binder}], [~[{lhs}, {outer-lhs}]],
          [
            [{:binder}], [~[{rhs}, {outer-rhs}]],
            [
              [{:binder}],
              [~[{result}, [[{$}], [[{:triage}], [{lhs}, {rhs}], %s, %s, %s], %s]]],
              [[{:id}], {result}]
            ]
          ]
        ]"""
        cases = (
            ("~", "lhs", "[[{:id}], {lhs}]"),
            ("~", "rhs", "[[{:id}], {rhs}]"),
            ("~[{left}, {right}]", "lhs", "{left}"),
            ("~[{left}, {right}]", "rhs", "{right}"),
            ("{fruit}", "lhs", "{fruit}"),
            ("{fruit}", "rhs", "[[{:id}], {rhs}]"),
        )
        for subject, name, expected in cases:
            with self.subTest(subject=subject, name=name):
                body = "[[{:id}], {%s}]" % name
                self.assertEqual(run(template % (body, body, body, subject)), expected)

    def test_triage_leaves_pending_lambda_parameters_as_identifiers(self):
        left = Term(parse("[[{:triage}], [{lhs}, {rhs}], ~, [[{:id}], {lhs}], ~]"), {})
        right = Term(parse("[[{:triage}], [{lhs}, {rhs}], ~, [[{:id}], {rhs}], ~]"), {})
        callee = Term(
            parse("[[{:lambda}], [{x}, {y}], ~[[[{:id}], {x}], [[{:id}], {y}]]]"),
            {b"x": Term(b"outer-x", {}), b"y": Term(b"outer-y", {})},
        )
        partial = apply(callee, Term(b"supplied-x", {}))
        complete = apply(partial, Term(b"supplied-y", {}))
        cases = (
            ("unapplied", apply(left, apply(right, apply(right, callee))),
             ("[[{:id}], {x}]", "[[{:id}], {y}]")),
            ("partial", apply(left, apply(right, apply(right, partial))),
             ("{supplied-x}", "[[{:id}], {y}]")),
            ("complete", complete, ("{supplied-x}", "{supplied-y}")),
        )
        for state, body, expected in cases:
            with self.subTest(state=state):
                observed = tuple(display(inspect(apply(pick, body)).tree) for pick in (left, right))
                self.assertEqual(observed, expected)


if __name__ == "__main__":
    unittest.main()
