# Syntax work

- [ ] Implement the accepted raw-byte literal syntax from `2026-11-07.md` in the parser and formatter. Check empty payloads, contiguous and spaced byte pairs, mixed hex case, malformed contents, and `~{` disambiguation. Coordinate runtime representation and lowering with 20260926-091318.
- [ ] Align the parser and encoder with the October 26 syntax additions in this task's `2026-10-26.md`: tag lists, binding, triage, and full-expression nyad children. The grammar and structural lowering are documented; execution contracts remain owned by 20260529-103914.
- [ ] Choose the Unicode identifier rule, then align the grammar, lexer, and tests.
- [ ] Define extensible number syntax without expanding the initial numeric runtime beyond the agreed scope. Floating-point grammar is tracked in 20261020-103514.
- [ ] Apply the literal and metadata decisions from 20260926-091318 to the encoder.
- [ ] Decide formatter idempotence. If required, add a regression case and correct newline handling.
- [ ] Check the recorded extra space before `...` and the handling of `#;` with inserted semicolons.
- [ ] Keep annotation layout consistent: no special newline exception after `]`.
- [ ] Implement annotation lowering according to the tagging decision in TASK.md, with the construction from 20260926-091318.
- [ ] Verify source-location metadata: empty by default, with the agreed opt-in fields.

Each item is work within this task. Completion requires evidence, not merely a decision to organize the work here.
