# Rule 3 dispatches on storage arity, wrong for APPLY

- STATUS: OPEN
- PRIORITY: 100
- TAGS: reducer, machine
- EPIC: 20241225-125809

- [fact] `reducer/reducer_reducer.c` Rule 3 switches on storage arity. That is wrong for `APPLY` and other non-nyad cells.
- ~~Decision (user, 2026-08-30): Rule 3 never evaluates `{@}` data. A pending machine application in the operand is evaluated to expose its root shape, then Rule 3 resumes.~~ superseded by the 2026-09-25 ruling: Rule 3 never evaluates.
- [fact] The regression dates from commit `e0b7b3f`.
- Decision (user, 2026-09-25): Rule 3 never evaluates. Applications persist and Rule 3 observes them like any node (arity 2); identifier dereference is its only indirection, which is how programs use binders' non-locality with Rule 3. To inspect a result rather than the computation, bind it with `$fn` and inspect the identifier. This departs from treecalcul.us, where an application cannot reach a Rule 3 branch, and the spec should say so. For this code: Rule 3 must request the argument's triage view without evaluating it; an application is observed as a node of arity 2.
