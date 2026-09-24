# ADR 1: Adjacency-classified operator tokens

Status: accepted
Date: 2026-08-30

## Context

The parser must tell prefix from infix operator use without an operator table:
operator spellings overlap, the inventory is open, and vf — not v0 — assigns
meaning. Finding: the grammar has no juxtaposition, so `x - y` and `x -y` are
the same token sequence; the collision is purely lexical.

## Decision

The lexer classifies each operator run by adjacency (trivia breaks gluing):

1. glued to preceding identifier — continues the identifier (`x-y` is one name);
2. glued to preceding literal, `)`, or `]` — syntax error (`f(x)-y`, `1-2`);
3. else glued to right neighbor — `op_prefix` (`!x`, `-y`);
4. else — `op_infix` (`x - y`).

Grammar uses the two terminals:

```ebnf
infix_expression  ::= prefix_expression (op_infix prefix_expression)*
prefix_expression ::= op_prefix* postfix_expression
```

`x -y` fails with no production. The closed `prefix_operator` list is removed;
any operator run can be used prefix.

## Alternatives considered

- Parens-forced prefix context (`x (-y)`): subsumed — C1 yields it for free.
- Spacing-blind tokens + side rules: leaves `x -y` legal-but-forbidden by
  prose; rejected as implicit.

## Assumptions and revisit triggers

- No juxtaposition application ever enters the grammar. If it does, this ADR
  is void and disambiguation must be revisited.
- Reversible until a real code corpus exists.

## Consequences

- Easier: no fixity tables, explicit errors, one-char lexer peek (~10 lines).
- Given up: spaced prefix (`! x`), glued infix after non-identifier (`1-2`,
  `f(x)-y`) — all errors now. `!!x` is one operator spelled `!!`, not two `!`.
