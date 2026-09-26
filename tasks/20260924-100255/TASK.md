# Reword cells: runtime nodes with non-trivial piths

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding, spec
- KIND: TASK
- PARENT: 20260925-134730

- [fact] `docs/new_spec/02_syntax.md`, Cells: "Every node is typed ... and has its arity: `0, 1 or 2`. Note that semantically a node could have any amount of children if proper list encoding is used." Written by the user, 2026-08-05.
- Decision (user, 2026-09-24): the idea stays; the wording changes. There are special runtime nodes throughout, a "runtime category" with non-trivial piths, and they may have more than two children.
- Reminder for the user: rewrite that paragraph in the spec.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
