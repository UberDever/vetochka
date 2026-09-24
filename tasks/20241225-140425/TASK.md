# Scopes, modules and binders

- STATUS: OPEN
- PRIORITY: 100
- TAGS: epic, binders

- (user, 2026-08-30) Binder axiom: binders are the only nonlocality, side effects aside. Env belongs to binder nodes only; fragments are pure trees.
- (user, 2026-07-17) Model B: code stays nameful and identifiers resolve through the captured env on demand. `LOCAL` is rejected as semantics.
- (user, 2026-08-30) Executing an unbound identifier is an uncatchable trap. Observation never errors. A bound identifier is observed as its value.
- (user, 2026-08-30) `$fn` bodies are lexically scoped: they extend the declaration env.
- (user, 2026-07-09) A module is a function of its imports. `GLOBAL` is removed.
- Direction (user, 2026-08-30): `$fn` is the single universal construction form; `$f` is folded away.
- History: items.md.

## Tasks

- [ ] 20260604-111747 Boxes for mutability
- [x] 20260606-141939 Function arity: positional and labeled arguments
- [x] 20260711-131256 Recursion derived, not primitive
- [x] 20260805-162239 Experiment: alternatives to lambda abstraction
- [ ] 20260830-111134 Spec the let binding
- [ ] 20260830-165615 Spec the binder discipline
- [ ] 20260830-165616 Define $form
