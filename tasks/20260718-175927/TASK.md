# Rule 3 dispatches on storage arity, wrong for APPLY

- STATUS: OPEN
- PRIORITY: 100
- TAGS: reducer, machine
- EPIC: 20241225-125809

- [fact] `reducer/reducer_reducer.c` Rule 3 switches on storage arity. That is wrong for `APPLY` and other non-nyad cells.
- Decision (user, 2026-08-30): Rule 3 never evaluates `{@}` data. A pending machine application in the operand is evaluated to expose its root shape, then Rule 3 resumes.
- [fact] The regression dates from commit `e0b7b3f`.
