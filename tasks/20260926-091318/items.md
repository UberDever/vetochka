# Value work

- [ ] Review the September 24 representation rulings and reconcile literal leaves, tagged forms, and designator strings.
- [ ] Decide the pith model needed by the machine: direct node observation or a separate view.
- [ ] Specify each initial kind: nyad, list, integer, bytes, application, identifier, closure, opcode state, native handle, and pith.
- [ ] Resolve metadata placement, payload grouping, k-v ordering, and the metadata inspection boundary.
- [ ] Specify k-v collection semantics (user, 2026-10-20): plist, alist, map, or another model. Decide key equality, duplicate keys, ordering, missing-key behavior, and updates.
- [ ] Specify observation versions, their runtime encoding, coexistence, and any role in behavior selection.
- [ ] Explore annotation laws corresponding to `tag_apply` and `getTag_tag`. Separate them from dispatch semantics.
- [ ] Clarify reconstruction guarantees at binder boundaries with 20241225-140425 and at host boundaries with 20250123-091611.
- [ ] Revise cell kinds and storage as required. Decide the fate of `APPLY`, `OP_FN0..2`, and dedicated native heads.
- [ ] Replace stale Cells wording with the agreed runtime-node and pith model.
- [ ] Implement both literal forms and all initial piths. Test observation, construction, and the agreed limits of duality.

The identity discussion remains research. An executable identity test or an enumerable relation set is not an agreed deliverable.
