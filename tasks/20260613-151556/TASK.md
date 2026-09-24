# Host rim and bootstrap

- STATUS: OPEN
- PRIORITY: 100
- TAGS: epic, host

- (user, 2026-06-11) The executable launches bootstrap v0 code that loads sources and prepares them for execution.
- (user, 2026-07-11) `load_file` is the only required host effect. `parse_term` is pure and returns inert term data for the full shared syntax.
- (user, 2026-08-30) The host API is modeled on WASI: print, load and trap analogues. `parse_term` stays native by choice.
- (user, 2026-08-30) Load, parse, transform on the machine, then feed the root term. `exec` is the host-side door.
- ~~(2026-07-09) a `{specialize}` backbone intrinsic~~ superseded 2026-07-11 and 2026-08-30.
- (user, 2026-07-09) No capability machinery in v0. The June idea of capabilities in `initial_term` remains a direction.

## Tasks

- [ ] 20260903-085029 Specify exec, the host entry
