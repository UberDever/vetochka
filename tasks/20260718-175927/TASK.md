# Rule 3 dispatches on storage arity, wrong for APPLY

- STATUS: OPEN
- PRIORITY: 100
- TAGS: reducer, machine
- KIND: TASK
- PARENT: 20260529-103914

- [fact] `reducer/reducer_reducer.c` Rule 3 switches on storage arity. That is wrong for `APPLY` and other non-nyad cells.
- [fact] The regression dates from commit `e0b7b3f`.
- Rule 3 never evaluates (user, 2026-09-25): see 20260529-103914. For this code: Rule 3 must request the argument's triage view without evaluating it; an application is observed as a node of arity 2.
