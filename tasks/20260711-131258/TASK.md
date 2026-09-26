# do...end sequencing

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- Candidates (user, 2026-07-09): A, pure sugar to bind-and-discard chains; B, statement lists run by a BLOCK frame.
- Decision (user, 2026-08-30): B. The opcode machinery executes `do ... end`; it is not desugared.
