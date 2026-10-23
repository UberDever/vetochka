# Binding work

- [ ] Assess A-normalization as a basis for explicit computation and binding in the lambda-based evaluation redesign (user, 2026-10-23; 20260529-103914). Tentative surface syntax: `=[a: 12, b: f()] do ... end`. Bindings would perform computations as in A-normalization. The syntax and adaptation require assessment before adoption. Clarify evaluation order, scope, permitted computation positions, and the relation to the earlier let forms. Prior discussion source: TASK.md.
- [ ] Reconcile the identifier-context working model with the binder rules, including dynamically constructed parameter declarations.
- [ ] Specify and implement `$fn` construction, lexical capture, input handling, currying, and saturation.
- [ ] Replace the old let sketch with the agreed sequential and recursive clause forms.
- [ ] Evaluate replacing `$form` with explicit closures, and resolve the zero-parameter thunk conflict recorded in TASK.md.
- [ ] Specify derived recursion, including mutual recursion through collections of functions.
- [ ] State the closure reconstruction boundary and its relation to captured values and shared mutable references.
- [ ] Test shadowing, outer-context declarators, raw inputs, unbound traps, and the declared-without-value invariant.
- [ ] Test tail calls in constant machine stack using the control contract in 20260529-103914.
- [ ] Later: decide box construction, access, mutation, storage, and observation.
