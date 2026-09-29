# Piths for every runtime kind

- STATUS: OPEN
- PRIORITY: 100
- TAGS: encoding, machine
- KIND: TASK
- PARENT: 20260925-134730

- Step 4 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091323; the pith model is decided in 20260926-091318.
- Work: a pith for each kind in `20260925-134730/object-kinds.md`: nyad, list, integer, byte string, application, identifier, closure, opcode state, native handle, and the pith itself. Syntax-only kinds are plain lists. Definitions live in 20260805-162238.
- Scope (user, 2026-09-26): the object-kinds inventory for now; further kinds such as boxes (20260604-111747) later.
- Done when: every kind has a specified pith, and tests show duality: a pith built from scratch represents the same node.
