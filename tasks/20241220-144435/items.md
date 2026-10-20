# Syntax work

- [ ] Choose the Unicode identifier rule, then align the grammar, lexer, and tests.
- [ ] Define extensible number syntax without expanding the initial numeric runtime beyond the agreed scope. Floating-point grammar is tracked in 20261020-103514.
- [ ] Apply the literal and metadata decisions from 20260926-091318 to the encoder.
- [ ] Decide formatter idempotence. If required, add a regression case and correct newline handling.
- [ ] Check the recorded extra space before `...` and the handling of `#;` with inserted semicolons.
- [ ] Keep annotation layout consistent: no special newline exception after `]`.
- [ ] Implement annotation lowering according to the tagging decision in TASK.md, with the construction from 20260926-091318.
- [ ] Verify source-location metadata: empty by default, with the agreed opt-in fields.

Each item is work within this task. Completion requires evidence, not merely a decision to organize the work here.
