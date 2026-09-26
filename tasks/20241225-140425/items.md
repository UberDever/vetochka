# Scopes, modules and binders: history

- 2024-12-25 (user): `scope` blocks; named scopes are modules, unnamed ones are let-bindings; `root.tree` anchors module lookup; `use` brings bindings in.
- 2026-06-04 (user): module discovery is separate from its effect on the VM.
- 2026-06-11 (user): the host supplies candidate sources; bootstrap code decides what they mean.
- 2026-06-17 (user): implementation order item: checked module candidates, dependency resolution, exports.
- 2026-07-11 (user): `$f` one-argument and `$fn` multi-argument functions; `$f` later folded into `$fn`.
- 2026-08-30 (user): identifier evaluation matrix, in identifier-context.md.
- Examples: scope_syntax_example.md and the module_*.md files.
- Dialogues: archive/20260717-120000/lexical-reference-models.md, identifier-context.md.
