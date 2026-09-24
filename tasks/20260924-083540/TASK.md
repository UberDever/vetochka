# Project license

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: process

- Decision (user, 2026-09-24): code is GPL-3.0-or-later (`LICENSE`).
- Decision (user, 2026-09-24): docs and design records, including AI output, are CC BY 4.0 (`LICENSE-DOCS`); it is public knowledge.
- Decision (user, 2026-09-24): generated code is not subject to the project license.
- [fact] Vendored code keeps its own licenses, all compatible with GPL-3.0: `stb_ds.h` MIT or public domain, `da.h` public domain, `arena.h` Apache-2.0 (Carter Dugan, 2024).
- [fact] tatr (GPL-2.0, Alexey Kutepov) is only a tool. `tasks/README.md` is our own text.
- [fact] `docs/smart/*.pdf` are third-party works outside the project license. SML Definition and Lafont's paper are likely not redistributable; the tree book is unknown.
- Done: `LICENSE`, `LICENSE-DOCS`, README license section, copyright line `Copyright (C) 2024-2026 uberdever` (user, 2026-09-24).
- Deferred: resolve `docs/smart/*.pdf` (remove or link). Decision (user, 2026-09-24): keep for now, likely remove later.
- Deferred: permissive carve-out for examples users copy into their programs.
- Deferred: a runtime-library exception if the compiler ever copies its own code into output.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
