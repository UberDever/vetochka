# Development-log migration — 2026-11-10

Scope (user): migrate all archive/ and tasks/ records to devlog/ and HUID-addressed artifacts. Consult Git when dates are uncertain. Preserve current working-tree changes.

Format: ../../devlog/format.md. Preferences: ../../devlog/preferences.json.

## Plan

1. Inventory every source file, including ignored files. Save hashes and original bytes before changing paths.
2. Extract prose, work items, and dated history into chronological entries. Preserve complex Markdown, code, PDFs, and complete source documents as artifacts.
3. Use explicit section/file dates. Otherwise, consult Git. Treat Git dates as snapshot evidence, not decision dates. Use the migration date for uncommitted or undated material. Record each date basis.
4. Assign IDs after sorting all entries. Translate old task references and preserve a source-to-entry mapping. Mark supersession only where supported by the source or where a newer snapshot replaces the same record.
5. Validate coverage, source preservation, date order, entry syntax, relation targets, and rotation boundaries before removing old source paths.
6. Update navigation and project directions. Verify changed references and rebuild the memory index for devlog/ and current docs/.

## Decisions and limits

- Decision (user): zero priority means none; all tags are explicit; relations precede bodies; entries can change without changing existing IDs.
- Decision (user): date sections increase monotonically; rotation occurs between sections; retain all history.
- Decision (agent): original sources remain in a compressed migration snapshot. Relocated Markdown may have path repairs, but source prose and attribution remain unchanged.
- Decision (agent): historical snapshots are labeled as historical. Closed historical work does not become current work. Existing proposals remain proposals.
- Decision (agent): use a one-time standard-library Python migration script. Do not introduce a dependency or a general-purpose tool during migration.
- Open (user): future graph tooling is not selected.
- Decision (user): use existing subject tags plus `#decision`, `#proposal`, `#evidence`, `#experiment`, and `#question`.
  Use `#superseded` as defined in the format. Keep rationale and snapshot provenance in the text.
  Do not add `#snapshot`, `#historical`, `#rationale`, `#constraint`, `#rejected`, or `#supersession` tags.

## Progress

- Inventory started. Existing spec moves and uncommitted task changes belong to the user and must be preserved.
- Complete: 138 source files, including 19 that differ from their HEAD source, are preserved in `originals.tar.gz`.
  `inventory.json` records initial hashes and Git evidence. `manifest.json` maps original paths, source lines, entry IDs, artifacts, and 73 legacy record IDs.
- Complete: 4,983 imported entries occupy 51 increasing date sections across `devlog/log.md.1` and `devlog/log.md`.
  The older rotation remains uncompressed because `compress_after` is 1.
  Date/entry identities remain provisional until this initial migration and its review finish; later edits must preserve them.
- Complete: old source paths were removed only after the original-byte backup and relocated artifact hashes passed verification.
  Markdown links were repaired where their targets could be established. Complete original source text remains available in the snapshot.
- Complete: active project instructions, README navigation, and two code/test comments reference the new record structure.
  Source and test executable text did not change. The configured memory collection remains `vetochka`.

## Review and verification

- Independent review found two Important errors: ISO date headings were ignored, and the phrase "Selected source lines" received `#decision`.
  Both findings were reproduced by failing regression checks and corrected.
- The date correction also recognizes explicit document collection dates and restores the parent date after a dated subsection ends.
  Qualifiers such as "user drunk; review sober" remain in entry text.
- `python3 artifacts/20261110-000000/test_migration.py`: 4 regression checks pass.
- `python3 artifacts/20261110-000000/verify.py`: validates all 138 source/archive hashes, relocated hashes, source-to-entry mappings,
  entry counts, date order, and relation targets. Deliberate date regressions, dangling relations, and artifact corruption were rejected in staging.
- Additional checks: no rejected tag names occur in entry metadata; source code and tests differ only in comments; `git diff --check` passes.
- Review did not evaluate runtime behavior, external URL availability, future tooling, or memory indexing. These were outside this data migration.
  No runtime logic changed. A retired header reference was checked in local Git and linked to that historical revision.

## Remaining questions

- Three original placeholder links remain unresolved: `04_vf.md` in the retained new-spec contents, `TODO:` in its syntax chapter,
  and `TODO:04_bootstap` in its v0 chapter. Their original text is retained; no replacement chapter was inferred.
- Automatic approval review rejected `memsearch index devlog/log.md devlog/log.md.1 docs/` because the new log payload may reach
  the configured memory service, while previous authorization names `tasks/` and `docs/`. No retry or workaround was attempted.
  Indexing requires user approval. The old index can contain stale paths and must not be treated as the migrated source of truth.

## Reproduction

The scripts are a one-time migration recipe, not new project tooling.
`inventory.py` created the initial snapshot and refuses to overwrite it.
`migrate.py` rebuilds a preview from that snapshot under `/tmp/vetochka-devlog-migration/`.
Its initial `apply` mode refuses existing destinations. Verification accepts an optional preview root.
Final session entries appended after the import are identified separately in the manifest.

## Final state

- Complete: 4,983 imported entries and 7 session entries, totaling 4,990 entries. Initial migration IDs are now fixed.
- Complete: original file permissions are preserved, including executable build scripts. A failing permission check reproduced the defect before restoration.
- Complete: `devlog/rules.md` defines operating procedures and is referenced by `AGENTS.md`.
- Pending: memory indexing requires the user approval described above.

## Subsequent semantic compaction

The user later requested semantic compaction while retaining the full replayable development WAL.
The mechanically imported log and this manifest's IDs are preserved in `../20261110-000002/before-compaction.tar.gz`.
They describe the preceding import, not the compacted active log. The active verification and reference scope are recorded in
`../20261110-000002/compaction.md`, `../20261110-000002/verify.py`, and `../20261110-000002/references.json`.
