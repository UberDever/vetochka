# Cell encoding and reducer core: history

- 2024-12-25 (user): 64-bit nodes in one array, relative child offsets.
- 2026-01-31 (user): typed nodes with refs and fixed/variable natives. Rejected, see task 20260201-183605.
- 2026-02-01 (user): tree as a byte stream with inline payload. See task 20260201-183608.
- 2026-02-09 (user): only `ref2` and `ref8` remain; other tags start at `0x80`; delta, value and opcode each come in arity 0..2.
- 2026-02-10 (user): a `call` opcode calls natives; plain values are no longer callable.
- 2026-02-22 (user): keep the tree calculus as is; opcodes are tree values; triage rules 0a..3c written down.
- 2026-06-14 (user): byte encoding revisited. No record of what changed.
