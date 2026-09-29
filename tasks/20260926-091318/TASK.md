# Settle value representation

- STATUS: OPEN
- PRIORITY: 100
- TAGS: machine, encoding
- KIND: TASK
- PARENT: 20260925-134730

- Step 0 of the v0 plan (user+agent, 2026-09-26): decisions only, before any machine C code. Plan: 20260529-103914/2026-09-26.md.
- Work (user, 2026-09-26): sober review of the rulings marked "Review sober" (2026-09-24), in 20260209-162003, 20260903-085028, 20260805-162238, 20260903-085029, 20260606-141938.
- (agent) [fact] Three rulings pull apart: general values such as i64 and strings are leaves (20260209-162003, 2026-09-24); opaque nodes are distinguished by the tag first in their list (same task, same day); literals come in two tagged forms (20260926-053139, 2026-09-26).
- Decide: one value representation for integers, byte strings and designators; the fate of the `APPLY` and `OP_FN0..2` cell tags; the pith model Rule 3 works on (open in 20260805-162238); who repeats the one-step `exec` (open in 20260903-085029).
- Done when: each decision above is recorded as a user ruling in its owning task.
