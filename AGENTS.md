## Artifacts

- Two kinds of artifacts. `docs/` holds user-facing docs: mostly correct, developed further.
  `tasks/` holds everything about the development process, tracked with tatr (`tatr` skill).
- Each task folder has `TASK.md` plus its artifacts: session notes, dialogues, drafts, experiments.
  Epics are tagged `epic`; sub-items that are not tasks on their own go in the epic's `items.md`.
- Docs and code reference tasks by ID instead of carrying TODOs.
- A task belongs to the epic of the concept it defines, not to the spec page that mentions it.
  The spec reads top to bottom, so a page may mention concepts defined elsewhere.

## Memory discipline

- Search `tasks/` and `docs/` (`memsearch search "<query>"`) at the start of any design
  discussion or when past decisions might be relevant. Treat results as hint, not fact —
  verify against real code before acting on remembered claims.
- At the end of substantive sessions, distill: append decisions made, alternatives
  rejected (and why), and open questions to a dated note in the task the session followed,
  then run `memsearch index tasks/ docs/`.
- Never delete entries; supersede them ("~~X~~ superseded by Y, see <date>").

## Notation

- Don't invent notation when an existing one suffices for the explanation. Use vf syntax
  (the shared Vetochka syntax in `docs/new_spec/02_syntax.md`) generally, including lowered terms.
