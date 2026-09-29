# Native operations slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode, syntax
- KIND: TASK
- PARENT: 20250123-091611

- Step 3 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091321.
- Work: integer and byte-string literals in both forms (20260926-053139); i64 arithmetic and comparison; byte-string operations; native tag equality (ruled inevitable, 20260209-162003); native calls as trampolines with no machine re-entry from C (20260711-131257).
- Open (agent): the v0 set of native operations beyond these.
- Done when: the opcodes are specified, and tests run arithmetic, comparison and string programs through `exec`.
