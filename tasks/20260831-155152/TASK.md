# Paren-statements candidate

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, rejected
- EPIC: 20241220-144435

- Candidate: `(a; b)` as a block, dropping `do ... end`. It loses the loose trailing block `f do ... end`.
- ~~Decision (user, 2026-08-30): deferred until a bootstrap corpus in the current grammar decides.~~ superseded by the 2026-09-24 ruling.
- Rejected (user, 2026-09-24): no paren-statements. `do ... end` stays the block form. R1 gluing stays.
- Follow-up: the grouping question became task 20260924-092637.
- Details: labels-kv.md in the syntax epic.
- Context (user): decided on the evening of 2026-09-24 while the user was drunk. Review sober.
