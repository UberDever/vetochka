# Case-lambda construction experiment

Task: (22.11.26, 26). Fresh throwaway evaluator `evaluator.py`; programs in `programs.py`
(expanded v0 in `programs/`); checks in `test_case_lambdas.py`.
Run: `python3 -m pytest -q -p no:cacheprovider` from this directory. 14 checks pass.

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

## Choices the log does not settle

- C1. An authored lambda carries `~` in its private-state slot.
- C2. Declarations (header names, binder names) are read through context, so a program can put
  an identifier it computed there. A partial application stores the header it read, as literal
  identifier trees. Without storing it, `identifier-argument` fails: the second application
  re-reads `p` through its own binding and finds the argument. The ablation test shows this.
  Remaining hole: a declaration whose identity is already bound where it is read resolves to
  that value. Ordinary programs do not reach that; reflective code that rebuilds a lambda inside
  its own body can.
- C3. When an occurrence resolves to identifier data, that identifier is looked up again in the
  execution environment (19.11.26, 16). Generated bodies depend on it. Issued identities keep it
  from capturing unrelated names.
- C4. Spines, cases, and clauses are read as literal trees; only identifiers and shape symbols go
  through context.

## Not covered

Pending-parameter masking when shape inspection reads a partial lambda's cases (21.11.26, 3);
intrinsic operands under shape rows beyond `{:all}`; tail calls; the cells layout.
