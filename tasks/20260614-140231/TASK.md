# Abstract opcode node creation in the source encoder

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: reducer
- EPIC: 20241225-125809

- [fact] `new_op_fn` in `reducer/bytecode_source.c` hardcodes `OP_FN0`. The comment asks to derive it from VM info.
- [fact] `OP_FN0/1/2` are an old experiment, not target semantics (2026-07-10).
- Closed (user, 2026-09-24): the `{fn}` / OP_FN0 recognition was dropped from the encoder; `$fn:` superseded it.
