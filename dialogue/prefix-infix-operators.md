# Prefix vs infix operator disambiguation

Mode: lightweight (listen+question → generate+attack → converge+record).

## Framing

Question: by what rule does the parser tell a prefix operator use from an infix
one, when operator spellings overlap and the operator inventory is open (vf
defines meaning; the parser must never consult an operator table)?

Constraints (user rulings, 2026-08-30):
- [fact] operators are whitespace-separated tokens; operator chars glued to an
  identifier continue that identifier (`x-y` is one identifier).
- [fact] lone `:` is punctuation; multi-char operators may contain `:` (`::`).
- [fact] mixed infix chains parse flat; vf analyzes them.
- [preference] `x -y` (space before, glued after, right of a complete
  expression) must be a syntax error, not a disambiguator.
- [fact] prefix form stays; `!x` and the like are wanted surface syntax.
- [assumption] decision is reversible while the spec is hand-fluid; no corpus
  exists yet.

## Model (to confirm)

- The parser knows operator spellings only, never their fixity or meaning.
- The only ambiguous position is directly after a token that can end an
  expression (same token class ASI already uses). Everywhere else — start of
  expression, after `(`, `[`, `,`, `;`, after another operator, after a label
  `:` — a glued operator can only be prefix.
- User's sketch: force parens to enter prefix context in the ambiguous spot,
  e.g. `x (-y)`.

## Candidates

C1 (agent): adjacency-classified operator tokens. Lexer classifies each
operator run: glued to preceding identifier -> identifier continuation;
whitespace both sides -> `op_infix`; preceded by ws/opener/operator and glued
to operand -> `op_prefix`; glued to preceding literal/`)`/`]` -> lexical error.
Grammar uses two terminals: `infix_expression ::= prefix_expression (op_infix
prefix_expression)*`, `prefix_expression ::= op_prefix* postfix_expression`.
`x -y` then fails with no production, explicitly. No operator table needed.

Key finding: no juxtaposition in the grammar, so with spacing-blind tokens
`x - y` and `x -y` are the same parse; the collision is purely lexical. Hence
disambiguation belongs in token classification, not in productions.

C0 (user sketch, subsumed): force parens `x (-y)` — C1 gives this for free,
since parens re-enter opener context where `op_prefix` is legal.

## Objections

- O1 [accepted cost]: spaced prefix `! x` illegal under C1.
- O2 [accepted cost]: `f(x)-y`, `1-2` become errors (operator glued to
  non-identifier). User: consistent with the rest of the language.

## Decision

C1 accepted (2026-08-30). See `adr-1-operator-adjacency.md`. Applied to
`docs/new_spec/02_syntax.md`: operator_run + 4-rule adjacency classification,
`op_infix`/`op_prefix` terminals, closed `prefix_operator` list removed.
Lexer cost: one-char peek, two branches, trivia breaks gluing.
