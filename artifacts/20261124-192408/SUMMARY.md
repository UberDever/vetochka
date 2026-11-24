# Documentation recovered from the whole Git history

The 24.11.26 sweep of every commit on every branch, tag and pull-request ref, including the Feb–Sep 2026 history
that the user pushed to `archive/pre-reroot-2026`. It collects documents, notes and code intent that no retained
file preserved. Log entries: (24.11.26, 0) to (24.11.26, 27).

- `history-recovery.md` — how the old and current histories connect, the re-root of master, and the survey method.
- `2024-2025.md` — the year the devlog skips: the original C-preprocessor goal, the 2025 grammar, language levels,
  the Python pipeline, the second C evaluator, natives, and the Zig and nob/Lua build moves.
- `2026.md` — what the recovered 2026 commits add to the log, and an index of the spec revisions kept here.
- `timeline.md` — every commit before `9bceff8`, with the line it belongs to.
- `survey.py` — the survey that finds documentation and comment lines preserved nowhere.
- `sources/<commit>/<path>` — files copied verbatim from Git, unchanged:

| Commit | Date | Path | Why kept |
| --- | --- | --- | --- |
| `8ff2f02` | 25.01.25 | `TODO.md`, `docs/grammar.md` | Python-era plan and checklist; grammar with intrinsics and `use` forms |
| `0999763` | 07.02.25 | `README.md` | first README, tag `v0.1.0` |
| `84e6f93` | 16.02.25 | `docs/language.md` | final 2025 grammar and the Vetochka 0–3 levels |
| `85d5881` | 21.05.25 | `TODO.md` | final requirements, ideas, tagging essay and level checklists |
| `ee4712d` | 21.05.25 | `eval/state_schema.json`, `eval/tests/*.json` | machine-state test format of the second evaluator, tag `v0.2.0` |
| `cc34eed` | 02.04.26 | `README.md` | the three reasons for the project and the phase summary |
| `27fc844` | 30.05.26 | `docs/grammar.md` | first rewrite rules; mixed infix rejected |
| `4de0cb8` | 30.05.26 | `project/vetochka_uniform.tree` | first full vf-style example program |
| `1f3aa1d` | 10.06.26 | `docs/grammar.md` | rewrite rules with byte-string tags |
| `6b0bfc5` | 13.06.26 | `PROJECT.md` | callable protocol and implementation order |
| `4cca930` | 15.06.26 | `PROJECT.md` | last version before it became `project_history.md` |
| `9cc70f6` | 15.07.26 | `docs/spec/` | five-layer v0 tower and binary cell ABI |
| `2f24752`, `d126d83`, `907874e` | 16–18.07.26 | `docs/spec/03_v0.md` | opcode, TERM and small-step machine revisions |
| `a66220f` | 21.07.26 | `docs/spec/` | last machine-written spec, with LENS |
| `dbb6c0f` | 05.08.26 | `docs/new_spec/` | first handwritten spec |
| `f10c3a0`, `c75edb4` | 30.08.26 | `docs/new_spec/` | caret notation, then nyads and the unified grammar |
| `39f7116` | 21.10.26 | `docs/new_spec/02_syntax.md` | core and general syntax; tagging guarantees |

Relative links inside copied files point to their original locations and may not resolve here.
