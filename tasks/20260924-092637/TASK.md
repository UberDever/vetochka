# Lowering of mixed infix chains

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: syntax, spec
- EPIC: 20241220-144435

- [fact] Rewrite rule 12 in `docs/new_spec/02_syntax.md` shows a single operator: `x op y op z -> [{:infix}, {op}, x, y, z]`. It doesn't say how a chain with different operators lowers.
- Decision (user, 2026-09-24): operators in a chain may differ. Without parens the whole chain is flat, in one list.
- [fact] Parens already keep grouping apart: `(a + b) * c` has the lowered `a + b` as one operand, while `a + b * c` is one flat chain.
- Decision (user, 2026-09-24, spec rule 12): `x op1 y op2 z -> [{:infix}, [{op1}, {op2}], x, y, z]`. Operators go in a list; operands stay flat. vf stages analyze them.
- Background: paren-statements task 20260831-155152, closed the same day.
