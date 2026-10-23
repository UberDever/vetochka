# Value work

- [ ] Specify basic value concepts using Scheme terminology (user, 2026-10-22): disjointness of types, external representations, storage model, and equivalence predicates. Cover `~`, `[]`, `{}`, and `0`, including native strings. Relate these concepts to application under rules 0–3 and closure reduction. Distinguish type recognition from payload validity, equivalence under application from equivalence under inspection, and conceptual storage from physical cell layout. State what native representations and reconstructed piths must preserve. Done when the language spec defines these concepts with examples, coordinated with evaluation task 20260529-103914 and syntax task 20241220-144435. Scope and Scheme comparison: `2026-10-22.md`.
- [ ] Review the September 24 representation rulings and reconcile literal leaves, tagged forms, and designator strings.
- [ ] Decide the pith model needed by the machine: direct node observation or a separate view.
- [ ] Specify each initial kind: nyad, list, integer, bytes, application, identifier, closure, opcode state, native handle, and pith.
- [ ] Resolve metadata placement, payload grouping, k-v ordering, and the metadata inspection boundary.
- [ ] Specify k-v collection semantics (user, 2026-10-20): plist, alist, map, or another model. Decide key equality, duplicate keys, ordering, missing-key behavior, and updates.
- [ ] Specify observation versions, their runtime encoding, coexistence, and any role in behavior selection.
- [ ] ~~Explore annotation laws corresponding to `tag_apply` and `getTag_tag`. Separate them from dispatch semantics.~~ Superseded by the custom-tagging assessment below (user, 2026-10-23).
- [ ] Assess a Vetochka tagging mechanism instead of directly adopting Jay's construction (user, 2026-10-23). Application should ignore tags by construction. Tags should compose: the tentative example `@[a] @[b] x` would combine the annotations, approximately as `@[[a, b]] x`; the exact law and syntax remain open. Define how tagging interacts with structural inspection, currently called Rule 3. Coordinate with the evaluation redesign in 20260529-103914. Done when the tag representation, composition law, application behavior, and inspection behavior are specified and checked together.
- [ ] Clarify reconstruction guarantees at binder boundaries with 20241225-140425 and at host boundaries with 20250123-091611.
- [ ] Revise cell kinds and storage as required. Decide the fate of `APPLY`, `OP_FN0..2`, and dedicated native heads.
- [ ] Replace stale Cells wording with the agreed runtime-node and pith model.
- [ ] Implement both literal forms and all initial piths. Test observation, construction, and the agreed limits of duality.

The identity discussion remains research. An executable identity test or an enumerable relation set is not an agreed deliverable.
