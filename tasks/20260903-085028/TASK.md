# Metadata list in lowered nodes

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- EPIC: 20241220-144435

- Decision (user, 2026-08-30): lowered nodes carry a metadata list after the type tag: `[{node}, [version, line, file, ...], ...payload]`.
- Spans and provenance live in the tree, not in side tables. Programs can observe line numbers through it.
- ~~(2026-07-09) spans in a host side table keyed by cell index~~ superseded 2026-07-10, then by this.
- Open: the exact list, and how piths treat the slot.
