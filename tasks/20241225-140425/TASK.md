# Specify and implement bindings, functions, and mutation

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders, spec
- KIND: TASK
- PARENT: 20260926-050130

- Work (agent): make lexical binding, function construction, raw-input forms, and the agreed mutation model precise and executable.
- Done when (agent): the agreed v0 forms have rules and tests for binding, saturation, shadowing, traps, and tail calls.
- Deferred scope (user, 2026-09-26): boxes remain recorded here, beyond the initial runtime-kind inventory.

## Current basis

- Decision (user, 2026-08-30): binders are the only nonlocality, apart from effects. Environments belong to binder nodes. Other fragments are pure trees.
- Decision (user, 2026-07-17): code remains nameful. Identifiers resolve through captured environments on demand. `LOCAL` is not language semantics.
- Decision (user, 2026-08-30): `$fn` extends its declaration environment. It replaces the separate `$f` form.
- Decision (user, 2026-08-30): executing an unbound identifier traps. Observation follows a bound value and otherwise exposes the raw name.
- Decision (user, 2026-08-30): closure piths expose declarators and body syntax, without an environment slot.
- Qualification (user, 2026-08-30): rebuilding syntax across a binder boundary rebinds it at the destination. Duality holds elsewhere.
- Open (agent, 2026-10-19): reconcile this exception with the reconstruction hypotheses in 20260926-091318. The hypotheses do not supersede the existing direction.
- Decision (user, 2026-08-30): machine TERM pairs support deferred evaluation but never become observable data. Closures are the environment-carrying values.
- Decision (user, 2026-08-30): declared bindings without values occur only in unsaturated closure data, not running control.
- Direction (user): embed values by application, preserve behavior in a thunk, and return literal trees for syntax that rebinds.
- Decision (user, 2026-07-09): a module is a function of its imports. There is no `GLOBAL` primitive.
- Direction (user, 2026-06-04): boxes provide mutation. Their representation and store semantics remain open.

## Forms and implementation

- Decision (user, 2026-08-30): let uses `let[clauses] do ... end`. `=` binds, `<~` binds recursively, and scope proceeds sequentially.
- Decision (user, 2026-08-30): a list-shaped clause forms a simultaneous knot. `_ = expr` runs an effect. Clause blocks require parentheses.
- Decision (user, 2026-08-30): `$form` accepts raw inputs. Its exact shape remains open.
- Decision (user): function arity is strict, without variadic parameter lists. Recursion is derived rather than a primitive `{rec}` form.
- Boundary (agent): 20250123-091611 owns opcode dispatch. 20260529-103914 owns machine control, sequencing, and proper tail calls.
- Evidence (agent): `binders.md` preserves the settled August direction. `identifier-context.md` is a working model, including dynamic declarators.

- History (agent): `archive/20261020-000000/records.md` preserves the original records. Its SUMMARY maps former subjects to their current owners.
