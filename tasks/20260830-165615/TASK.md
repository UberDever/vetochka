# Spec the binder discipline

- STATUS: OPEN
- PRIORITY: 100
- TAGS: binders, spec
- EPIC: 20241225-140425

- Settled direction (user, 2026-08-30), not yet in the spec by choice. Details: binders.md.
- TERM pairs a term with an env inside the machine only. The closure is the only env-carrying value.
- Values are embedded by building with application, behavior is kept with a thunk, and open syntax rebinds where it lands.
- Invariant: declared-no-value bindings exist only inside closures waiting for saturation.
