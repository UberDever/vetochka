# Git history recovery — 24.11.26

The user asked for a sweep of the whole Git history for documentation that the devlog and artifacts had missed.
The sweep found that GitHub held two disconnected histories, with the eight months between them missing.
The user then pushed that period from a second computer. This file records how the histories connect.

## Lines of history

| Line | Root | Span | Where |
| --- | --- | --- | --- |
| Python and first C evaluators | `c628d8d` | 14.12.24 – 23.09.25 | `feature/branch-first-eval-17.02.25`, `feature/native-eval-16.05.25`, `adding-scopes-to-parser`, `tokenizer-locations`, `main`; tags `v0.1.0`, `v0.1.1`, `v0.2.0` |
| nob.h and Lua side experiment | branches from `48af161` | 26.11.25 – 05.12.25 | `feature/new-stack-26.11.25` |
| Cells, reducer, spec rewrites | branches from `48af161` | 01.02.26 – 24.09.26 | `archive/pre-reroot-2026` (tip `4701b8b`, 81 commits not on any other ref) |
| Current master | `9bceff8` (no parent) | 24.09.26 – | `master` and later branches |

- `6b4cf93` "working on stuff" (01.02.26) has parent `48af161` "added first test using C library" (23.09.25).
  The February 2026 rewrite therefore continued the Zig-tested C library and did not start from the nob/Lua branch.
- `f4eea58` (31.08.26) merges `origin/master`. This shows that the 2026 line was GitHub's master at that time.
- `9bceff8` has no parent. Its tree differs from `4701b8b` only by one commit's change:
  10 files, about 130 lines, matching its message "Record tag, pith and metadata design; add meta to rewrite rules".
  Both commits carry the timestamp 24.09.26 14:50:26.
- Conclusion: the commit after `4701b8b` was recorded without a parent, and master was later force-pushed with the
  new line. The command or tool that did this is not known; the reflog of the computer where it happened would show it.
- `archive/work` (tip `150d386`) adds no commits. It is a stale copy of the PR #3 branch.
- The 10.11.26 migration inventory (`../20261110-000000/inventory.json`) cites 90 commits. All of them are now on GitHub.

## Survey method

`survey.py` walks every commit on every fetched ref, collects every added documentation line and code comment,
and reports the lines that survive nowhere today. "Today" means branch tips rooted at `9bceff8`, the working tree,
and the two migration tarballs. A line counts as preserved if 60% of its word 4-grams occur in that corpus,
so a rewording can count as lost even though its meaning survived.

```sh
git fetch origin '+refs/heads/*:refs/remotes/origin/*' '+refs/pull/*/head:refs/remotes/pr/*' '+refs/tags/*:refs/tags/*'
python3 -I artifacts/20261124-192408/survey.py . /tmp/unpreserved.json
```

Result before this artifact: 1,441 unpreserved lines. After copying the files under `sources/`: 890,
of which 601 are code comments. The remainder is judged to have no content worth keeping:
- vendored or generated text: the ctypes wrapper licence block, base64 and string-buffer helpers, pylint pragmas;
- commented-out code, test-case strings, and log/debug statements;
- intermediate checklist states of `TODO.md` and earlier wording of documents whose later versions are kept;
- superseded agent workflow rules (`AGENTS.md`, `.agents/skills/memory/SKILL.md`, memsearch notes).
  Git retains them on the branches above.

PR descriptions #1–#10 were checked; #3–#8 are reflected in the log and #9–#10 are open. The repository has no issues.
