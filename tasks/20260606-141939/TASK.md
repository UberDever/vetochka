# Function arity: positional and labeled arguments

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: binders
- EPIC: 20241225-140425

- (user, 2026-06-06) Options: positional plus labeled arguments, or only positionals count.
- Decision (user, 2026-06-11): no variadics; named parameters count as parameters; strict signatures.
- Current form (spec): `$fn: [params]` has arity N; arguments come through `to:`.
