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

## Skills

Load the matching skill before doing the work:

- `tatr` — any task edit: create, update, close, move, query. It also carries the rules for
  tasks as project memory.
- `memory` — recall and distill, with this project's locations: dated notes go into the task
  folder the session followed; index and search `tasks/` and `docs/`. There is no
  `.memsearch/memory/` here.
- `fpf` — design reasoning where claims, evidence, decisions and descriptions could mix.
- `dialogue` — a design session with the user; its ADR output goes into the task.
- `adopt-purist-language-researcher` / `adopt-pragmatic-language-developer` — when the user
  asks for a critique or challenge of a language design choice.
- `systematic-debugging` — a failing test or unexpected behavior, before proposing a fix.
- `verification-before-completion` — before reporting code as done or committing it.

If a skill is not available, tell the user which one is missing and continue without it,
following the rules in this file.

Source: all of these live in https://github.com/UberDever/agents, under `skills/<name>/`.
To restore one, copy or link that folder into the harness's skills directory
(for Claude Code, `~/.claude/skills/<name>/`).
