# Working with the development log

`format.md` defines syntax. `preferences.json` defines rotation and tool defaults.

## Purpose

`devlog/` is the project's development WAL and single source of truth.
It retains the complete chronological history, including all entries from the project's first two years.
Replaying entries must recover decisions, constraints, rationale, unresolved questions, and pending work as they developed.
A current-state summary alone cannot replace that history.

- Record meaningful state changes: adoption, rejection, revision, answers, reopened questions, and work-state changes.
- Preserve earlier directions and the reasons later entries changed them. Distinguish what was known then from later knowledge.
- Keep that replayable record in `devlog/`, including its rotations. Artifacts support entries; they must not become the only record of development history.

## Adjust content

- Extract actionable knowledge rather than summarize entries.
- Remove bookkeeping, artifact inventories, repeated metadata, and mechanical descriptions.
- Consolidate related decisions, constraints, rationale, questions, and pending work without merging distinct historical state changes.
- Reconcile later changes with earlier directions. Preserve meaningful evolution, contradictions, and details needed for replay or rationale.
- Retain source references selectively for consequential or non-obvious claims.
- Prefer a few information-dense statements. Preserve attribution, scope, and uncertainty.
- Apply these principles to every log adjustment while preserving existing IDs and replayable history.

## Read

- Search the current log and all rotations. Follow references to supporting artifacts.
- Read later entries that supersede relevant entries. Preserve attribution, uncertainty, and the distinction between proposals and approved decisions.
- During historical replay, apply supersession when its later entry is reached.
  The target's `#superseded` tag describes current status; it must not erase the earlier state.
- Verify claims against their sources and current code. Search indexes can omit rotations or contain stale paths.

## Write

- Append to the latest date section for today's local date. Start a later section when the date advances.
  Record newly discovered older information under today's date; state its original date in the body.
- Reread the file before writing. Reload if another writer changed it.
- Count recognized entries from zero within the date section. Reference entries by `(date, index)`.
- Keep each entry on one physical line. Put long reasoning, code, and experiments under `artifacts/<HUID>/`.
- Reuse subject tags. Add entry-kind tags only when their meanings apply:

| Tag | Meaning |
| --- | --- |
| `#decision` | An explicitly approved choice |
| `#proposal` | An unapproved candidate or hypothesis |
| `#evidence` | An observation, result, or cited support, with its scope |
| `#experiment` | Exploratory work |
| `#question` | An unresolved question |

- Keep rationale, constraints, and snapshot provenance in the text. Use checkboxes for work state.
- Use in-place edits for fixups that preserve replayable history. Record substantive state changes in new entries.
  Preserve existing IDs, entry order, and recognition as entries.
- To supersede an entry, append a replacement with `Supersession for` after its optional title.
  Mark the target `#superseded`. Preserve the earlier claim and its rationale.
- Use `Link to` for a generic reference. Explain any intended relationship in the body.
- Preserve cited artifact content. Give revisions new paths when they would invalidate existing citations.

## Check

- Check date order, entry syntax, unchanged existing IDs, and reference targets before reporting completion.
- Rotate only between complete date sections, according to preferences. Retain every rotation.
- At session end, record durable decisions, proposals, evidence, experiments, and open questions, with their reasons and sources.
