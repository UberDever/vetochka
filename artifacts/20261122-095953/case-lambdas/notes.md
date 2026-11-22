# Case-lambda construction experiment

Task: (22.11.26, 26). Fresh throwaway evaluator `evaluator.py`; programs in `programs.py`
(expanded v0 in `programs/`); checks in `test_case_lambdas.py`.
Run: `python3 -m pytest -q -p no:cacheprovider` from this directory. 15 checks pass.

## What runs

- `builder`: a lambda receives one identifier from `unique({x})` and returns a lambda that uses it
  as both declaration and body occurrence; applying the result to `{hello}` returns `{hello}`.
- `shapes`: a case-lambda with header `[[a], [l, r]]` built from generated identifiers. One partial
  application `g = f({p})` completes three times: fork, fruit, and leaf rows each select.
  `g` stores count `01`, the header it read as literal identifiers, and the original cases (shared).
- `two-positions`: rows `[fork, leaf]` and `[fork, fruit]` both survive the first argument; the second decides.
- `overlap`: first matching row in written order wins.
- `binder`: a binder held in a fork is extracted by a case-lambda (not run), then a clause forces it.
  Clauses bind in order: `~[{v}, {v}]`.
- `identity`: two generated `{x}` identifiers stay distinct bindings.
- Errors: no matching row, `{:fork}` at a one-name position, a short row, a lambda without cases.

## Adjusted after (22.11.26, 28) and (22.11.26, 29)

- One rule, `rewrite`, serves execution, shape inspection, and display. An occurrence bound in its
  environment rewrites to its value; identifier data reached that way stands where the
  occurrence stood and is rewritten in that environment; an unbound identifier is a hole.
  New program `observe`: inside `f`, which declares generated `X`, shape inspection of the
  fork child `x` gives `{fruit}`. The evaluator before this change gave `{fork}`.
- The header is rewritten at the first application and stored in the partial application as a
  term with an empty environment. Without the wrap, `identifier-argument` fails: the second
  application rewrites `p` through its own binding to the argument (ablation test).
- Remaining hole: a declaration whose identity is already bound where the header is first read
  rewrites to that value. Ordinary programs do not reach that; reflective code that rebuilds a
  lambda inside its own body can.

## Choices the log does not settle

- C1. An authored lambda carries `~` in its private-state slot.
- C4. Spines, cases, and clauses are read as literal trees; only identifiers and shape symbols go
  through context.

## Not covered

Pending-parameter masking when shape inspection reads a partial lambda's cases (21.11.26, 3);
intrinsic operands under shape rows beyond `{:all}`; tail calls; the cells layout.
