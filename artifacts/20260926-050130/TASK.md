# New languages are defined by programs over a shared core

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine
- KIND: INTENT

- Desired condition (user, 2026-09-26): programs define new languages over a shared executable core without changing that core.
- Why (user): the introduction's M3, M4, S4, and S5 motivate a reusable language substrate.
- Qualification (user): “any compiler to any language” remains an aspiration, not a checkable completion condition.

## Implementation basis

- Completed work (agent, 2026-09-29): the project migrated to C, muh-build, and Lua tests. Zig removal is recorded in `4b0ff4c`.
- Recorded evidence (agent): the migration reported 25 passing Lua tests and fresh-clone verification. No tests were rerun for the task rewrite.
- Decision (user): deployment is library-first, with public C packages and shipped Lua bindings.
- Decision (user, 2026-09-29): dependency repositories are siblings. `recipe.lua` owns dependency names. `mine` holds vendored libraries.
- Decision (user, 2026-09-29): setup instructions belong in the relevant README. Do not duplicate dependency lists, add submodules, or invent a repository registry.
- Direction (user): preserve a hermetic, self-sufficient development workflow. Historical build experiments remain in the archive.

- History (agent): `archive/20261020-000000/records.md` preserves the original records. Its SUMMARY maps former subjects to their current owners.
