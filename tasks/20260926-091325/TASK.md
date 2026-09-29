# Host rim slice

- STATUS: OPEN
- PRIORITY: 100
- TAGS: host
- KIND: TASK
- PARENT: 20260613-151556

- Step 5 of the v0 plan (user+agent, 2026-09-26). Follows 20260926-091324.
- Work: the host entry `exec` (20260903-085029); `load_file`, `parse_term` (reusing `public/source/`), print and trap, modeled on WASI; a bootstrap v0 file that loads and runs a program.
- Done when: an executable runs a hello-world v0 program from a file end to end.
