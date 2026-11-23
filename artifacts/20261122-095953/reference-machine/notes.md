# Reference machine (throwaway)

`machine.py`: v0 with substitution as literal rewriting, so it behaves as in-place rewriting by
construction (22.11.26, 24). No environment: the state is a control tree, a store, and a stack
of `bind` frames. `test_reference.py`: 27 checks, including cross-tests against the throwaway
evaluator in `../case-lambdas/`. Run `python3 -m pytest -q -p no:cacheprovider` here.

## Rules as implemented

- `subst` replaces substituted identifiers by their values, inserting values as they are.
- General shadowing (22.11.26, 26): identifiers written directly in a lambda's header positions,
  or as a binder clause's left side, are declarations; rewriting leaves them alone and stops
  replacing those identities inside the lambda's cases, or in the binder's later clauses and body.
  Anything else in those slots (an occurrence standing for a whole header or clause list, or a
  list tail) is rewritten like any tree.
- `enter` runs a binder however it was reached; `force` applies, runs a binder, errors on a hole,
  or returns a value; `return` pops a `bind` frame and rewrites the rest of the binder.
- Applying a lambda binds the next position by the operand's shape and rewrites the cases. A
  partial result keeps the header and stores `~[{:private}, <count, shape codes>]`; the last
  argument selects the first matching row and enters it as a tail call.
- Intrinsics are trees `[[{:intrinsic}], {name}]` taking a list of arguments: `unique`,
  `fruit-equal`, and store operations `new`, `get`, `set` over addresses
  `[[{:address}], ~[{:private}, n]]`. The runner rewrites global identities to these at entry,
  and the compiler's `source` to the parsed tree.

## Findings

- **Header positions declare what is written there.** With literal rewriting and general
  shadowing, `[[{:lambda}], ~, [[decl]], ...]` inside `mk = \[decl, body] ...` declares `decl`
  itself, so the builder's generated identifier in `body` stays a hole. A computed declaration
  has to arrive with the whole header (or the whole clause list) as data. The compiler now builds
  the header as data (`$f-hdr`), and runs the same on both machines. The `builder` and `spines`
  programs put occurrences in header positions and differ between the machines by design. This
  removes the reason given for (22.11.26, 14), that a header can name values from its defining
  environment.
- **Decision (a) in practice.** Taking a generated binder out of its wrapper enters it and runs
  it. The compiler therefore returns the wrapper as its code output and runs the code with
  `tail(wrapper)`. The throwaway evaluator now also runs a binder reached through an identifier.
- **No host recursion per step.** Recursive `length` over 1500 elements runs within a Python
  recursion limit of 4500, which `subst` needs for the tree depth alone; the throwaway evaluator
  fails at the same limit. 3000 elements take about 5 s, because every step copies trees.
- **Cross-test.** Two-positions, overlap, binder, identity, identifier-argument, observe, reflect,
  reflect-identifier, fix, and the compiler (code and result) agree with the evaluator. Shapes
  agrees on values; its lambda trees differ in declared identities and in private-state bytes.
