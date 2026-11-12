## Artifacts

- `docs/` holds user-facing documentation. `devlog/` holds the development record.
  Read `devlog/format.md` and `devlog/rules.md` before working with entries.
  `devlog/preferences.json` holds rotation and tool defaults.
- Session notes, dialogues, designs, experiments, and migration evidence live under `artifacts/<HUID>/`.
  Log entries contain the decisions, proposals, evidence, work items, and references to those artifacts.
- Docs and code reference entries by `(date, index)` instead of carrying TODOs.
- `devlog/rules.md` defines reading, writing, tag use, supersession, and verification procedures.

## Spec

- The user writes the spec; the agent makes targeted edits to existing text only, in plain English. Add spec text only when the user explicitly requests it. A request to apply decisions does not authorize new prose.
- Spec text is concise and normative; reasoning and alternatives go into log entries and their artifacts.
- Retained specification snapshots are under `artifacts/20261110-000001/`. Their migration does not approve old proposals.

## Memory discipline

- Search the current log, its rotations, and `docs/` at the start of design discussions or when past decisions matter.
  Follow artifact references for detail. Verify remembered claims against current code before relying on them.
- At the end of substantive sessions, append decisions, rejected alternatives and their reasons, evidence, and open questions to the log.
  Preserve attribution and distinguish proposals from approved decisions.
- Treat search indexes as disposable caches. An index that omits rotated logs is incomplete; search those files directly.
- The migration mapping and original source snapshot are under `artifacts/20261110-000000/`.
  Those imported IDs refer to the preceding log saved under `artifacts/20261110-000002/`, not the compacted active log.
  The compaction record and active reference mapping are in that same directory.
  Legacy instructions and task metadata retained in artifacts are historical records, not the active workflow.

## Notation

- Don't invent notation when an existing one suffices. Use vf syntax, including lowered terms.
  The retained syntax snapshot is `artifacts/20261110-000001/new_spec/02_syntax.md`.

## Skills

Load the matching skill before doing the work:

- `memory` — recall and distill through `devlog/` and `docs/`; supporting records go into `artifacts/<HUID>/`.
  Follow this project's log format instead of the skill's default memory layout.
- `fpf` — design reasoning where claims, evidence, decisions and descriptions could mix.
- `dialogue` — a design session with the user; its artifacts go under `artifacts/<HUID>/` and conclusions go into the log.
- `adopt-purist-language-researcher` / `adopt-pragmatic-language-developer` — when the user
  asks for a critique or challenge of a language design choice.
- `systematic-debugging` — a failing test or unexpected behavior, before proposing a fix.
- `verification-before-completion` — before reporting code as done or committing it.

If a skill is not available, tell the user which one is missing and continue without it,
following the rules in this file.

Source: all of these live in https://github.com/UberDever/agents, under `skills/<name>/`.
To restore one, copy or link that folder into the harness's skills directory
(for Claude Code, `~/.claude/skills/<name>/`).
