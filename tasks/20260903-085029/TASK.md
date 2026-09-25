# Specify exec, the host entry

- STATUS: OPEN
- PRIORITY: 100
- TAGS: host, machine, spec
- EPIC: 20260613-151556

- (user, 2026-08-30) `exec` is a host-side, top-level, apply-once entry. It forces a value term containing `{@}`.
- Direction (user): `exec` is likely not user-accessible; applying one value to another already interprets code in the language.
- Open: its exact semantics in the spec.
- Direction (user, 2026-09-24): `exec` follows a meta-ignoring principle. Turning `[{@}, meta, f, x]` into a machine application drops `meta`, as Jay's tags never change what a function does (tree book 5.4).
- (agent) Refinement, not ruled: the rule covers all execution, `exec` and every opcode that reads raw syntax, since those nodes carry `meta` too. Wording: `meta` never changes what execution computes; reading it for diagnostics is allowed.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
