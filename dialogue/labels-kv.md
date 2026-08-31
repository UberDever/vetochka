# K-V lists and label syntax

Mode: lightweight.

## Framing

Question: how are k-v collections written — convention over existing label
applications (`k-v x: 1 ...`), or a reserved sigil form (`| foo: bar ...`)?

User examples (2026-08-30):

```vetochka
map = Map[[{x}, 2], [{y}, 4]]      ;; verbose
map = Map[{x}, 2, {y}, 4]          ;; flat list
map = Map x: 2 y: 4                ;; label-oriented
k-v x: 123 y: 256 [{::}, ~[]]      ;; option 1: convention, mixed entries
| foo: bar bee: baz [{str}, stuff] ;; option 2: sigil form
| nest: (| just: like) that: 42
```

- [fact] labels already lower to data applied to a head: `f x: 1` ->
  `{@} f [{:label}, {x}, 1]`. A label chain on any head IS a k-v list; the
  head interprets.
- [preference] lone `|` could be banned like lone `:` (user); `=`, `|` already
  removed from identifier_continue.
- [ruling] no passing bare `$`; operator-as-datum uses desugared forms.

## Findings (agent, pass 1)

- F1: both mixed examples are ungrammatical today. `postfix_expression ::=
  primary tight_postfix* loose_postfix*` — a bracket/paren (tight) cannot
  follow a labeled argument (loose). Positional-verbatim-after-labels is the
  real question, shared by both options.
- F2: `|` as a `$`-style quasiid is nearly free: lower `| -> [{:id}, {|}]`,
  existing label machinery chains onto it. Nesting via parens as in the user
  example. No new grammar beyond the token.
- F3: but if the `|`-form takes positional entries, it needs juxtaposition
  inside the form — brushes ADR-1's revisit trigger (contained, but real).
- F4: grammatical-today alternatives for verbatim entries: wrap under a
  reserved label (`k-v x: 123 raw: [[{::}, ~[]]]`) or pass a bracket list
  positionally first (`k-v([...]) x: 123`? — tight before loose, legal).
- F5: cost of sigil `|`: lone `|` infix (pipe style `x | f`) dies forever;
  `||` survives.

## Candidates (pass 2)

- C2 (agent): `|[entry, ...]` sigil form, comma-separated entries
  `key ":" expr | expr`; grammar-verified pairs; costs the `|` sigil.
- C3 (agent, favored): colon entries in ordinary brackets — `x: 2` is not a
  legal expression today, so `[x: 2, y: 4]` is free syntax space. entry ::=
  expression | key ":" argument_expression; key ::= label | string_literal.
  `Map[x: 2, y: 4]` works via rule 9; mixed and nested entries free; entry
  lowers to `[{:label}, {k}, v]` list element. No new tokens; `|` stays free
  (pipe spellable as `|` or `|>`).
- C4 (user sketch): headless loose chain `| foo: bar ... [..]` — needs
  positional-after-label + in-form juxtaposition (ADR-1 trigger); parens for
  nesting. Weakest.
- C5: convention only — no grammatical verification; fails stated rationale.

User rationale [preference]: k-v is general enough for syntax level; verify
structure grammatically. Slippery-slope worry (bytevectors next?) — C3 answer:
no sigil spent, brackets remain the single data-shape syntax.

## C6: labeled expressions (user convergence insight, pass 3)

User: is the loose label arg just unparenthesized application of a standalone
labeled expression `foo: bar`? Assessment: yes — lowering rule 6 already
applies a label term; grammar can make `label ":" argument_expression` a
first-class expression. Free syntax space verified: bracket entries, call
parens, group contents, statements — `label ":"` errors today in all of them.
Payload stays tight (nesting/chaining needs parens: `x: (y: 1)`); labeled
expression excluded from bare infix/prefix operand position; adjacency still
never applies (loose postfix remains the only attached parse) — ADR-1 safe.
ASI cooperates: `:` cannot end an expression.

Delta: entry ::= labeled_expression | expression in comma_list/block_list and
group; lowering `label: expr -> [{:label}, {label}, expr]`; rule 6 becomes
ordinary application. Subsumes C3 (and C2/C4 die: no sigil needed).

## Attachment principle (pass 4)

User asked why tight/loose ordering can't be relaxed. Answer: each postfix
must have an owner. Glued postfix belongs to the nearest atom (payloads may
carry them: `f x: 1(a)` is legal). Spaced tight postfix after a loose chain
has no owner — that shape IS juxtaposition, absent from the language. Parens
name the owner: `(f x: 1)(a)`.

Proposed ruling R1: tight postfixes must be glued; spaced labels/blocks attach
to the head chain. Consequences: `f (a)` is not a call; `[a (b)]` is a caught
missing-comma error; the tight*/loose* ordering follows from one principle.

Rejected relaxation: true juxtaposition (`f x` = apply) — would void ADR-1
(`x -y` reinterpreted) and ASI rule 3. C6's five application shapes
(`()`, `[]`, `{}`, label, do-end) are the DSL surface instead.

## Decision

C6 + R1 accepted (2026-08-30). Applied to `docs/new_spec/02_syntax.md`:
labeled_expression replaces labeled_argument and is a standalone entry in
comma_list/block_list/group; tight postfixes must be glued (`f (a)` error);
loose spacing immaterial; lowering rule 6 split into 6 (label term) and 6a
(application of it). `|` sigil not spent; C2/C4/C5 dead.
Application shapes: `f(args)`, `f[list]`, `f{bytes}`, `f x: v`, `f do end`.

## Addendum: block promotion (accepted 2026-08-30)

Bare `do ... end` promoted to standalone entry, symmetric with C6: lowers to
inert `[{:block}, ...]` (rule 7/7a split); execution still only via consumer.
Cost accepted: `x` NL `do a end` = two statements, not an error; user rule of
thumb — to split intentionally write `do` NL `... end` or `if:` NL `do...end`.
Note: `f do a end` and `f(do a end)` lower identically ({@} f block-datum);
call parens are application, not a group, so no `[{:group}]` wrapper.

## Standing candidate: paren-statements (2026-08-30, deferred to corpus)

Proposal: `(a; b)` as statement sequence lowering to `[{:block}, ...]`, drop
`do end`. Needs: layout-active parens; `(a)` group erased, `(a;)`/`(a; b)`
block (trailing `;` disambiguates, mirrors trailing comma). Deletes: two
reserved words, block_argument, layout special case. Loses: loose trailing
block `f do ... end` (best DSL shape; application shapes 5 -> 4), `do:`/`end:`
labels, vsystem imperative skin. Rule-count roughly unchanged.
Ruling: deferred — bootstrap corpus in current grammar decides.

Gluing question re-asked and held: R1 stays; spacious DSL heads use labels
(`let x: 2 y: 4 in: (...)` is legal today), glued forms are the mechanical
layer.
