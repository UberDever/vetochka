# Opcode protocol and binders slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, opcode, binders, spec
- KIND: TASK
- PARENT: 20260529-103914

- Step 2 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091320.
- Work: a generic dispatcher for designator-headed lists with the Continue/Dispatch/hard-trap states (20250123-091611); then `$fn` closures with proper tail calls, `let[...]` (20260830-111134) and `$form` (20260830-165616), under the binder discipline (20260830-165615).
- Done when: the spec has their CESK rules, and tests cover currying, saturation, shadowing, tail calls in constant stack, and traps.
