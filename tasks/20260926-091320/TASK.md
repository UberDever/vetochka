# Machine core slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, reducer, spec
- KIND: TASK
- PARENT: 20260529-103914

- Step 1 of the v0 plan (user+agent, 2026-09-26). Needs 20260926-091318 and the build migration 20260606-141938.
- Work: CESK state in C; one-step `exec`; triage rules 0a..3c; identifier dereference; hard traps. Includes the Rule 3 fix, 20260718-175927.
- Spec side: the matching CESK rules in `03_v0.md` (20260711-131257), written with the code.
- Done when: the rules are in the spec, and tests run nyad-only and identifier programs through `exec` to a value, including Rule 3 on an application.
