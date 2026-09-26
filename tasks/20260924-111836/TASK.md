# K-V lists as a language concept

- STATUS: OPEN
- PRIORITY: 100
- TAGS: opcode
- KIND: TASK
- PARENT: 20241220-144435

- Direction (user, 2026-09-24): refine k-v lists into a language concept. They give optionality and free order. Metadata could be an optional k-v entry, present or absent, leaving the positional node well-formed.
- (user, 2026-09-24) Restriction: k-v entries go after positionals, enforced by the parser.
- [fact] Today labeled expressions are ordinary entries (C6, 2026-08-30): `[x: 2, y: 4]` is already a k-v list, and `label: expr` lowers to `[{:label}, meta, {label}, expr]`. Details: archive/20260807-120000/labels-kv.md.
- (agent) Challenge: blocks and lists may already contain labeled entries, so a trailing `meta:` entry can collide with payload. It needs a reserved key or tag.
- (agent) Conflict: the spec now puts `meta` positionally after the tag. A trailing optional k-v entry would replace that.
- (agent) Prior art, from general knowledge: Python keyword arguments (positionals first, parser-enforced), Elixir keyword lists (plain lists of pairs, sugar in last position), Lua tables (positional and keyed parts in one table), Common Lisp `&key` plists.
- Decision (user, 2026-09-24): `meta` is a reserved key, special like Lua metatables or Python's special fields. No other reserved keys.
- (user, 2026-09-24) Leaning: k-v `meta` over a positional slot that is sometimes empty. Not settled.
- (user, 2026-09-24) Idea: a list with zero or more k-v entries is a proper list; a pure k-v list has k-v entries only. Both fit the subtyping relation.
- (agent) Cost: a trailing `meta` entry is found by walking the whole payload; a block with a thousand statements walks a thousand cells. An optional `meta:` entry right after the tag keeps the lookup to one step, as an exception to "k-v after positionals".
- (agent) The subtypes are predicates on structure, like Scheme's `list?`: proper list, list with k-v entries, pure k-v list, tagged node. They are checked by walking, not declared.
- Decision (user, 2026-09-24): Vetochka has `meta`. The name is debatable.
- (user, 2026-09-24) A node's top level should be chunked: kind, meta, contents, data. It should never have a thousand top-level entries. A second-position `meta:` present on some nodes and absent on others is acceptable but less unified.
- (agent) [fact] Today's rewrite rules don't chunk: `do a; b end -> [{:block}, meta, a, b]` and `x op1 y op2 z -> [{:infix}, meta, [{op1}, {op2}], x, y, z]` put every statement or operand at the top level.
- Proposal (user, 2026-09-24, unreliable narrator): every node is `[{:stuff}, [meta-or-empty], ...]`. The meta list is always present, possibly empty; its fields are k-v entries, so each field is optional.
- Open: where the payload goes. Chunked reading (agent): `[{:stuff}, [meta...], payload]`, one payload entry.
- Leaning (user, 2026-09-24, unreliable narrator): `[{:stuff}, [meta...], payload]` rather than an embedded list, to avoid an extra node for the embedding on every node.

- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
