# Recursion derived, not primitive

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: binders
- KIND: TASK
- PARENT: 20241225-140425

- Decision (user, 2026-07-09): recursion is derived, not primitive. `{rec}` is rejected.
- `fix` is the plain Y combinator through call-by-name `{form}`. Mutual recursion passes a function list through one `rec`. Verified in a stand-in.
- (user, 2026-08-30) `let`'s `<~` clause routes through `$form` and fix.
