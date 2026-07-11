## Memory discipline

- Project memory lives in `.memsearch/memory/` (dated markdown, git-tracked).
  Search it (`memsearch search "<query>"`) at the start of any design discussion or
  when past decisions might be relevant. Treat memory as hint, not fact — verify
  against real code before acting on remembered claims.
- At the end of substantive sessions, distill: append decisions made, alternatives
  rejected (and why), and open questions to `.memsearch/memory/YYYY-MM-DD.md`,
  then run `memsearch index .memsearch/memory/`. The `memory` skill has the exact
  procedure.
- Never delete memory entries; supersede them ("~~X~~ superseded by Y, see <date>").
