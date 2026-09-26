# Move task tracking to tatr

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: process

- Decision (user): task tracking uses tatr. `docs/` holds user-facing docs; `tasks/` holds the record of design work.
- Decision (user): tasks are the source of truth for design labour, as important as code. They state current knowledge, not migrated quotes.
- Decision (user): epics first, tagged `epic`. Sub-items that are not tasks on their own go in the epic's `items.md`.
- Decision (user): task ID is the time the note was first written. Collisions take the next free second.
- Decision (user): docs and code reference tasks by ID instead of TODOs.
- Decision (user): `dialogue/`, `project/` and `.memsearch/memory/` are abandoned. Their files live in task folders.
- Decision (user): each project has its own memsearch collection (`vetochka`, in `.memsearch.toml`), indexing `tasks/` and `docs/`. The shared default collection is to be purged.
- Decision (user): the repo-local memory skill is removed. The tatr skill carries the task-as-memory rules.
- Decision (user): built spec PDFs (`docs/*/output.pdf`) are gitignored.
- Open: purge of the shared `memsearch_chunks` collection was blocked by permissions; the user runs it.
- Details: archive/20260924-120000/2026-09-24.md.
- Decision (user, 2026-09-26): a third artifact kind, `archive/`, for garbage-collected process notes; see AGENTS.md.
