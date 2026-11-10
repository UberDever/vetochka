# Semantic WAL compaction

Authorized by the user on 2026-11-10. The first import produced mechanical paragraph records rather than a usable development WAL.

## Requirements

- Keep the full development timeline in devlog/, from the first recorded work through the current direction.
- Retain decisions, constraints, rationale, hypotheses, contradictory directions, unresolved questions, and pending work needed to recover historical development state.
- Remove inventories, repeated metadata, mechanical excerpt labels, and repeated statements that add no state change.
- Prefer dated source statements. Use Git to distinguish cumulative notes from original events. Mark first-recorded dates and uncertain attribution honestly.
- Preserve sources selectively for consequential claims. Artifacts supplement the WAL; they do not replace its substantive content.

## Method

1. Independently extract early and middle history; reconcile with the later language redesign and current source.
2. Integrate chronologically, resolve duplication, and retain the actual sequence of supersession and open choices.
3. Assign IDs only after integration. This user-authorized initial cleanup rebuilds IDs; ordinary future edits preserve them.
4. Update active references. Verify syntax, chronological order, relation targets, source preservation, and coverage of each major transition.

## Reference scope

before-compaction.tar.gz preserves the preceding log and its manifest. IDs in the original import manifest and migration artifacts refer to that preceding version.
The new active log receives its own IDs. Historical citations in retained artifacts remain historical; they must not be interpreted as references into the compacted log.

## Progress

- Prior log and reference scope saved. Source documents remain unchanged.

## Outcome

- Replaced the mechanical import with 249 semantic entries across 55 dates, from 2024-12-25 through 2026-11-10.
- Reduced 156,759 words to 9,766 words (93.8%). Earlier states, changes of direction, constraints, rationale, unresolved choices, and pending work remain in the active WAL.
- Added 13 dated supersession relations. Targets are limited to the policies actually replaced; retained syntax and control constraints remain separate.
- Updated the reducer and Unicode-test comments to the new active IDs. The README's February 17 reference still identifies the corresponding decision.
- `references.json` records active reference changes and the preceding ID scope. The original import manifest remains historical and is not an active-log index.
- `AGENTS.md` explains that scope. `devlog/rules.md` explains that replay applies supersession only when the later entry is reached, despite current-status tags on earlier entries.

## Review corrections and date evidence

- Restored the pith reconstruction contract and the later one-layer/pith-child direction. Local Git places the basic duality contract in the August 5 handwritten spec (`dbb6c0f`) and the depth direction in the September 24 record (`9bceff8`). Neither is silently backdated to the other.
- Split August's calculus/effect policy from its pending-application Rule 3 policy. September 25 supersedes only the latter. October's lambda redesign separately replaces the calculus basis.
- Restored the pending influences work. Git `dbb6c0f` already requests explanations of tree/triage calculus, lambda calculus, C, ML, and Scheme, with sources and affected levels.
- Split retained bare-leaf/list notation from the homogeneous-value assumption, so November's Bytes choice replaces only the latter.
- Separated the reopened block mechanism from retained tail-call requirements. Retained literal-tag and header-layout edits separately so October 27 reverses their exact scopes.
- Retained the September 24 sober-review caveat and uncertain July first-recorded dates. Historical experiment results remain reports, not fresh conformance evidence.

## Verification

- `python3 artifacts/20261110-000002/test_semantics.py`: four review regressions reproduced before correction and pass afterward.
- `python3 artifacts/20261110-000002/verify.py`: checks chronology, valid entry metadata, relation scope/order/targets, citations, active references, preceding-log backup hashes, and source integrity.
- All 138 original files remain byte-verifiable in the original snapshot. The 136 currently retained artifacts match their recorded content and permissions.
- Two generated specification PDFs are now absent from their artifact folders. The compaction did not remove or recreate them; their original bytes remain in `artifacts/20261110-000000/originals.tar.gz`. No active semantic entry depends on them.
- Source and test changes affect reference comments only. Runtime tests were not rerun for this prose/data change.
- The whitespace check passes for AGENTS.md, README.md, and the two reference-comment files. A full working-tree check also reported trailing whitespace in the concurrently present `docs/spec/01_introduction.md:29`; that unrelated document was left unchanged.
- Memory indexing was not retried. The earlier approval restriction still applies.
