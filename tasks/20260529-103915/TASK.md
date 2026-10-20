# C gains a modern frontend with compile-time meta; output stays ordinary C

- STATUS: OPEN
- PRIORITY: 100
- TAGS: framing
- KIND: INTENT

- Desired condition (user, 2026-05-28): a reflective, staged frontend emits ordinary C without carrying language semantics into the output.
- Why (user): preserve C's ability to interact with other systems while adding internal reflection during construction. See `porosity.md`.
- Direction (user, 2026-08-30): the stack is v0, then vmeta, then a vsystem compiler written in vmeta.
- Decision (user, 2026-08-30): internal porosity exists at build time. vsystem has no runtime reflection. Emitted C retains external porosity.
- Direction (user, 2026-08-30): an appliance is a language with required passes. Analysis libraries can implement disciplines such as ownership.
- Direction (user, 2026-08-30): the substrate is permissive. Appliances restrict it during their compile step.
- Documentation work (agent): `items.md` retains the introduction work within this motivation.

- History (agent): `archive/20261020-000000/records.md` preserves the original records. Its SUMMARY maps former subjects to their current owners.
