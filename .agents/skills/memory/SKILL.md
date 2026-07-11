---
name: memory
description: Recall and distill persistent project memory stored in .memsearch/memory/ (dated markdown, semantically indexed). Use at the start of design discussions, when past decisions/rationale/debugging history might be relevant, when the user references earlier work, or at the end of a substantive session to record decisions. Also triggered by /skill:memory [recall <query> | distill].
---

# Project memory

Memory = dated markdown files in `.memsearch/memory/` (source of truth, git-tracked).
`memsearch` CLI provides hybrid semantic+keyword search over them. The vector index is
a disposable cache — never treat it as the data.

## Recall

1. Search: `memsearch search "<query>" --top-k 5` (run from project root).
2. Topic not project-specific (tooling, general patterns, lessons) or project
   collection empty → also try the shared collection:
   `memsearch search "<query>" --collection commons --top-k 5`.
3. Need full context of a hit: `memsearch expand <chunk_hash>`.
4. Nothing relevant → say so and continue; do not invent memories.
5. Treat results as hints. Verify remembered claims against the current code/docs
   before relying on them — memory goes stale.

## Distill (end of substantive session, or on request)

1. Append to `.memsearch/memory/YYYY-MM-DD.md` (today's date; create if missing) a
   section:

   ```markdown
   ## <topic> (HH:MM)
   - Decision: <what was decided and why>
   - Rejected: <alternatives and why not>
   - Open: <questions left unresolved>
   ```

2. Only durable knowledge: decisions, rationale, constraints discovered, gotchas.
   No play-by-play, no code listings (reference files/commits instead).
3. Contradicts an earlier entry? Do not delete old one; mark it
   `~~superseded~~ by <today's entry>` in place.
4. Note is genuinely cross-project (would help in any repo)? Propose moving it to
   the commons memory (`~/dev/agents/.memsearch/memory/`, indexed with
   `--collection commons`) instead, leaving a one-line pointer here. Ask first.
5. Re-index: `memsearch index .memsearch/memory/`.
6. Remind the user to commit the memory file with the related changes.
