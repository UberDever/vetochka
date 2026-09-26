# Opcode and value tagging representation: history

- 2025-01-23 (user): native tagging chosen over in-calculus tagging. Faster, but couples the calculus to the implementation.
- 2026-02-04 (user): nodes are delta, value or opcode. `get_type`/`get_payload` and `set_type`/`set_payload`. Programs are trusted; payloads are forgeable.
- 2026-02-10 (user): `call` opcode for natives.
- 2026-02-22 (user): opcodes are tree values `^ [tag-magic opcode stuff...]`.
- ~~2026-03-18 (LLM): magic native integer at a fixed position~~ superseded by families, 2026-07-09.
- 2026-03-24 (user): runtime ABI of tagged values, shared by parser, VM and programs.
- 2026-06-04 (user): opcode as `[callable-tag, [cur, max arity], [curried args], impl]`.
- 2026-06-11 (user): a byte-string literal in function position names a native.
- Research notes: archive/20260322-120000/llm_research_on_opcode_shape.md, archive/20260322-120000/claude_opcode_do_research_2026-07-10.md.
