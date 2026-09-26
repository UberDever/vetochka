# Shift perspective from reducer to VM

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: machine
- KIND: TASK
- PARENT: 20260529-103914

- Decision (user, 2026-04-06): treat the runtime as a VM with tree-shaped opcodes and big-step semantics.
- VM state may include a continuation stack and an environment for bound variables.
