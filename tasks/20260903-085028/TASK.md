# Metadata list in lowered nodes

- STATUS: OPEN
- PRIORITY: 100
- TAGS: syntax, spec
- KIND: TASK
- PARENT: 20241220-144435

- Decision (user, 2026-08-30): lowered nodes carry a metadata list after the type tag: `[{node}, [version, line, file, ...], ...payload]`.
- Spans and provenance live in the tree, not in side tables. Programs can observe line numbers through it.
- ~~(2026-07-09) spans in a host side table keyed by cell index~~ superseded 2026-07-10, then by this.
- Open: the exact list, and how piths treat the slot.
- Decision (user, 2026-09-24): `meta` sits right after the tag of every lowered node: `[{node}, meta, ...payload]`. It is a k-v list whose fields depend on the node. Now in the spec's rewrite rules.
- ~~Open: rules without a tag slot have no `meta` place yet, although every node carries metadata.~~ resolved (user, 2026-09-24): lists, nyads and applications carry no metadata. Byte strings need a slot.
- ~~Open: the lowered shape of a byte string with its `meta` slot.~~ superseded (user, 2026-09-24): byte strings carry no `meta`; applications do, `[{@}, meta, ...]`.
- Decision (user, 2026-09-24): `parse_term` may have a mode where every node carries metadata, for example to locate a list. Off by default, since meta on every node would recurse: meta needing its own meta.
- Open (agent): what that mode does with the meta lists themselves.
- (user, 2026-09-24) No recursion: meta lists are inspected, not given meta of their own, for example via a `{:meta}` slot. The slot may be a k-v entry, see task 20260924-111836.
- (user, 2026-09-24) Metadata is data about data: what you need to inspect a value, not to use it through functions. Some nodes should have it. Concern: if only some nodes have metadata, a node can't be inspected reliably without first knowing its kind.
- ~~Decision (user, 2026-09-24): assume every node carries some metadata, possibly empty.~~ superseded the same evening: lists, nyads and applications carry none (resolved open item above), later refined for applications.
- Open (user, 2026-09-24): inspection should show metadata, so every pith would include it by default. That suggests two layers, language and representation, where language-level metadata need not be stored in the node itself. Debatable: it may break the hermetic principle ("As above, so below").
- Decision (user, 2026-09-24): applications carry `meta`: `[{@}, meta, ...]`. (agent) Consistent with the head rule: `{@}` already heads the node.
- Decision (user, 2026-09-24): byte strings carry no `meta`. Their meaning comes from the surrounding node, e.g. `{x}` inside `[{:id}, meta, {x}]`.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
